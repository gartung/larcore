/**
 * @file   GeometryTestAlg.cxx
 * @brief  Unit test for geometry functionalities: implementation file
 * @date   2011/02/17
 * @author brebel@fnal.gov
 * @see    GeometryTestAlg.h
 */

// our header
#include "test/Geometry/GeometryTestAlg.h"

// LArSoft includes
#include "SimpleTypesAndConstants/geo_types.h"
#include "SimpleTypesAndConstants/PhysicalConstants.h" // util::pi<>
#include "Geometry/GeometryCore.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "Geometry/OpDetGeo.h"
#include "Geometry/AuxDetGeo.h"
#include "Geometry/geo.h"

// Framework includes
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// ROOT includes
#include "TGeoManager.h"
#include "TStopwatch.h"

// C/C++ standard libraries
#include <cmath>
#include <vector>
#include <iterator> // std::inserter()
#include <algorithm> // std::copy()
#include <set>
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include <limits> // std::numeric_limits<>


namespace {
  template <typename T>
  inline T sqr(T v) { return v*v; }
  
  template <typename T>
  std::string to_string(const T& v) {
    std::ostringstream sstr;
    sstr << v;
    return sstr.str();
  } // ::to_string()
  
  
  /// Returns whether the CET exception e contains the specified category cat
  bool hasCategory(cet::exception const& e, std::string const& cat) {
    for (auto const& e_category: e.history())
      if (e_category == cat) return true;
    return false;
  } // hasCategory()
  
  
  /// Returns a C-string with the name of the view
  const char* ViewName(geo::View_t view) {
    switch (view) {
      case geo::kU:       return "U";
      case geo::kV:       return "V";
      case geo::kZ:       return "Z";
      case geo::k3D:      return "3D";
      case geo::kUnknown: return "?";
      default:            return "<UNSUPPORTED>";
    } // switch
  } // ViewName()
  
} // local namespace


namespace simple_geo {
  
  struct Point2D {
    double y = 0.;
    double z = 0.;
    
    Point2D() = default;
    Point2D(double new_y, double new_z): y(new_y), z(new_z) {}
  }; // struct Point2D
  
  Point2D operator+ (Point2D const& a, Point2D const& b)
    { return { a.y + b.y, a.z + b.z }; }
  Point2D operator* (Point2D const& p, double f)
    { return { p.y * f, p.z * f }; }
  Point2D operator/ (Point2D const& p, double f)
    { return { p.y / f, p.z / f }; }
  template <typename Stream>
  Stream& operator<< (Stream& out, Point2D const& p)
    { out << "( " << p.y << " ; " << p.z << " )"; return out; }
  
  class Area {
      public:
    Area() = default;
    
    Area(Point2D const& a, Point2D const& b)
      {
        set_sorted(min.y, max.y, a.y, b.y);
        set_sorted(min.z, max.z, a.z, b.z);
      } // Area(Point2D x2)
    
    Point2D const& Min() const { return min; }
    Point2D const& Max() const { return max; }
    Point2D Center() const { return (min + max) / 2; }
    double DeltaY() const { return Max().y - Min().y; }
    double DeltaZ() const { return Max().z - Min().z; }
    bool isEmpty() const { return (DeltaY() == 0) || (DeltaZ() == 0); }
    
    void IncludePoint(Point2D const& point)
      {
        set_min_max(min.y, max.y, point.y);
        set_min_max(min.z, max.z, point.z);
      } // Include()
    
    void Include(Area const& area)
      { IncludePoint(area.min); IncludePoint(area.max); }
    
    void Intersect(Area const& area)
      {
        set_max(min.y, area.min.y);
        set_min(max.y, area.max.y);
        set_max(min.z, area.min.z);
        set_min(max.z, area.max.z);
      }
    
      protected:
    Point2D min, max;
    
    void set_min(double& var, double val) { if (val < var) var = val; }
    void set_max(double& var, double val) { if (val > var) var = val; }
    void set_min_max(double& min_var, double& max_var, double val)
      { set_min(min_var, val); set_max(max_var, val); }
    void set_sorted(double& min_var, double& max_var, double a, double b)
      {
        if (a > b) { min_var = b; max_var = a; }
        else       { min_var = a; max_var = b; }
      }
  }; // class Area
  
  
  simple_geo::Area PlaneCoverage(geo::PlaneGeo const& plane) {
    simple_geo::Area plane_area;
    // add both coordinates of first and last wire
    std::array<double, 3> end;
    geo::WireGeo const& first_wire = plane.FirstWire();
    first_wire.GetStart(end.data());
    plane_area.IncludePoint({ end[1], end[2] });
    geo::WireGeo const& last_wire = plane.LastWire();
    last_wire.GetEnd(end.data());
    plane_area.IncludePoint({ end[1], end[2] });
    return plane_area;
  } // PlaneCoverage()
  
} // namespace simple_geo


namespace geo{
  
  
  namespace details {
    
    /// Checks the test and records the request
    bool TestTrackerClassBase::operator() (std::string test_name)
      { return Query(test_name); }
    
    TestTrackerClassBase::TestList_t TestTrackerClassBase::QueriedTests() const
    {
      TestList_t all;
      std::set_union(SkippedTests().begin(), SkippedTests().end(),
        RunTests().begin(), RunTests().end(),
        std::inserter(all, all.end())
        );
      return all;
    } // QueriedTests()
    
    bool TestTrackerClassBase::CheckQueriesRegistry() const
      { return true; /* all fine */ }
    
    void TestTrackerClassBase::PrintConfiguration(std::ostream&) const {}
    
    void TestTrackerClassBase::RecordRequest(std::string test_name, bool bRun)
      { (bRun? run: skipped).insert(test_name); }
    
    bool TestTrackerClassBase::Query(std::string test_name) {
      bool bRun = ShouldRun(test_name);
      RecordRequest(test_name, bRun);
      return bRun;
    }
    
    /// Adds a vector of tests into a test set
    void TestTrackerClassBase::CopyList
      (TestList_t& dest, std::vector<std::string> const& from)
      { std::copy(from.begin(), from.end(), std::inserter(dest, dest.end())); }
    
    
    /// Asks to run all the tests
    class PassAllTestTrackerClass: public TestTrackerClassBase {
        public:
      
      /// Returns whether the specified test should run
      virtual bool ShouldRun(std::string test_name) const override
        { return true; }
      
      // everything always runs already
      virtual void PleaseRunAlso(std::string /* test_name */) override {}
      
    }; // class PassAllTestTrackerClass
    
    /// Asks to skip tests in a list
    class BlackListTestTrackerClass: public TestTrackerClassBase {
        public:
      using TestList_t = TestTrackerClassBase::TestList_t;
      
      //@{
      /// Constructor: takes the list of tests to be skipped
      BlackListTestTrackerClass(TestList_t skip_these):
        to_be_skipped(skip_these) {}
      BlackListTestTrackerClass(std::vector<std::string> const& skip_these):
        to_be_skipped()
        { CopyList(to_be_skipped, skip_these); }
      //@}
      
      /// Returns whether the specified test should run
      virtual bool ShouldRun(std::string test_name) const override
        { return to_be_skipped.count(test_name) == 0; }
      
      // everything always runs already
      virtual void PleaseRunAlso(std::string test_name) override
        { to_be_skipped.erase(test_name); }
      
      virtual bool CheckQueriesRegistry() const override
        {
          TestList_t not_registered, queried = QueriedTests();
          std::set_difference(
            to_be_skipped.cbegin(), to_be_skipped.cend(),
            queried.cbegin(), queried.cend(),
            std::inserter(not_registered, not_registered.end())
            );
          if (!not_registered.empty()) {
            auto iTest = not_registered.cbegin(), tend = not_registered.cend();
            mf::LogError error("GeometryTestAlg");
            error
              << "The configuration presents " << not_registered.size()
              << " tests that are not supported: " << *iTest;
            while (++iTest != tend) error << ", " << *iTest;
            return false;
          }
          return true;
        } // CheckQueriesRegistry()
      
      /// Prints information about the configuration of the filter
      virtual void PrintConfiguration(std::ostream& out) const override
        {
          auto iTest = to_be_skipped.cbegin(), tend = to_be_skipped.cend();
          if (iTest == tend) {
            out << "Will skip no tests.";
            return;
          }
          out << "Will skip " << to_be_skipped.size() << " tests: " << *iTest;
          while (++iTest != tend) out << ", " << *iTest;
        } // PrintConfiguration()
      
        protected:
      TestList_t to_be_skipped; ///< tests that should be skipped
      
    }; // class BlackListTestTrackerClass
    
    /// Asks to run only tests in a list
    class WhiteListTestTrackerClass: public TestTrackerClassBase {
        public:
      using TestList_t = TestTrackerClassBase::TestList_t;
      
      //@{
      /// Constructor: takes the list of tests to be skipped
      WhiteListTestTrackerClass(TestList_t run_these): to_be_run(run_these) {}
      WhiteListTestTrackerClass(std::vector<std::string> const& run_these):
        to_be_run()
        { CopyList(to_be_run, run_these); }
      //@}
      
      /// Returns whether the specified test should run
      virtual bool ShouldRun(std::string test_name) const override
        { return to_be_run.count(test_name) > 0; }
      
      // everything always runs already
      virtual void PleaseRunAlso(std::string test_name) override
        { to_be_run.insert(test_name); }
      
      virtual bool CheckQueriesRegistry() const override
        {
          TestList_t not_registered, queried = QueriedTests();
          std::set_difference(
            to_be_run.cbegin(), to_be_run.cend(),
            queried.cbegin(), queried.cend(),
            std::inserter(not_registered, not_registered.end())
            );
          if (!not_registered.empty()) {
            auto iTest = not_registered.cbegin(), tend = not_registered.cend();
            mf::LogError error("GeometryTestAlg");
            error
              << "The configuration presents " << not_registered.size()
              << " tests that are not supported: " << *iTest;
            while (++iTest != tend) error << ", " << *iTest;
            return false;
          }
          return true;
        } // CheckQueriesRegistry()
      
      /// Prints information about the configuration of the filter
      virtual void PrintConfiguration(std::ostream& out) const override
        {
          auto iTest = to_be_run.cbegin(), tend = to_be_run.cend();
          if (iTest == tend) {
            out << "Will run no tests.";
            return;
          }
          out << "Will run only " << to_be_run.size() << " tests: " << *iTest;
          while (++iTest != tend) out << ", " << *iTest;
        } // PrintConfiguration()
      
        protected:
      TestList_t to_be_run; ///< tests that should be run
      
    }; // class WhiteListTestTrackerClass
    
  } // namespace details
  
  
  
  //......................................................................
  GeometryTestAlg::GeometryTestAlg(fhicl::ParameterSet const& pset) 
    : geom(nullptr)
    , fDisableValidWireIDcheck( pset.get<bool>("DisableWireBoundaryCheck", false) )
    , fExpectedWirePitches( pset.get<std::vector<double>>("ExpectedWirePitches", {}) )
    , fExpectedPlanePitches( pset.get<std::vector<double>>("ExpectedPlanePitches", {}) )
  {
    // initialize the list of non-fatal exceptions
    std::vector<std::string> NonFatalErrors(pset.get<std::vector<std::string>>
      ("ForgiveExceptions", std::vector<std::string>()));
    std::copy(NonFatalErrors.begin(), NonFatalErrors.end(),
      std::inserter(fNonFatalExceptions, fNonFatalExceptions.end()));
    
    // initialize the list of tests to be run
    std::vector<std::string> RunTests(pset.get<std::vector<std::string>>
      ("RunTests", std::vector<std::string>()));
    std::vector<std::string> SkipTests(pset.get<std::vector<std::string>>
      ("SkipTests", std::vector<std::string>()));
    if (!RunTests.empty() && !SkipTests.empty()) {
      throw cet::exception("GeometryTestAlg") << "Configuration error: "
        "'RunTests' and 'SkipTests' can't be specified together.\n";
    }
    
    if (!RunTests.empty())
      fRunTests.reset(new details::WhiteListTestTrackerClass(RunTests));
    else if (!SkipTests.empty())
      fRunTests.reset(new details::BlackListTestTrackerClass(SkipTests));
    else
      fRunTests.reset(new details::PassAllTestTrackerClass());
    
    if (pset.get<bool>("CheckForOverlaps", false))
      fRunTests->PleaseRunAlso("CheckOverlaps");
    
    if (pset.get<bool>("PrintWires", false))
      fRunTests->PleaseRunAlso("PrintWires");
    
    std::ostringstream sstr;
    fRunTests->PrintConfiguration(sstr);
    mf::LogInfo("GeometryTestAlg") << sstr.str();
    
  } // GeometryTestAlg::GeometryTestAlg()

  //......................................................................
  unsigned int GeometryTestAlg::Run()
  {
    
    if (!geom) {
      throw cet::exception("GeometryTestAlg")
        << "GeometryTestAlg not configured: no valid geometry provided.\n";
    }
    
    unsigned int nErrors = 0; // currently unused
    
    // change the printed version number when changing the "GeometryTest" output
    mf::LogVerbatim("GeometryTest") << "GeometryTest version 1.0";
    
    mf::LogInfo("GeometryTestInfo")
      << "Running on detector: '" << geom->DetectorName() << "'";
    
    try{
      geo::WireGeo const& testWire = geom->Wire(geo::WireID(0, 0, 1, 10));
      mf::LogVerbatim("GeometryTest")
        <<   "Wire Rmax  "         << testWire.RMax()
        << "\nWire length "        << 2.*testWire.HalfL()
        << "\nWire Rmin  "         << testWire.RMin()
        << "\nTotal mass "         << geom->TotalMass()
        << "\nNumber of views "    << geom->Nviews()
        << "\nNumber of channels " << geom->Nchannels()
        << "\nMaximum number of:"
        << "\n  TPC in a cryostat: " << geom->MaxTPCs()
        << "\n  planes in a TPC:   " << geom->MaxPlanes()
        << "\n  wires in a plane:  " << geom->MaxWires()
        ;

      //LOG_DEBUG("GeometryTest") << "print channel information ...";
      //printChannelSummary();
      //LOG_DEBUG("GeometryTest") << "done printing.";
      //mf::LogVerbatim("GeometryTest") << "print Cryo/TPC boundaries in world coordinates ...";
      //printVolBounds();
      //mf::LogVerbatim("GeometryTest") << "done printing.";
      //mf::LogVerbatim("GeometryTest") << "print Cryo/TPC dimensions ...";
      //printDetDim();
      //mf::LogVerbatim("GeometryTest") << "done printing.";
      //mf::LogVerbatim("GeometryTest") << "print wire center positions in world coordinates ...";
      //printWirePos();
      //mf::LogVerbatim("GeometryTest") << "done printing.";

      if (shouldRunTests("CheckOverlaps")) {
        LOG_INFO("GeometryTest") << "test for overlaps ...";
        gGeoManager->CheckOverlaps(1e-5);
        gGeoManager->PrintOverlaps();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Cryostat")) {
        LOG_INFO("GeometryTest") << "test Cryostat methods ...";
        testCryostat();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("ChannelToWire")) {
        LOG_INFO("GeometryTest") << "test channel to plane wire and back ...";
        testChannelToWire();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("FindPlaneCenters")) {
        LOG_INFO("GeometryTest") << "test find plane centers...";
        testFindPlaneCenters();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("WireCoordAngle")) {
        LOG_INFO("GeometryTest") << "testWireCoordAngle...";
        testWireCoordAngle();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Projection")) {
        LOG_INFO("GeometryTest") << "testProject...";
        testProject();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("WirePos")) {
        LOG_INFO("GeometryTest") << "testWirePos...";
        // There is a contradiction here, and these must be tested differently
        // Testing based on detector ID should NOT become common practice
        LOG_INFO("GeometryTest") << "disabled.";
      }

      if (shouldRunTests("NearestWire")) {
        LOG_INFO("GeometryTest") << "testNearestWire...";
        testNearestWire();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("WireIntersection")) {
        LOG_INFO("GeometryTest") << "testWireIntersection...";
        testWireIntersection();
        LOG_INFO("GeometryTest") << "testWireIntersection complete";
      }

      if (shouldRunTests("ThirdPlane")) {
        LOG_INFO("GeometryTest") << "testThirdPlane...";
        testThirdPlane();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("ThirdPlaneSlope")) {
        LOG_INFO("GeometryTest") << "testThirdPlaneSlope...";
        testThirdPlane_dTdW();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("WirePitch")) {
        LOG_INFO("GeometryTest") << "testWirePitch...";
        testWirePitch();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("PlanePitch")) {
        LOG_INFO("GeometryTest") << "testPlanePitch...";
        testPlanePitch();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Stepping")) {
        LOG_INFO("GeometryTest") << "testStepping...";
        testStepping();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("PrintWires")) {
        LOG_INFO("GeometryTest") << "printAllGeometry...";
        printAllGeometry();
        LOG_INFO("GeometryTest") << "complete.";
      }
    }
    catch (cet::exception &e) {
      mf::LogWarning("GeometryTest") << "exception caught: \n" << e;
      if (fNonFatalExceptions.count(e.category()) == 0) throw;
    }
    
    if (!fRunTests->CheckQueriesRegistry()) {
      throw cet::exception("GeometryTest")
        << "(postumous) configuration error detected!\n";
    }
    
    mf::LogInfo log("GeometryTest");
    log << "Tests completed:";
    auto const& tests_run = fRunTests->RunTests();
    if (tests_run.empty()) {
      log << "\n  no test run";
    }
    else {
      log << "\n  " << tests_run.size() << " tests run:\t ";
      for (std::string const& test_name: tests_run) log << " " << test_name;
    }
    auto const& tests_skipped = fRunTests->SkippedTests();
    if (!tests_skipped.empty()) {
      log << "\n  " << tests_skipped.size() << " tests skipped:\t ";
      for (std::string const& test_name: tests_skipped) log << " " << test_name;
    }

    return nErrors;
  } // GeometryTestAlg::Run()



  //......................................................................
  void GeometryTestAlg::printChannelSummary()
  {
    static unsigned int OneSeg = 0;
    static unsigned int TwoSegs = 0;
    static unsigned int ThreeSegs = 0;
    static unsigned int FourSegs = 0;
    uint32_t channels = geom->Nchannels();
    if(geom->NTPC() > 1) channels /= geom->NTPC()/2;

    for(uint32_t c = 0; c < channels; c++){

      unsigned int ChanSize = geom->ChannelToWire(c).size();

       if     (ChanSize==1) ++OneSeg;
       else if(ChanSize==2) ++TwoSegs;
       else if(ChanSize==3) ++ThreeSegs;
       else if(ChanSize==4) ++FourSegs;

    }

     mf::LogVerbatim("GeometryTest") << "OneSeg: "       << OneSeg 
				     << ",  TwoSegs: "   << TwoSegs
				     << ",  ThreeSegs: " << ThreeSegs
				     << ",  FourSegs: "  << FourSegs;

  }

  //......................................................................
  void GeometryTestAlg::printVolBounds()
  {
      double origin[3] = {0.};
      double world[3] = {0.};
      for(unsigned int c = 0; c < geom->Ncryostats(); ++c){
	geom->Cryostat(c).LocalToWorld(origin, world);

        mf::LogVerbatim("GeometryTest") << "Cryo " << c;
	mf::LogVerbatim("GeometryTest") << "    -x: " << world[0] - geom->Cryostat(c).HalfWidth();
	mf::LogVerbatim("GeometryTest") << "    +x: " << world[0] + geom->Cryostat(c).HalfWidth();
	mf::LogVerbatim("GeometryTest") << "    -y: " << world[1] - geom->Cryostat(c).HalfHeight();
	mf::LogVerbatim("GeometryTest") << "    +y: " << world[1] + geom->Cryostat(c).HalfHeight();
	mf::LogVerbatim("GeometryTest") << "    -z: " << world[2] - geom->Cryostat(c).Length()/2;
	mf::LogVerbatim("GeometryTest") << "    +z: " << world[2] + geom->Cryostat(c).Length()/2;

        for(unsigned int t = 0; t < geom->NTPC(c); ++t){
          geom->Cryostat(c).TPC(t).LocalToWorld(origin, world);

          mf::LogVerbatim("GeometryTest") << "  TPC " << t;
          mf::LogVerbatim("GeometryTest") << "    -x: " << world[0] - geom->Cryostat(c).TPC(t).HalfWidth();
          mf::LogVerbatim("GeometryTest") << "    +x: " << world[0] + geom->Cryostat(c).TPC(t).HalfWidth();
          mf::LogVerbatim("GeometryTest") << "    -y: " << world[1] - geom->Cryostat(c).TPC(t).HalfHeight();
          mf::LogVerbatim("GeometryTest") << "    +y: " << world[1] + geom->Cryostat(c).TPC(t).HalfHeight();
          mf::LogVerbatim("GeometryTest") << "    -z: " << world[2] - geom->Cryostat(c).TPC(t).Length()/2;
          mf::LogVerbatim("GeometryTest") << "    +z: " << world[2] + geom->Cryostat(c).TPC(t).Length()/2;
        }
      }

  }



  //......................................................................
  // great sanity check for geometry, only call in analyze when debugging
  void GeometryTestAlg::printDetDim()
  {
    for(unsigned int c = 0; c < geom->Ncryostats(); ++c){

      mf::LogVerbatim("GeometryTest") << "Cryo " << c;
      mf::LogVerbatim("GeometryTest") << "    width: "
				      << geom->CryostatHalfWidth(c);
      mf::LogVerbatim("GeometryTest") << "    height: "
				      << geom->CryostatHalfHeight(c);
      mf::LogVerbatim("GeometryTest") << "    length: "
				      << geom->CryostatLength(c);

      mf::LogVerbatim("GeometryTest") << "  TPC 0";
      mf::LogVerbatim("GeometryTest") << "    width: "
				      << geom->DetHalfWidth(0,c);
      mf::LogVerbatim("GeometryTest") << "    height: "
				      << geom->DetHalfHeight(0,c);
      mf::LogVerbatim("GeometryTest") << "    length: "
				      << geom->DetLength(0,c);      
    }
  }

  //......................................................................
  // great sanity check for volume sorting, only call in analyze when debugging
  void GeometryTestAlg::printWirePos()
  {
    unsigned int cs = 0;

    for(unsigned int t=0; t<std::floor(geom->NTPC()/12)+1; ++t){
      for(unsigned int p=0; p<3; ++p){
        for(unsigned int w=0; w<geom->Cryostat(0).TPC(t).Plane(p).Nwires(); w++){
        
          double xyz[3] = {0.};
          geom->Cryostat(0).TPC(t).Plane(p).Wire(w).GetCenter(xyz);

          std::cout << "WireID (" << cs << ", " << t << ", " << p << ", " << w
                    << "):  x = " << xyz[0] 
                    << ", y = " << xyz[1]
                    << ", z = " << xyz[2] << std::endl;
        }
      }
    }
  }

  //......................................................................
  // great insanity: print all wires in a TPC
  void GeometryTestAlg::printWiresInTPC
    (const geo::TPCGeo& tpc, std::string indent /* = "" */) const
  {
    const unsigned int nPlanes = tpc.Nplanes();
    const double Origin[3] = { 0., 0., 0. };
    double TPCpos[3];
    tpc.LocalToWorld(Origin, TPCpos);
    mf::LogVerbatim("GeometryTest") << indent << "TPC at ("
      << TPCpos[0] << ", " << TPCpos[1] << ", " << TPCpos[2]
      << ") cm has " << nPlanes << " wire planes (max wires: " << tpc.MaxWires()
      << "):";
    
    for(unsigned int p = 0; p < nPlanes; ++p) {
      const geo::PlaneGeo& plane = tpc.Plane(p);
      const unsigned int nWires = plane.Nwires();
      double PlanePos[3];
      plane.LocalToWorld(Origin, PlanePos);
      std::string coord, orientation;
      switch (plane.View()) {
        case geo::kU:       coord = "U direction"; break;
        case geo::kV:       coord = "V direction"; break;
        case geo::kZ:       coord = "Z direction"; break;
        case geo::k3D:      coord = "3D coordinate"; break;
        case geo::kUnknown: coord = "an unknown direction"; break;
        default:            coord = "unexpected direction"; break;
      } // switch
      switch (plane.Orientation()) {
        case geo::kHorizontal: orientation = "horizontal"; break;
        case geo::kVertical:   orientation = "vertical"; break;
        default:               orientation = "unexpected"; break;
      }
      
      // get the area spanned by the wires
      simple_geo::Area plane_area = simple_geo::PlaneCoverage(plane);
      
      mf::LogVerbatim("GeometryTest") << indent << "  plane #" << p << " at ("
        << PlanePos[0] << ", " << PlanePos[1] << ", " << PlanePos[2]
        << ") cm, covers " << plane_area.DeltaY() << " x "
        << plane_area.DeltaZ() << " cm around " << plane_area.Center()
        << ", has " << orientation << " orientation and "
        << nWires << " wires measuring " << coord << " with a pitch of "
        << plane.WirePitch() << " mm:";
      for(unsigned int w = 0;  w < nWires; ++w) {
        const geo::WireGeo& wire = plane.Wire(w);
        double xyz[3] = { 0. };
        wire.LocalToWorld(xyz, xyz); // LocalToWorld() supports in place transf.
        double WireS[3],  WireM[3], WireE[3]; // start, middle point and end
        
        // the wire should be aligned on z axis, half on each side of 0,
        // in its local frame
        wire.GetStart(WireS);
        wire.GetCenter(WireM);
        wire.GetEnd(WireE);
        mf::LogVerbatim("GeometryTest") << indent
          << "    wire #" << w
          << " at (" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")"
          << "\n" << indent << "       start at (" << WireS[0] << ", " << WireS[1] << ", " << WireS[2] << ")"
          << "\n" << indent << "      middle at (" << WireM[0] << ", " << WireM[1] << ", " << WireM[2] << ")"
          << "\n" << indent << "         end at (" << WireE[0] << ", " << WireE[1] << ", " << WireE[2] << ")"
          ;
      } // for wire
    } // for plane
  } // GeometryTestAlg::printWiresInTPC()

  
  void GeometryTestAlg::printAllGeometry() const {
    const unsigned int nCryostats = geom->Ncryostats();
    const double Origin[3] = { 0., 0., 0. };
    mf::LogVerbatim("GeometryTest") << "Detector " << geom->DetectorName()
      << " has " << nCryostats << " cryostats:";
    for(unsigned int c = 0; c < nCryostats; ++c) {
      const geo::CryostatGeo& cryostat = geom->Cryostat(c);
      const unsigned int nTPCs = cryostat.NTPC();
      double CryoPos[3];
      cryostat.LocalToWorld(Origin, CryoPos);
      mf::LogVerbatim("GeometryTest") << "  cryostat #" << c << " at ("
				      << CryoPos[0] << ", " << CryoPos[1] << ", " << CryoPos[2] << ") cm has "
				      << nTPCs << " TPC(s):";
      for(unsigned int t = 0;  t < nTPCs; ++t) {
        const geo::TPCGeo& tpc = cryostat.TPC(t);
        if (nTPCs > 1) mf::LogVerbatim("GeometryTest") << "    TPC #" << t;
        printWiresInTPC(tpc, "    ");
      } // for TPC
    } // for cryostat
    mf::LogVerbatim("GeometryTest") << "End of detector "
				    << geom->DetectorName() << " geometry.";
  } // GeometryTestAlg::printAllGeometry()

  //......................................................................
  void GeometryTestAlg::testCryostat()
  {
    mf::LogVerbatim("GeometryTest") << "\tThere are " << geom->Ncryostats() << " cryostats in the detector";

    for(unsigned int c = 0; c < geom->Ncryostats(); ++c){

      mf::LogVerbatim("GeometryTest") << "\n\t\tCryostat " << c 
				      << " " << geom->Cryostat(c).Volume()->GetName()
				      << " Dimensions: " << 2.*geom->Cryostat(c).HalfWidth()
				      << " x "           << 2.*geom->Cryostat(c).HalfHeight() 
				      << " x "           << geom->Cryostat(c).Length()
				      << "\n\t\t mass: " << geom->Cryostat(c).Mass();

      double cryobound[6] = {0.};
      geom->CryostatBoundaries(cryobound, c);
      mf::LogVerbatim("GeometryTest") << "Cryostat boundaries are at:\n"
				      << "\t-x:" << cryobound[0] << " +x:" << cryobound[1]
				      << "\t-y:" << cryobound[2] << " +y:" << cryobound[3]
				      << "\t-z:" << cryobound[4] << " +z:" << cryobound[5];

      // pick a position in the middle of the cryostat in the world coordinates
      double worldLoc[3] = {0.5*(cryobound[1] - cryobound[0]) + cryobound[0],
			    0.5*(cryobound[3] - cryobound[2]) + cryobound[2],
			    0.5*(cryobound[5] - cryobound[4]) + cryobound[4]};
		
      LOG_DEBUG("GeometryTest") << "\t testing GeometryCore::PoitionToCryostat....";
      try{
	unsigned int cstat = 0;
	geom->PositionToCryostat(worldLoc, cstat);
      }
      catch(cet::exception &e){
	mf::LogWarning("FailedToLocateCryostat") << "\n exception caught:" << e;
	if (fNonFatalExceptions.count(e.category()) == 0) throw;
      }
      LOG_DEBUG("GeometryTest") << "done";

      LOG_DEBUG("GeometryTest") << "\t Now test the TPCs associated with this cryostat";
      this->testTPC(c);
    }

    return;
  }

  //......................................................................
  void GeometryTestAlg::testTPC(unsigned int const& c)
  {
    geo::CryostatGeo const& cryo = geom->Cryostat(c);

    mf::LogVerbatim("GeometryTest") << "\tThere are " << cryo.NTPC() 
                                    << " TPCs in the detector";
    
    for(size_t t = 0; t < cryo.NTPC(); ++t){
      geo::TPCGeo const& tpc = cryo.TPC(t);
      
      // figure out the TPC coordinates
      
      std::array<double, 3> TPClocalTemp, TPCstart, TPCstop;
      TPClocalTemp[0] = -tpc.HalfWidth(); // x
      TPClocalTemp[1] = -tpc.HalfHeight(); // y
      TPClocalTemp[2] = -tpc.Length() / 2.; // z
      tpc.LocalToWorld(TPClocalTemp.data(), TPCstart.data());
      for (size_t i = 0; i < TPClocalTemp.size(); ++i) TPClocalTemp[i] = -TPClocalTemp[i];
      tpc.LocalToWorld(TPClocalTemp.data(), TPCstop.data());
      
      mf::LogVerbatim("GeometryTest")
        << "\n\t\tTPC " << t 
          << " " << geom->GetLArTPCVolumeName(t, c) 
          << " has " << tpc.Nplanes() << " planes."
        << "\n\t\tTPC location: ( "
          << TPCstart[0] << " ; " << TPCstart[1] << " ; "<< TPCstart[2]
          << " ) =>  ( "
          << TPCstop[0] << " ; " << TPCstop[1] << " ; "<< TPCstop[2]
          << " ) [cm]"
        << "\n\t\tTPC Dimensions: "
          << 2.*tpc.HalfWidth() << " x " << 2.*tpc.HalfHeight() << " x " << tpc.Length()
        << "\n\t\tTPC Active Dimensions: " 
          << 2.*tpc.ActiveHalfWidth() << " x " << 2.*tpc.ActiveHalfHeight() << " x " << tpc.ActiveLength()
        << "\n\t\tTPC mass: " << tpc.ActiveMass()
        << "\n\t\tTPC drift distance: " << tpc.DriftDistance();
      
      for(size_t p = 0; p < tpc.Nplanes(); ++p) {
        geo::PlaneGeo const& plane = tpc.Plane(p);
        mf::LogVerbatim("GeometryTest")
          << "\t\tPlane " << p << " has " << plane.Nwires() 
            << " wires and is at (x,y,z) = (" 
            << tpc.PlaneLocation(p)[0] << "," 
            << tpc.PlaneLocation(p)[1] << "," 
            << tpc.PlaneLocation(p)[2] 
            << ");"
          << "\n\t\t\tpitch from plane 0 is " << tpc.Plane0Pitch(p) << ";"
          << "\n\t\t\tOrientation " << plane.Orientation()
            << ", View " << ViewName(plane.View())
          << "\n\t\t\tWire angle " << plane.Wire(0).ThetaZ()
            << ", Wire coord. angle " << plane.PhiZ()
            << ", Pitch " << plane.WirePitch();
      } // for plane
      geo::DriftDirection_t dir = tpc.DriftDirection();
      if     (dir == geo::kNegX) 
        mf::LogVerbatim("GeometryTest") << "\t\tdrift direction is towards negative x values";
      else if(dir == geo::kPosX) 
        mf::LogVerbatim("GeometryTest") << "\t\tdrift direction is towards positive x values";
      else{
        throw cet::exception("UnknownDriftDirection") << "\t\tdrift direction is unknown\n";
      }

      LOG_DEBUG("GeometryTest") << "\t testing PositionToTPC...";
      // pick a position in the middle of the cryostat in the world coordinates
      double worldLoc[3] = {0.};
      double localLoc[3] = {0.};
      tpc.LocalToWorld(localLoc, worldLoc);

      const unsigned int tpcNo = cryo.FindTPCAtPosition(worldLoc, 1+1.e-4);

      if(tpcNo != t)
        throw cet::exception("BadTPCLookupFromPosition") << "TPC look up returned tpc = "
                                                         << tpcNo << " should be " << t << "\n";

      LOG_DEBUG("GeometryTest") << "done.";
    } // for TPC
    
    return;
  }


  //......................................................................
  void GeometryTestAlg::testWireCoordAngle() const {
    /*
     * Tests that the angle PhiZ() actually points to the next wire.
     *
     * The test, for each plane, performs the following:
     * - pick the middle wire, verify that we can get the expected wire
     *   coordinate for its centre
     * - move one wire pitch away from the centre in the direction determined
     *   by PhiZ(), verify that the coordinate increases by 1
     */
    
    for (geo::PlaneID const& planeid: geom->IteratePlaneIDs()) {
      
      geo::PlaneGeo const& plane = geom->Plane(planeid);
      
      // define the wires to work with
      const unsigned int nWires = plane.Nwires();
      
      geo::WireID middle_wire_id(planeid, nWires / 2);
      geo::WireID next_wire_id(planeid, nWires / 2 + 1);
      
      if (next_wire_id.Wire >= nWires) {
        throw cet::exception("WeirdGeometry")
          << "Plane " << std::string(planeid) << " has only " << nWires
          << " wires?!?\n";
      }
      
      
      geo::WireGeo const& middle_wire = geom->Wire(middle_wire_id);
      std::array<double, 3> middle_wire_center;
      middle_wire.GetCenter(middle_wire_center.data());
      LOG_TRACE("GeometryTest")
        << "Center of " << std::string(middle_wire_id) << " at ("
          << middle_wire_center[0]
          << "; " << middle_wire_center[1] << "; " << middle_wire_center[2]
          << ")";
      
      // cross check: we can find the middle wire
      const double middle_coord = geom->WireCoordinate
        (middle_wire_center[1], middle_wire_center[2], planeid);
      
      if (std::abs(middle_coord - double(middle_wire_id.Wire)) > 1e-3) {
        throw cet::exception("WireCoordAngle")
          << "Center of " << std::string(middle_wire_id) << " at ("
          << middle_wire_center[0]
          << "; " << middle_wire_center[1] << "; " << middle_wire_center[2]
          << ") has wire coordinate " << middle_coord
          << " (" << middle_wire_id.Wire << " expected)\n";
      } // if
      
      // the check: this coordinate should lie on the next wire
      std::array<double, 3> on_next_wire = middle_wire_center;
      const double pitch = plane.WirePitch();
      
      LOG_TRACE("GeometryTest")
        << "  pitch: " << pitch << " cos(phi_z): " << plane.CosPhiZ()
        << "  sin(phi_z): " << plane.SinPhiZ();
    
    /*
      // this would test GeometryCore::GetIncreasingWireDirection()
      TVector3 wire_coord_dir = plane.GetIncreasingWireDirection();
      on_next_wire[0] += wire_coord_dir.X();
      on_next_wire[1] += pitch * wire_coord_dir.Y();
      on_next_wire[2] += pitch * wire_coord_dir.Z();
    */
      on_next_wire[1] += pitch * plane.SinPhiZ();
      on_next_wire[2] += pitch * plane.CosPhiZ();
      
      const double next_coord
        = geom->WireCoordinate(on_next_wire[1], on_next_wire[2], planeid);
      
      if (std::abs(next_coord - double(next_wire_id.Wire)) > 1e-3) {
        throw cet::exception("WireCoordAngle")
          << "Position (" << on_next_wire[0]
          << "; " << on_next_wire[1] << "; " << on_next_wire[2]
          << ") is expected to be on wire " << std::string(next_wire_id)
          << " but it has wire coordinate " << next_coord << "\n";
      } // if
      
    } // for
    
  } // GeometryTestAlg::testWireCoordAngle()
  
  
  //......................................................................
  void GeometryTestAlg::testChannelToWire()
  {

    for(unsigned int cs = 0; cs < geom->Ncryostats(); ++cs){
      for(unsigned int tpc = 0; tpc < geom->Cryostat(cs).NTPC(); ++tpc){
	for(unsigned int plane = 0; plane < geom->Cryostat(cs).TPC(tpc).Nplanes(); ++plane){
	  for(unsigned int wire = 0; wire < geom->Cryostat(cs).TPC(tpc).Plane(plane).Nwires(); ++wire){

	    uint32_t channel = geom->PlaneWireToChannel(plane, wire, tpc, cs);
	    //std::cout << "WireID (" << cs << ", " << tpc << ", " << plane << ", " << wire 
	    //	<< ") --> Channel " << channel << std::endl;    
	    std::vector< geo::WireID > wireIDs = geom->ChannelToWire(channel);
	    

	    if ( wireIDs.size() == 0 ) 
	      throw cet::exception("BadChannelLookup") << "requested channel: " << channel 
						       << ";" << cs << "," << tpc
						       << "," << plane << "," << wire << "\n"
						       << "got back an empty vector of WireID " << "\n";

	    bool goodLookup = false;
	    for( auto const& wid : wireIDs){
	      if(wid.Cryostat == cs    && 
		 wid.TPC      == tpc   && 
		 wid.Plane    == plane && 
		 wid.Wire     == wire) goodLookup = true;
	    }
	    
	    if(!goodLookup)
	    {
	      std::cout << "Returned: " << std::endl;
              for(unsigned int id=0; id<wireIDs.size(); ++id)
	      {
		std::cout << "wireIDs[" << id << "] = ("
		          << wireIDs[id].Cryostat << ", "
		          << wireIDs[id].TPC      << ", "
		          << wireIDs[id].Plane    << ", "
		          << wireIDs[id].Wire     << ")" << std::endl;
              }
	      throw cet::exception("BadChannelLookup") << "requested channel " << channel 
						       << "expected to return" << cs << "," << tpc
						       << "," << plane << "," << wire << "\n"
						       << "no returned geo::WireID structs matched\n";
            }

	    if(geom->SignalType(channel) != geom->Plane(plane, tpc, cs).SignalType() )
	      throw cet::exception("BadChannelLookup") << "expected signal type: SignalType(channel) = " 
						       << geom->SignalType(channel)
						       << " for channel " 
						       << channel << ", WireID ("  
						       << cs << ", " << tpc << ", " << plane << ", " << wire
						       << "), got: Plane(" << plane << ", " << tpc 
						                           << ", " << cs << ").SignalType() = "
						       << geom->Plane(plane, tpc, cs).SignalType() << "\n";


	    if(geom->View(channel) != geom->Plane(plane, tpc, cs).View() )
	      throw cet::exception("BadChannelLookup") << "expected view type: View(channel) = " 
						       << ViewName(geom->View(channel))
						       << " for channel " 
						       << channel << ", WireID ("  
						       << cs << ", " << tpc << ", " << plane << ", " << wire
						       << "), got: Plane(" << plane << ", " << tpc 
						                           << ", " << cs << ").View() = "
						       << ViewName(geom->Plane(plane, tpc, cs).View()) << "\n";

	  }
	}
      }
    }

    return;
  }

  //......................................................................
  void GeometryTestAlg::testFindPlaneCenters()
  {
    double xyz[3] = {0.},   xyzW[3] = {0.};
    for(size_t i = 0; i < geom->Nplanes(); ++i){ 
      geom->Plane(i).LocalToWorld(xyz,xyzW);
      mf::LogVerbatim("GeometryTest") << "\n\tplane " 
				      << i << " is centered at (x,y,z) = (" 
				      << xyzW[0] << "," << xyzW[1]
				      << "," << xyzW[2] << ")";
    } 
  } 

  //......................................................................
  void GeometryTestAlg::testStandardWirePos() 
  {
    double xyz[3] = {0.};
    double xyzprev[3] = {0.};
    for(size_t cs = 0; cs < geom->Ncryostats(); ++cs){
      for(size_t t = 0; t < geom->Cryostat(cs).NTPC(); ++t){
	const geo::TPCGeo* tpc = &geom->Cryostat(cs).TPC(t); 

	for (size_t i=0; i < tpc->Nplanes(); ++i) {
	  const geo::PlaneGeo* plane = &tpc->Plane(i);

	  for (size_t j = 1; j < plane->Nwires(); ++j) {

	    const geo::WireGeo wire = plane->Wire(j);
	    const geo::WireGeo wireprev = plane->Wire(j-1);

	    wire.GetCenter(xyz);
	    wireprev.GetCenter(xyzprev);

	    // wires increase in +z order
	    if(xyz[2] < xyzprev[2])
	      throw cet::exception("WireOrderProblem") 	<< "\n\twires do not increase in +z order in"
							<< "Cryostat " << cs
							<< ", TPC " << t
							<< ", Plane " << i
							<< ";  at wire " << j << "\n";

	  }// end loop over wires
	}// end loop over planes
      }// end loop over tpcs
    }// end loop over cryostats

}

  //......................................................................
  void GeometryTestAlg::testAPAWirePos() 
  {
    double origin[3] = {0.};
    double tpcworld[3] = {0.};
    double xyz[3] = {0.};
    double xyzprev[3] = {0.};
    for(size_t cs = 0; cs < geom->Ncryostats(); ++cs){
      for(size_t t = 0; t < geom->Cryostat(cs).NTPC(); ++t){
	const geo::TPCGeo* tpc = &geom->Cryostat(cs).TPC(t);
	tpc->LocalToWorld(origin, tpcworld);

	for (size_t i=0; i < tpc->Nplanes(); ++i) {
	  const geo::PlaneGeo* plane = &tpc->Plane(i);

	  for (size_t j = 1; j < plane->Nwires(); ++j) {
	    const geo::WireGeo wire = plane->Wire(j);
	    const geo::WireGeo wireprev = plane->Wire(j-1);

	    wire.GetCenter(xyz);
	    wireprev.GetCenter(xyzprev);

            // top TPC wires increase in -y
	    if(tpcworld[1] > 0 && xyz[1] > xyzprev[1])
	      throw cet::exception("WireOrderProblem") 	<< "\n\ttop TPC wires do not increase in -y order in"
							<< "Cryostat " << cs
							<< ", TPC " << t
							<< ", Plane " << i
							<< ";  at wire " << j << "\n";
            // bottom TPC wires increase in +y
	    else if(tpcworld[1] < 0 && xyz[1] < xyzprev[1])
	      throw cet::exception("WireOrderProblem") 	<< "\n\tbottom TPC wires do not increase in +y order in"
                                                        << "Cryostat " << cs
							<< ", TPC " << t
                                                        << ", Plane " << i 
                                                        << ";  at wire " << j << "\n";
	  }// end loop over wires
	}// end loop over planes
      }// end loop over tpcs
    }// end loop over cryostats

  }
  
  
  //......................................................................
  inline std::array<double, 3> GeometryTestAlg::GetIncreasingWireDirection
    (const geo::PlaneGeo& plane)
  {
    TVector3 IncreasingWireDir = plane.GetIncreasingWireDirection();
    return
      { IncreasingWireDir.X(), IncreasingWireDir.Y(), IncreasingWireDir.Z() };
  } // GeometryTestAlg::GetIncreasingWireDirection()
  
  
  //......................................................................
  void GeometryTestAlg::testNearestWire()
  {
    // Even if you comment it out, please leave the TStopWatch code
    // in this code for additional testing. The NearestChannel routine
    // is the most frequently called in the simulation, so its execution time
    // is an important component of LArSoft's speed.
    TStopwatch stopWatch;
    stopWatch.Start();

    bool bTestWireCoordinate = true;
    
    // get a wire and find its center
    geo::GeometryCore::plane_id_iterator iPlane(&*geom);
    while (iPlane) {
      unsigned int cs = iPlane->Cryostat;
      unsigned int t = iPlane->TPC;
      unsigned int p = iPlane->Plane;
      
      const geo::PlaneGeo& plane = *(iPlane.get());
      const unsigned int NWires = plane.Nwires();
      
      const std::array<double, 3> IncreasingWireDir
        = GetIncreasingWireDirection(plane);
      
      LOG_DEBUG("GeoTestWireCoordinate")
        << "The direction of increasing wires for plane C=" << cs << " T=" << t
        << " P=" << p << " (theta=" << plane.Wire(0).ThetaZ() << " pitch="
        << plane.WirePitch() << " orientation="
        << (plane.Orientation() == geo::kHorizontal? "H": "V")
        << (plane.WireIDincreasesWithZ()? "+": "-")
        << ") is ( " << IncreasingWireDir[0] << " ; "
        << IncreasingWireDir[1] << " ; " << IncreasingWireDir[2] << ")";
      
      for (unsigned int w = 0; w < NWires; ++w) {
        
        geo::WireID wireID(*iPlane, w);
        
        const geo::WireGeo& wire = plane.Wire(w);
        const double pos[3] = {0., 0.0, 0.};
        std::array<double, 3> wire_center;
        wire.LocalToWorld(pos, wire_center.data());
        
        uint32_t nearest = 0;
        std::vector< geo::WireID > wireIDs;
        
        try{
          // The double[] version tested here falls back on the
          // TVector3 version, so this test both.
          nearest = geom->NearestChannel(wire_center.data(), p, t, cs);
          
          // We also want to test the std::vector<duoble> version
          std::array<double, 3> posWorldV;
          for (int i=0; i<3; ++i) {
            posWorldV[i] = wire_center[i] + 0.001;
          }
          nearest = geom->NearestChannel(posWorldV.data(), p, t, cs);
        }
        catch(cet::exception &e){
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
        try{
          wireIDs = geom->ChannelToWire(nearest);
          
          if ( wireIDs.empty() ) {
            throw cet::exception("BadPositionToChannel") << "test point is at " 
                                                         << wire_center[0] << " " 
                                                         << wire_center[1] << " " 
                                                         << wire_center[2] << "\n"
                                                         << "nearest channel is " 
                                                         << nearest << " for " 
                                                         << cs << " " << t << " "
                                                         << p << " " << w << "\n";
          }
        }
        catch(cet::exception &e){
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
        if(std::find(wireIDs.begin(), wireIDs.end(), wireID) == wireIDs.end()) {
          throw cet::exception("BadPositionToChannel") << "Current WireID ("
                                                       << cs << "," << t << "," << p << "," << w << ") "
                                                       << "has a world position at "
                                                       << wire_center[0] << " " 
                                                       << wire_center[1] << " " 
                                                       << wire_center[2] << "\n"
                                                       << "NearestWire for this position is "
                                                       << geom->NearestWire(wire_center.data(),p,t,cs) << "\n"
                                                       << "NearestChannel is " 
                                                       << nearest << " for " 
                                                       << cs << " " << t << " " << p << " " << w << "\n"
                                                       << "Should be channel "
                                                       << geom->PlaneWireToChannel(p,w,t,cs);
        } // if good lookup fails
        
        
        // nearest wire, integral and floating point
        try {
          // The test consists in sampling NStep (=5) points between the current
          // wire and the previous/next, following the normal to the wire.
          // We expect WireCoordinate() to reflect the same shift.
          
          // using absolute value just in case (what happens if w1 > w2?)
          const double pitch
            = std::abs(geom->WirePitch((w > 0)? w - 1: 1, w, p, t, cs));
          
          double wire_shifted[3];
          double step[3];
          for (size_t i = 0; i < 3; ++i) step[i] = pitch * IncreasingWireDir[i];
          
          constexpr int NSteps = 5; // odd value avoids testing half-way
          for (int i = -NSteps; i <= +NSteps; ++i) {
            // we move away by this fraction of wire:
            const double f = NSteps? (double(i) / NSteps): 0.0;
            
            // these are the actual shifts on the positive directions y and z
            std::array<double, 3> delta;
            
            for (size_t i = 0; i < 3; ++i) {
              delta[i] = f * step[i];
              wire_shifted[i] = wire_center[i] + delta[i];
            } // for
            
            // we expect this wire number
            const double expected_wire = w + f;
            
            double wire_from_wc = 0;
            if (bTestWireCoordinate) {
              if (IncreasingWireDir[0] != 0.) {
                // why? because WireCoordinate() has 2D input
                LOG_ERROR("WireCoordinateNotImplemented")
                  << "The direction of increasing wires for plane "
                  << "C=" << cs << " T=" << t << " P=" << p
                  << " (theta=" << plane.Wire(0).ThetaZ() << " orientation="
                  << (plane.Orientation() == geo::kHorizontal? "H": "V")
                  << ") is ( " << IncreasingWireDir[0] << " ; "
                  << IncreasingWireDir[1] << " ; " << IncreasingWireDir[2]
                  << "), not orthogonal to x axis."
                  << " This configuration is not supported"
                  << "\n";
                bTestWireCoordinate = false;
              } // if
              try {
                wire_from_wc = geom->WireCoordinate
                  (wire_shifted[1], wire_shifted[2], p, t, cs);
              }
              catch (cet::exception& e) {
                if (hasCategory(e, "NotImplemented")) {
                  LOG_ERROR("WireCoordinateNotImplemented")
                    << "WireCoordinate() is not implemented for your ChannelMap;"
                    " skipping the test";
                  bTestWireCoordinate = false;
                }
                else if (bTestWireCoordinate) throw;
              }
            }
            if (bTestWireCoordinate) {
              if (std::abs(wire_from_wc - expected_wire) > 1e-3) {
              //  throw cet::exception("GeoTestErrorWireCoordinate")
                mf::LogError("GeoTestErrorWireCoordinate")
                  << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") shows " << wire_from_wc << ", " << expected_wire
                  << " expected.\n";
              } // if mismatch
              
            } // if testing WireCoordinate
            
            if ((expected_wire > -0.5) && (expected_wire < NWires - 0.5)) {
              const unsigned int expected_wire_number = std::round(expected_wire);
              unsigned int wire_number_from_wc;
              try {
                wire_number_from_wc = geom->NearestWire(wire_shifted, p, t, cs);
              }
              catch (cet::exception& e) {
                throw cet::exception("GeoTestErrorWireCoordinate", "", e)
              //  LOG_ERROR("GeoTestErrorWireCoordinate")
                  << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") failed NearestWire(), " << expected_wire_number
                  << " expected (more precisely, " << expected_wire << ").\n";
              }
              
              if (mf::isDebugEnabled()) {
                // In debug mode, we print a lot and we don't (fatally) complain
                std::stringstream e;
                e << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") near to " << wire_number_from_wc;
                if (wire_number_from_wc != expected_wire_number) {
                  e << ", " << expected_wire_number
                    << " expected (more precisely, " << expected_wire << ").";
                // throw e;
                  LOG_ERROR("GeoTestErrorWireCoordinate") << e.str();
                }
                else {
                  mf::LogVerbatim("GeoTestWireCoordinate") << e.str();
                }
              }
              else if (wire_number_from_wc != expected_wire_number) {
                // In production mode, we don't print anything and throw on error
                throw cet::exception("GeoTestErrorWireCoordinate")
                  << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") near to " << wire_number_from_wc
                  << ", " << expected_wire_number
                  << " expected (more precisely, " << expected_wire << ").";
              }
            } // if shifted wire not outside boundaries
            
          } // for i
          
        } // try
        catch(cet::exception &e) {
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
      } // for all wires in the plane
      ++iPlane;
    } // end loop over planes

    stopWatch.Stop();
    LOG_DEBUG("GeometryTest") << "\tdone testing closest channel";
    stopWatch.Print();
    
    // trigger an exception with NearestChannel
    mf::LogVerbatim("GeometryTest") << "\tattempt to cause an exception to be caught "
                                    << "when looking for a nearest channel";

    // pick a position out of the world
    double posWorld[3];
    geom->WorldBox(nullptr, posWorld + 0,
      nullptr, posWorld + 1, nullptr, posWorld + 2);
    for (int i = 0; i < 3; ++i) posWorld[i] *= 2.;

    bool hasThrown = false;
    unsigned int nearest_to_what = 0;
    try{
      nearest_to_what = geom->NearestChannel(posWorld, 0, 0, 0);
    }
    catch(const geo::InvalidWireIDError& e){
      mf::LogWarning("GeoTestCaughtException") << e
        << "\nReturned wire would be: " << e.wire_number
        << ", suggested: " << e.better_wire_number;
      hasThrown = true;
    }
    catch(cet::exception& e){
      mf::LogWarning("GeoTestCaughtException") << e;
      hasThrown = true;
    }
    if (!hasThrown) {
      if (fDisableValidWireIDcheck) {
        // ok, then why do we disable it?
        // an implementation might prefer to cap the wire number and go on
        // instead of throwing.
        LOG_WARNING("GeoTestErrorNearestChannel")
          << "GeometryCore::NearestChannel() did not raise an exception"
          " on out-of-world position (" << posWorld[0] << "; "
          << posWorld[1] << "; " << posWorld[2] << "), and returned "
          << nearest_to_what << " instead.\n"
          "This is normally considered a failure.";
      }
      else {
        throw cet::exception("GeoTestErrorNearestChannel")
          << "GeometryCore::NearestChannel() did not raise an exception"
          " on out-of-world position (" << posWorld[0] << "; "
          << posWorld[1] << "; " << posWorld[2] << "), and returned "
          << nearest_to_what << " instead\n";
      }
    }

  }

  //......................................................................
  void GeometryTestAlg::testWireIntersection() const {
    /*
     * This is a test for WireIDsIntersect() function, that returns whether
     * two wires intersect, and where.
     *
     * The test strategy is to check all the TPC one by one:
     * - if a query for wires on different cryostats fails
     * - if a query for wires on different TPCs fails
     * - if a query for wires on the same plane fails
     * - for points at the centre of a grid SplitY x SplitZ on the wire planes,
     *   test these point by testWireIntersectionAt() function (see)
     * All tests are performed; at the end, the test is considered a failure
     * if any of the single tests failed.
     */
    
    unsigned int nErrors = 0;
    for (geo::GeometryCore::TPC_id_iterator iTPC(&*geom); iTPC; ++iTPC) {
      const geo::TPCGeo& TPC = *(iTPC.get());
      
      LOG_DEBUG("GeometryTest") << "Cryostat #" << iTPC->Cryostat
        << " TPC #" << iTPC->TPC;
      
      // sanity: wires on different cryostats
      if (iTPC->Cryostat < geom->Ncryostats() - 1) {
        geo::WireID w1 { iTPC->Cryostat, iTPC->TPC, 0, 0 },
          w2 { iTPC->Cryostat + 1, iTPC->TPC, 1, 1 };
        geo::WireIDIntersection xing;
        if (geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "WireIDsIntersect() on " << w1
            << " and " << w2 << " returned (" << xing.y << "; " << xing.z
            << ") in TPC=" << xing.TPC
            << ", while should have reported no intersection at all";
          ++nErrors;
        } // if intersect
      } // if not the last cryostat
      
      // sanity: wires on different TPC
      if (iTPC->TPC < geom->NTPC(iTPC->Cryostat) - 1) {
        geo::WireID w1 { iTPC->Cryostat, iTPC->TPC, 0, 0 },
          w2 { iTPC->Cryostat, iTPC->TPC + 1, 1, 1 };
        geo::WireIDIntersection xing;
        if (geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "WireIDsIntersect() on " << w1
            << " and " << w2 << " returned (" << xing.y << "; " << xing.z
            << ") in TPC=" << xing.TPC
            << ", while should have reported no intersection at all";
          ++nErrors;
        } // if intersect
      } // if not the last TPC
      
      // sanity: wires on same plane
      const unsigned int nPlanes = TPC.Nplanes();
      for (unsigned int plane = 0; plane < nPlanes; ++plane) {
        geo::WireID w1 { iTPC->Cryostat, iTPC->TPC, plane, 0 },
          w2 { iTPC->Cryostat, iTPC->TPC, plane, 1 };
        geo::WireIDIntersection xing;
        if (geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "WireIDsIntersect() on " << w1
            << " and " << w2 << " returned (" << xing.y << "; " << xing.z
            << ") in TPC=" << xing.TPC
            << ", while should have reported no intersection at all";
          ++nErrors;
        } // if intersect
      } // for all planes
      
      
      // sample the area covered by all planes, split into SplitY and SplitZ
      // rectangles; x is chosen in the middle of the TPC
      constexpr unsigned int SplitZ = 19, SplitY = 17;
      // get the area spanned by the wires
      simple_geo::Area covered_area;
      for (geo::PlaneID::PlaneID_t p = 0; p < nPlanes; ++p) {
        simple_geo::Area plane_area = simple_geo::PlaneCoverage(TPC.Plane(p));
        if (covered_area.isEmpty()) covered_area = plane_area;
        else covered_area.Intersect(plane_area);
      } // for
      
      if (covered_area.isEmpty()) { // if so, debugging is needed
        throw cet::exception("GeometryTestAlg")
          << "testWireIntersection(): failed to find plane coverage";
      }
      
      std::array<double, 3> origin, TPC_center;
      origin.fill(0);
      TPC.LocalToWorld(origin.data(), TPC_center.data());
      const double x = TPC_center[0];
      
      // let's pick a point:
      for (unsigned int iZ = 0; iZ < SplitZ; ++iZ) {
        
        // pick the coordinate in the middle of the iZ-th region:
        const double z = covered_area.Min().z
          + covered_area.DeltaZ() * (2*iZ + 1) / (2*SplitZ);
        
        for (unsigned int iY = 0; iY < SplitY; ++iY) {
          // pick the coordinate in the middle of the iY-th region:
          const double y = covered_area.Min().y
            + covered_area.DeltaY() * (2*iY + 1) / (2*SplitY);
          
          // finally, let's test this point...
          nErrors += testWireIntersectionAt(*iTPC, x, y, z);
        } // for y
      } // for z
    } // for iTPC
    
    if (nErrors > 0) {
      throw cet::exception("GeoTestWireIntersection")
        << "Accumulated " << nErrors << " errors (see messages above)\n";
    }
    
  } // GeometryTestAlg::testWireIntersection()
  
  
  unsigned int GeometryTestAlg::testWireIntersectionAt
    (const TPCID& tpcid, double x, double y, double z) const
  {
    /* Tests WireIDsIntersect() on the specified point on the wire planes of
     * a given TPC.
     * 
     * The test follows this strategy:
     * - find the ID of the wires closest to the point on each plane
     * - for all wire plane pairing, ask for the intersection between the wires
     * - fail if the returned point is farther than half a pitch from the
     *   original point
     * 
     * This function returns the number of accumulated failures.
     */
    
    unsigned int nErrors = 0;
    
    const geo::TPCGeo& TPC = geom->TPC(tpcid);
    const unsigned int NPlanes = TPC.Nplanes();
    
    // collect information per plane:
    std::vector<double> ThetaZ(NPlanes), WirePitch(NPlanes); // for convenience
    std::vector<geo::WireID> WireIDs; // ID of the closest wire
    WireIDs.reserve(NPlanes);
    std::vector<double> WireDistances(NPlanes); // distance from the closest wire
    for (unsigned int iPlane = 0; iPlane < NPlanes; ++iPlane) {
      const geo::PlaneGeo& plane = TPC.Plane(iPlane);
      ThetaZ[iPlane] = plane.FirstWire().ThetaZ();
      WirePitch[iPlane] = plane.WirePitch();
      
      const double WireDistance
        = geom->WireCoordinate(y, z, iPlane, tpcid.TPC, tpcid.Cryostat);
      WireIDs.emplace_back(
        tpcid.Cryostat, tpcid.TPC, iPlane,
        (unsigned int) std::round(WireDistance)
        );
      WireDistances[iPlane]
        = (WireDistance - std::round(WireDistance)) * WirePitch[iPlane];
      
      LOG_DEBUG("GeometryTest") << "Nearest wire to"
        " (" << x << ", " << y << ", " << z << ") on plane #" << iPlane
        << " (pitch: " << WirePitch[iPlane] << ", thetaZ=" << ThetaZ[iPlane]
        << ") is " << WireIDs[iPlane] << " (position: " << WireDistance << ")";
    } // for planes
    
    // test all the combinations
    for (unsigned int iPlane1 = 0; iPlane1 < NPlanes; ++iPlane1) {
      
      const geo::WireID& w1 = WireIDs[iPlane1];
      
      for (unsigned int iPlane2 = iPlane1 + 1; iPlane2 < NPlanes; ++iPlane2) {
        const geo::WireID& w2 = WireIDs[iPlane2];
        
        geo::WireIDIntersection xing;
        if (!geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "Wires " << w1 << " and " << w2
            << " should intersect around (" << x << ", " << y << ", " << z
            << ") of TPC " << tpcid
            << ", but they seem not to intersect at all!";
          ++nErrors;
          continue;
        }
        
        if (xing.TPC != tpcid.TPC) {
          LOG_ERROR("GeometryTest") << "Wires " << w1 << " and " << w2
            << " should intersect around (" << x << ", " << y << ", " << z
            << ") of TPC " << tpcid
            << ", but they seem to intersect in TPC #" << xing.TPC
            << " at (x, " << xing.y << "; " << xing.z << ")";
          ++nErrors;
          continue;
        }
        
        // the expected distance between the probe point (y, z) and the
        // intersection point is geometrically determined, given the distances
        // of the probe point from the two wires and the angle between the wires
        // the formula is a mix between the Carnot theorem and sine definition;
        const double dTheta = ThetaZ[iPlane1] - ThetaZ[iPlane2],
          d1 = WireDistances[iPlane1], d2 = WireDistances[iPlane2];
        const double expected_d = 
          std::sqrt(sqr(d1) + sqr(d2) - 2. * d1 * d2 * std::cos(dTheta))
          / std::abs(std::sin(dTheta));
        // the actual distance we have found:
        const double d = std::sqrt(sqr(xing.y - y) + sqr(xing.z - z));
        LOG_DEBUG("GeometryTest")
          << " - wires " << w1 << " and " << w2 << " intersect in TPC #"
          << xing.TPC << " at (x, " << xing.y << "; " << xing.z << "), "
          << d << " cm far from starting point (expected: " << expected_d << ")";
        
        // precision of the test is an issue; the 10^-3 x pitch threshold
        // is roughly tuned so that we don't get errors
        if (std::abs(d - expected_d)
          > std::max(WirePitch[iPlane1], WirePitch[iPlane2]) * 1e-3) // cm
        {
          LOG_ERROR("GeometryTest")
            << "wires " << w1 << " and " << w2 << " intersect in TPC #"
            << xing.TPC << " at (x, " << xing.y << "; " << xing.z << "), "
            << d << " cm far from starting point: too far from the expected "
            << expected_d << " cm!";
          ++nErrors;
          continue;
        } // if too far
        
      } // for iPlane2
    } // for iPlane1
    
    return nErrors;
  } // GeometryTestAlg::testWireIntersectionAt()


  //......................................................................
  void GeometryTestAlg::testThirdPlane() const {
    /*
     * This is a test for ThirdPlane() function, that returns the plane that is
     * not specified in the input.
     * Currently, the only implemented signature is designed for TPCs with 3
     * planes.
     *
     * The test strategy is to check all the TPC one by one:
     * - for all combinations of two planes, if the result is the expected one
     * 
     * All tests are performed; at the end, the test is considered a failure
     * if any of the single tests failed.
     */
    
    unsigned int nErrors = 0;
    for (geo::GeometryCore::TPC_id_iterator iTPC(&*geom); iTPC; ++iTPC) {
      const geo::TPCID tpcid = *iTPC;
      const geo::TPCGeo& TPC = geom->TPC(tpcid);
      
      const unsigned int nPlanes = TPC.Nplanes();
      LOG_DEBUG("GeometryTest") << tpcid << " (" << nPlanes << " planes)";
      
      
      for (geo::PlaneID::PlaneID_t iPlane1 = 0; iPlane1 < nPlanes; ++iPlane1) {
        geo::PlaneID pid1(tpcid, iPlane1);
        
        for (geo::PlaneID::PlaneID_t iPlane2 = 0; iPlane2 < nPlanes; ++iPlane2)
        {
          geo::PlaneID pid2(tpcid, iPlane2);
          
          const bool isValidInput = (nPlanes == 3) && (iPlane1 != iPlane2);
          bool bError = false;
          geo::PlaneID third_plane;
          try {
            third_plane = geom->ThirdPlane(pid1, pid2);
          }
          catch (cet::exception const& e) {
            if (isValidInput) throw;
            // we have gotten the error we were looking for
            // if "GeometryCore" is included in the categories of the exception
            bError = hasCategory(e, "GeometryCore");
          } // try...catch
          
          LOG_TRACE("GeometryTest")
            << "  [" << pid1 << "], [" << pid2 << "] => "
            << (bError? "error": std::string(third_plane));
          if (bError) continue; // we got what we wanted
          
          if (!bError && !isValidInput) {
            LOG_ERROR("GeometryTest") << "ThirdPlane() on " << pid1
              << " and " << pid2 << " returned " << third_plane
              << ", while should have thrown an exception";
            ++nErrors;
            continue;
          } // if no error
          
          if (third_plane != tpcid) {
            LOG_ERROR("GeometryTest") << "ThirdPlane() on " << pid1
              << " and " << pid2 << " returned " << third_plane
              << ", on a different TPC!!!";
            ++nErrors;
          }
          else if (!third_plane.isValid) {
            LOG_ERROR("GeometryTest") << "ThirdPlane() on " << pid1
              << " and " << pid2 << " returned an invalid " << third_plane;
            ++nErrors;
          }
          else if (third_plane.Plane >= nPlanes) {
            LOG_ERROR("GeometryTest") << "ThirdPlane() on " << pid1
              << " and " << pid2 << " returned " << third_plane
              << " with plane out of range";
            ++nErrors;
          }
          else if (third_plane == pid1) {
            LOG_ERROR("GeometryTest") << "ThirdPlane() on " << pid1
              << " and " << pid2 << " returned " << third_plane
              << ", same as the first input";
            ++nErrors;
          }
          else if (third_plane == pid2) {
            LOG_ERROR("GeometryTest") << "ThirdPlane() on " << pid1
              << " and " << pid2 << " returned " << third_plane
              << ", same as the second input";
            ++nErrors;
          }
          
        } // for plane 2
        
      } // for plane 1
      
    } // for TPC
    
    if (nErrors > 0) {
      throw cet::exception("GeoTestThirdPlane")
        << "Accumulated " << nErrors << " errors (see messages above)\n";
    }
    
  } // GeometryTestAlg::testThirdPlane()
  
  
  //......................................................................
  void GeometryTestAlg::testThirdPlane_dTdW() const {
    /*
     * This is a test for ThirdPlane_dTdW() function, that returns the apparent
     * slope on a wire plane, given the ones observed on other two planes.
     *
     * The test strategy is to check all the TPC one by one:
     * - if a query for planes on different cryostats fails
     * - if a query for planes on different TPCs fails
     * - for selected 3D points, compute the three dT/dW and verify them by
     *   test these slopes by testThirdPlane__dTdW_at() function (see)
     * 
     * All tests are performed; at the end, the test is considered a failure
     * if any of the single tests failed.
     */
    
    unsigned int nErrors = 0;
    for (geo::GeometryCore::TPC_id_iterator iTPC(&*geom); iTPC; ++iTPC) {
      const geo::TPCID tpcid = *iTPC;
      const geo::TPCGeo& TPC = geom->TPC(tpcid);
      
      const double driftVelocity = 0.1
        * ((TPC.DriftDirection() == geo::kNegX)? -1.: +1);
      
      const unsigned int NPlanes = TPC.Nplanes();
      LOG_DEBUG("GeometryTest") << tpcid << " (" << NPlanes << " planes)";
      
      // sanity: planes on different cryostats
      if (tpcid.Cryostat < geom->Ncryostats() - 1) {
        geo::PlaneID p1 { tpcid, 0 }, p2 { tpcid.Cryostat + 1, tpcid.TPC, 1 };
        bool bError = false;
        double slope;
        try {
          slope = geom->ThirdPlane_dTdW(p1, +1.0, p2, -1.0);
        }
        catch (cet::exception const& e) {
          // we have gotten the error we were looking for
          // if "GeometryCore" is included in the categories of the exception
          bError = hasCategory(e, "GeometryCore");
        } // try...catch
        if (!bError) {
          LOG_ERROR("GeometryTest") << "ThirdPlane_dTdW() on " << p1
            << " and " << p2 << " returned " << slope
            << ", while should have thrown an exception";
          ++nErrors;
        } // if no error
      } // if not the last cryostat
      
      // sanity: planes on different TPC
      if (tpcid.TPC < geom->NTPC(tpcid.Cryostat) - 1) {
        geo::PlaneID p1 { tpcid, 0 }, p2 { tpcid.Cryostat, tpcid.TPC + 1, 1 };
        bool bError = false;
        double slope;
        try {
          slope = geom->ThirdPlane_dTdW(p1, +1.0, p2, -1.0);
        }
        catch (cet::exception const& e) {
          // we have gotten the error we were looking for
          // if "GeometryCore" is included in the categories of the exception
          bError = hasCategory(e, "GeometryCore");
        } // try...catch
        if (!bError) {
          LOG_ERROR("GeometryTest") << "ThirdPlane_dTdW() on " << p1
            << " and " << p2 << " returned " << slope
            << ", while should have thrown an exception";
          ++nErrors;
        } // if no error
      } // if not the last TPC in its cryostat
      
      
      // pick a point in the very middle of the TPC:
      const std::array<double, 3> A
        = { TPC.CenterX(), TPC.CenterY(), TPC.CenterZ() };
      // pick a radius half the way to the closest border
      const double radius
        = std::min({ TPC.HalfWidth(), TPC.HalfHeight(), TPC.Length()/2. }) / 2.;
      
      // I arbitrary decide that the second point will have dX equal size as
      // the radius, and on the positive x direction (may be negative dT)
      const double dX = radius;
      const double dT = driftVelocity * dX;
      
      // sample a circle of SplitAngles directions around A
      constexpr unsigned int NAngles = 19;
      const double start_angle = 0.05; // radians
      const double step_angle = 2. * util::pi<double>() / NAngles; // radians
      
      for (unsigned int iAngle = 0; iAngle < NAngles; ++iAngle) {
        const double angle = start_angle + iAngle * step_angle;
        
        // define B as a point "radius" far from A in the angle direction,
        // with some arbitrary and fixed dx offset
        std::array<double, 3> B = {
          A[0] + dX,
          A[1] + radius * std::sin(angle),
          A[2] + radius * std::cos(angle)
          };
        
        // get the expectation; this function assumes a drift velocity of
        // 1 mm per tick by default; for the test, it does not matter
        std::vector<std::pair<geo::PlaneID, double>> dTdWs
          = ExpectedPlane_dTdW(A, B, driftVelocity);
        
        if (mf::isDebugEnabled()) {
          mf::LogTrace log("GeometryTest");
          log << "Expected dT/dW for a segment with " << radius
            << " cm long projection at "
            << angle << " rad, and dT " << dT << " cm:";
          for (auto const& dTdW_info: dTdWs)
            log << "  " << dTdW_info.first << " slope:" << dTdW_info.second;
        } // if debug
        
        // run the test
        nErrors += testThirdPlane_dTdW_at(dTdWs);
        
      } // for angle
      
    } // for TPC
    
    if (nErrors > 0) {
      throw cet::exception("GeoTestThirdPlane_dTdW")
        << "Accumulated " << nErrors << " errors (see messages above)\n";
    }
    
  } // GeometryTestAlg::testThirdPlane_dTdW()
  
  
  std::vector<std::pair<geo::PlaneID, double>>
  GeometryTestAlg::ExpectedPlane_dTdW(
    std::array<double, 3> const& A, std::array<double, 3> const& B,
    const double driftVelocity /* = 0.1 */
    ) const
  {
    // This function returns a list of entries, one for each plane:
    // - plane ID
    // - slope of the projection of AB from the plane, in dt/dw units
    
    // find which TPC we are taking about
    geo::TPCID tpcid = geom->FindTPCAtPosition(A.data());
    
    if (!tpcid.isValid) {
      throw cet::exception("GeometryTestAlg")
        << "ExpectedPlane_dTdW(): can't find any TPC containing point A ("
        << A[0] << "; " << A[1] << "; " << A[2] << ")";
    }
    
    if (geom->FindTPCAtPosition(B.data()) != tpcid) {
      throw cet::exception("GeometryTestAlg")
        << "ExpectedPlane_dTdW(): point A ("
        << A[0] << "; " << A[1] << "; " << A[2] << ") is in "
        << std::string(tpcid)
        << " while point B (" << B[0] << "; " << B[1] << "; " << B[2]
        << ") is in " << std::string(geom->FindTPCAtPosition(B.data()));
    }
    
    geo::TPCGeo const& TPC = geom->TPC(tpcid);
    
    // conversion from X coordinate to tick coordinate
    double dT_over_dX = 1. / driftVelocity;
    switch (TPC.DriftDirection()) {
      case geo::kPosX:
        // if the drift direction is toward positive x, higher x will reach the
        // plane earlier and have smaller t, hence the flip in sign
        dT_over_dX = -dT_over_dX;
        break;
      case geo::kNegX: break;
      default:
        throw cet::exception("InternalError")
          << "GeometryTestAlg::ExpectedPlane_dTdW(): drift direction #"
          << ((int) TPC.DriftDirection()) << " of " << std::string(tpcid)
          << " not supported.\n";
    } // switch drift direction
    
    const unsigned int nPlanes = TPC.Nplanes();
    std::vector<std::pair<geo::PlaneID, double>> slopes(nPlanes);
    
    for (geo::PlaneID::PlaneID_t iPlane = 0; iPlane < nPlanes; ++iPlane) {
      geo::PlaneID pid(tpcid, iPlane);
      const double wA = geom->WireCoordinate(A[1], A[2], pid);
      const double wB = geom->WireCoordinate(B[1], B[2], pid);
      
      slopes[iPlane]
        = std::make_pair(pid, ((B[0] - A[0]) * dT_over_dX) / (wB - wA));
      
    } // for iPlane
    
    return slopes;
  }  // GeometryTestAlg::ExpectedPlane_dTdW()
  
  
  unsigned int GeometryTestAlg::testThirdPlane_dTdW_at
    (std::vector<std::pair<geo::PlaneID, double>> const& plane_dTdW) const
  {
    /*
     * This function tests that for every combination of planes, the expected
     * slope is returned within some tolerance.
     * It returns the number of mistakes found.
     * 
     * The parameter is a list if pair of expected slope on the paired plane.
     */
    
    unsigned int nErrors = 0;
    for (std::pair<geo::PlaneID, double> const& input1: plane_dTdW) {
      for (std::pair<geo::PlaneID, double> const& input2: plane_dTdW) {
        
        const bool bValidInput = input1.first != input2.first;
        
        for (std::pair<geo::PlaneID, double> const& output: plane_dTdW) {
          bool bError = false;
          double output_slope = 0.;
          try {
            output_slope = geom->ThirdPlane_dTdW(
              input1.first, input1.second,
              input2.first, input2.second,
              output.first
              );
          }
          catch (cet::exception const& e) {
            // catch only "GeometryCore" category, and only if input is faulty;
            // otherwise, rethrow
            if (bValidInput) throw;
            bError = hasCategory(e, "GeometryCore");
            if (!bError) throw;
            LOG_TRACE("GeometryTest")
              << input1.first << " slope:" << input1.second
              << "  " << input2.first << " slope:" << input2.second
              << "  => exception";
            continue;
          }
          
          if (!bValidInput && !bError) {
            LOG_ERROR("GeometryTest")
              << "GeometryCore::ThirdPlane_dTdW() on "
              << input1.first << " and " << input2.first
              << " should have thrown an exception, it returned "
              << output_slope << " instead";
            ++nErrors;
            continue;
          } // if faulty input and no error
          
          LOG_TRACE("GeometryTest")
            << input1.first << " slope:" << input1.second
            << "  " << input2.first << " slope:" << input2.second
            << "  => " << output.first << " slope:" << output_slope;
          if (((output.second == 0.) && (output_slope > 1e-3))
            || std::abs(output_slope/output.second - 1.) > 1e-3) {
            LOG_ERROR("testThirdPlane_dTdW_at")
              << "GeometryCore::ThirdPlane_dTdW(): "
              << input1.first << " slope:" << input1.second
              << "  " << input2.first << " slope:" << input2.second
              << "  => " << output.first << " slope:" << output_slope
              << "  (expected: " << output.second << ")";
          } // if too far
          
          // now test the automatic detection of the other plane
          
        } // for output
      } // for second input
    } // for first input
    return nErrors;
  }  // GeometryTestAlg::testThirdPlane_dTdW_at()
  

  //......................................................................
  void GeometryTestAlg::testWirePitch()
  {
    // loop over all planes and wires to be sure the pitch is consistent
    unsigned int nPitchErrors = 0;

    if (fExpectedWirePitches.empty()) {
      // hard code the value we think it should be for each detector;
      // this is legacy and you should not add anything:
      // add the expectation to the FHiCL configuration of the test instead
      if(geom->DetectorName() == "bo") {
        fExpectedWirePitches = { 0.46977, 0.46977, 0.46977 };
      }
      if (!fExpectedWirePitches.empty()) {
        mf::LogInfo("WirePitch")
          << "Using legacy wire pitch parameters hard-coded for the detector '"
          << geom->DetectorName() << "'";
      }
    }
    if (fExpectedWirePitches.empty()) {
      mf::LogWarning("WirePitch")
        << "no expected wire pitch; I'll just check that they are all the same";
    }
    else {
      mf::LogInfo log("WirePitch");
      log << "Expected wire pitch per plane, in centimetres:";
      for (double pitch: fExpectedWirePitches) log << " " << pitch;
      log << " [...]";
    }
    
    for (geo::PlaneID const& planeid: geom->IteratePlaneIDs()) {
      
      geo::PlaneGeo const& plane = geom->Plane(planeid);
      const unsigned int nWires = plane.Nwires();
      if (nWires < 2) continue;
      
      geo::WireGeo const* pWire = &(plane.Wire(0));
      
      // which pitch to expect:
      // - if they did not tell us anything:
      //     get the one from the first two wires
      // - if they did tell something, but not for this plane:
      //     get the last pitch they told us
      // - if they told us about this plane: well, then use it!
      double expectedPitch = 0.;
      if (fExpectedWirePitches.empty()) {
        geo::WireGeo const& wire1 = plane.Wire(1); // pWire now points to wire0
        expectedPitch = geo::WireGeo::WirePitch(*pWire, wire1);
        LOG_DEBUG("WirePitch")
          << "Wire pitch on " << planeid << ": " << expectedPitch << " cm";
      }
      else if (planeid.Plane < fExpectedWirePitches.size())
        expectedPitch = fExpectedWirePitches[planeid.Plane];
      else
        expectedPitch = fExpectedWirePitches.back();
      
      geo::WireID::WireID_t w = 0; // wire number
      while (++w < nWires) {
        geo::WireGeo const* pPrevWire = pWire;
        pWire = &(plane.Wire(w));
        
        const double thisPitch = std::abs(pWire->DistanceFrom(*pPrevWire));
        if (std::abs(thisPitch - expectedPitch) > 1e-5) {
          mf::LogProblem("WirePitch") << "ERROR: on plane " << planeid
            << " pitch between wires W:" << (w-1) << " and W:" << w
            << " is " << thisPitch << " cm, not " << expectedPitch
            << " as expected!";
          ++nPitchErrors;
        } // if unexpected pitch
      } // while
      
    } // for
    
    if (nPitchErrors > 0) {
      throw cet::exception("UnexpectedWirePitch")
        << "unexpected pitches between " << nPitchErrors << " wires!";
    } // end loop over planes
    
  } // GeometryTestAlg::testWirePitch()

  //......................................................................
  void GeometryTestAlg::testPlanePitch()
  {
    // loop over all planes to be sure the pitch is consistent

    if (fExpectedPlanePitches.empty()) {
      // hard code the value we think it should be for each detector;
      // this is legacy and you should not add anything:
      // add the expectation to the FHiCL configuration of the test instead
      if(geom->DetectorName() == "bo") {
        fExpectedPlanePitches = { 0.65 };
      }
      if (!fExpectedPlanePitches.empty()) {
        mf::LogInfo("PlanePitch")
          << "Using legacy plane pitch parameters hard-coded for the detector '"
          << geom->DetectorName() << "'";
      }
    }
    if (fExpectedPlanePitches.empty()) {
      mf::LogWarning("PlanePitch")
        << "no expected plane pitch;"
        " I'll just check that they are all the same";
    }
    else {
      mf::LogInfo log("PlanePitch");
      log << "Expected plane pitch per plane pair, in centimetres:";
      for (double pitch: fExpectedPlanePitches) log << " " << pitch;
      log << " [...]";
    }
    
    unsigned int nPitchErrors = 0;
    for (geo::TPCID const& tpcid: geom->IterateTPCIDs()) {
      
      geo::TPCGeo const& TPC = geom->TPC(tpcid);
      const unsigned int nPlanes = TPC.Nplanes();
      if (nPlanes < 2) continue;
      
      double expectedPitch = 0.;
      if (fExpectedPlanePitches.empty()) {
        expectedPitch = TPC.PlanePitch(0, 1);
        LOG_DEBUG("PlanePitch")
          << "Plane pitch between the first two planes of " << tpcid << ": "
          << expectedPitch << " cm";
      }
      
      geo::PlaneID::PlaneID_t p = 0; // plane number
      while (++p < nPlanes) {
        // which pitch to expect:
        // - if they did not tell us anything:
        //     use the one from the first two planes (already in expectedPitch)
        // - if they did tell something, but not for this plane:
        //     get the last pitch they told us
        // - if they told us about this plane: well, then use it!
        if (!fExpectedPlanePitches.empty()) {
          if (p - 1 < fExpectedPlanePitches.size())
            expectedPitch = fExpectedPlanePitches[p - 1];
          else
            expectedPitch = fExpectedPlanePitches.back();
        } // if we have directions about plane pitch
        
        const double thisPitch = std::abs(TPC.PlanePitch(p - 1, p));
        if (std::abs(thisPitch - expectedPitch) > 1e-5) {
          mf::LogProblem("PlanePitch") << "ERROR: pitch of planes P:" << (p - 1)
            << " and P: " << p << " in " << tpcid 
            << " is " << thisPitch << " cm, not " << expectedPitch
            << " as expected!";
          ++nPitchErrors;
        } // if unexpected pitch
      } // while planes
      
    } // for TPCs
    
    if (nPitchErrors > 0) {
      throw cet::exception("UnexpectedPlanePitch")
        << "unexpected pitches between " << nPitchErrors << " planes!";
    } // end loop over planes
    
  } // GeometryTestAlg::testPlanePitch()

  //......................................................................

  void GeometryTestAlg::testStepping()
  {
    //
    // Test stepping. Example is similar to what one would do for photon
    // transport. Rattles photons around inside the scintillator
    // bouncing them off walls.
    //
    double xyz[3]      = {0.};
    double xyzWire[3]  = {0.};
    double dxyz[3]     = {0.};
    double dxyzWire[3] = {0, sin(0.1), cos(0.1)};

    geom->Plane(1).Wire(0).LocalToWorld(xyzWire,xyz);
    geom->Plane(1).Wire(0).LocalToWorldVect(dxyzWire,dxyz);

    mf::LogVerbatim("GeometryTest") << "\n\t" << xyz[0]  << "\t" << xyz[1]  << "\t" << xyz[2] ;
    mf::LogVerbatim("GeometryTest") << "\t"   << dxyz[0] << "\t" << dxyz[1] << "\t" << dxyz[2];

    gGeoManager->InitTrack(xyz, dxyz);
    for (int i=0; i<10; ++i) {
      const double* pos = gGeoManager->GetCurrentPoint();
      const double* dir = gGeoManager->GetCurrentDirection();
      mf::LogVerbatim("GeometryTest") << "\tnode = " 
				      << gGeoManager->GetCurrentNode()->GetName()
				      << "\n\t\tpos=" << "\t"
				      << pos[0] << "\t"
				      << pos[1] << "\t"
				      << pos[2]
				      << "\n\t\tdir=" << "\t"
				      << dir[0] << "\t"
				      << dir[1] << "\t"
				      << dir[2]
				      << "\n\t\tmat = " 
				      << gGeoManager->GetCurrentNode()->GetVolume()->GetMaterial()->GetName();
      
      gGeoManager->FindNextBoundary();
      gGeoManager->FindNormal();
      gGeoManager->Step(kTRUE,kTRUE);
    }

    xyz[0] = 306.108; xyz[1] = -7.23775; xyz[2] = 856.757;
    gGeoManager->InitTrack(xyz, dxyz);
    mf::LogVerbatim("GeometryTest") << "\tnode = " 
				    << gGeoManager->GetCurrentNode()->GetName()
				    << "\n\tmat = " 
				    << gGeoManager->GetCurrentNode()->GetVolume()->GetMaterial()->GetName();

    gGeoManager->GetCurrentNode()->GetVolume()->GetMaterial()->Print();

  }

  //......................................................................

  void GeometryTestAlg::testProject() 
  {
    double xlo, xhi;
    double ylo, yhi;
    double zlo, zhi;
    geom->WorldBox(&xlo, &xhi, &ylo, &yhi, &zlo, &zhi);
  
    double xyz[3]   = { 0.0, 0.0, 0.0};
    double dxyz1[3] = { 1.0, 0.0, 0.0};
    double dxyz2[3] = {-1.0, 0.0, 0.0};
    double dxyz3[3] = { 0.0, 1.0, 0.0};
    double dxyz4[3] = { 0.0,-1.0, 0.0};
    double dxyz5[3] = { 0.0, 0.0, 1.0};
    double dxyz6[3] = { 0.0, 0.0,-1.0};

    double xyzo[3];
    geo::ProjectToBoxEdge(xyz, dxyz1, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[0]-xhi)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz2, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[0]-xlo)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz3, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[1]-yhi)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz4, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[1]-ylo)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz5, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[2]-zhi)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz6, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[2]-zlo)>1.E-6) abort();
  }

  
  //......................................................................
  
  inline bool GeometryTestAlg::shouldRunTests(std::string test_name) const {
    return (*fRunTests.get())(test_name);
  } // GeometryTestAlg::shouldRunTests()

}//end namespace
