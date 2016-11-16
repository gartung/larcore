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
#include "larcore/Geometry/SimpleGeo.h"
#include "larcore/Geometry/GeometryCore.h"
#include "larcore/Geometry/CryostatGeo.h"
#include "larcore/Geometry/TPCGeo.h"
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"
#include "larcore/Geometry/OpDetGeo.h"
#include "larcore/Geometry/AuxDetGeo.h"
#include "larcore/Geometry/geo.h"
#include "larcoreobj/SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h" // util::pi<>

// Framework includes
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// ROOT includes
#include "TGeoManager.h"
#include "TStopwatch.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TClass.h"

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
#include <initializer_list> 


namespace {
  template <typename T>
  inline T sqr(T v) { return v*v; }
  

  /// Returns whether the CET exception e contains the specified category cat
  bool hasCategory(cet::exception const& e, std::string const& cat) {
    for (auto const& e_category: e.history())
      if (e_category == cat) return true;
    return false;
  } // hasCategory()
  
  
} // local namespace


namespace geo{
  
  
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
    // 
    // our name selector accepts everything by default;
    // the default set skips the following:
    fRunTests.AddToDefinition("default",
      "-CheckOverlaps", "-ThoroughCheck", "-PrintWires"
      );
    fRunTests.ParseNames("@default"); // let's start from default
    
    // read and parse the test list from the configuration parameters (if any)
    fRunTests.Parse(pset.get<std::vector<std::string>>("RunTests", {}));
    
    std::ostringstream sstr;
    fRunTests.PrintConfiguration(sstr);
    
    mf::LogInfo("GeometryTestAlg") << "Test selection:" << sstr.str();
    
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
    
    mf::LogVerbatim("GeometryTest")
      << "  Running on detector: '" << geom->DetectorName() << "'"
      << "\nGeometry file: " << geom->ROOTFile();
    
    try{
      if (shouldRunTests("DetectorIntro")) {
        LOG_INFO("GeometryTest") << "detector introduction:";
        printDetectorIntro();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("CheckOverlaps")) {
        LOG_INFO("GeometryTest") << "test for overlaps ...";
        gGeoManager->CheckOverlaps(1e-5);
        gGeoManager->PrintOverlaps();
        if (!gGeoManager->GetListOfOverlaps()->IsEmpty()) {
          mf::LogError("GeometryTest")
            << gGeoManager->GetListOfOverlaps()->GetSize()
            << " overlaps found in geometry during overlap test!";
          ++nErrors;
        }
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("ThoroughCheck")) {
        LOG_INFO("GeometryTest") << "thorough geometry test ...";
        gGeoManager->CheckGeometryFull();
        if (!gGeoManager->GetListOfOverlaps()->IsEmpty()) {
          mf::LogError("GeometryTest")
            << gGeoManager->GetListOfOverlaps()->GetSize()
            << " overlaps found in geometry during thorough test!";
          ++nErrors;
        }
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Cryostat")) {
        LOG_INFO("GeometryTest") << "test Cryostat methods ...";
        testCryostat();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("FindVolumes")) {
        LOG_INFO("GeometryTest") << "test FindAllVolumes method ...";
        testFindVolumes();
        LOG_INFO("GeometryTest") << "complete.";
      }

      if (shouldRunTests("ChannelToROP")) {
        LOG_INFO("GeometryTest") << "test channel to ROP and back ...";
        testChannelToROP();
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
    
    std::ostringstream out;
    if (!fRunTests.CheckQueryRegistry(out)) {
      throw cet::exception("GeometryTest")
        << "(postumous) configuration error detected!\n"
        << out.rdbuf();
    }
    
    mf::LogInfo log("GeometryTest");
    log << "Tests completed:";
    auto tests_run = fRunTests.AcceptedNames();
    if (tests_run.empty()) {
      log << "\n  no test run";
    }
    else {
      log << "\n  " << tests_run.size() << " tests run:\t ";
      for (std::string const& test_name: tests_run) log << " " << test_name;
    }
    auto tests_skipped = fRunTests.RejectedNames();
    if (!tests_skipped.empty()) {
      log << "\n  " << tests_skipped.size() << " tests skipped:\t ";
      for (std::string const& test_name: tests_skipped) log << " " << test_name;
    }

    return nErrors;
  } // GeometryTestAlg::Run()



  //......................................................................
  void GeometryTestAlg::printDetectorIntro() const {
    
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
      << "\nTotal number of TPCs " << geom->TotalNTPC()
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
    
  } // GeometryTestAlg::printDetectorIntro()
  
  
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
      
      // large verbosity
      plane.printPlaneInfo(mf::LogVerbatim("GeometryTest"), indent, 8);
      
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
    mf::LogVerbatim("GeometryTest") << "There are " << geom->Ncryostats() << " cryostats in the detector";

    for(geo::CryostatGeo const& cryo: geom->IterateCryostats()) {
      
      mf::LogVerbatim("GeometryTest") 
        << "\n\tCryostat " << cryo.ID()
        <<   " " << cryo.Volume()->GetName()
        <<   " Dimensions [cm]: " << cryo.Width()
        <<                  " x " << cryo.Height() 
        <<                  " x " << cryo.Length()
        << "\n\t\tmass [kg]: " << cryo.Mass()
        << "\n\t\tCryostat boundaries:"
        <<   "  -x:" << cryo.MinX() << " +x:" << cryo.MaxX()
        <<   "  -y:" << cryo.MinY() << " +y:" << cryo.MaxY()
        <<   "  -z:" << cryo.MinZ() << " +z:" << cryo.MaxZ();

      // pick a position in the middle of the cryostat in the world coordinates
      double const worldLoc[3]
        = { cryo.CenterX(), cryo.CenterY(), cryo.CenterZ() };
                
      LOG_DEBUG("GeometryTest") << "\t testing GeometryCore::PoitionToCryostat....";
      geo::CryostatID cid;
      try{
        geom->PositionToCryostat(worldLoc, cid);
      }
      catch(cet::exception &e){
        mf::LogWarning("FailedToLocateCryostat") << "\n exception caught:" << e;
        if (fNonFatalExceptions.count(e.category()) == 0) throw;
      }
      if (cid != cryo.ID()) {
        throw cet::exception("CryostatTest")
          << "Geometry points the middle of cryostat " << std::string(cryo.ID())
          << " to a different one (" << std::string(cid) << ")\n";
      }
      LOG_DEBUG("GeometryTest") << "done";

      LOG_DEBUG("GeometryTest") << "\t Now test the TPCs associated with this cryostat";
      testTPC(cryo.ID());
    }

    return;
  }

  //......................................................................
  unsigned int GeometryTestAlg::testFindWorldVolumes() {
    
    unsigned int nErrors = 0;
    
    std::set<std::string> volume_names;
    std::vector<TGeoNode const*> nodes;
    
    // world
    volume_names.insert(geom->GetWorldVolumeName());
    nodes = geom->FindAllVolumes(volume_names);
    {
      mf::LogVerbatim log("GeometryTest");
      log << "Found " << nodes.size() << " world volumes '"
        << geom->GetWorldVolumeName() << "':";
      for (TGeoNode const* node: nodes) {
        TGeoVolume const* pVolume = node->GetVolume();
        log << "\n - '" << pVolume->GetName() << "' (a "
          << pVolume->GetShape()->GetName() << ")";
      } // for
    } // anonymous block
    if (nodes.size() != 1) {
      ++nErrors;
      mf::LogError("GeometryTest")
        << "Found " << nodes.size() << " world volumes '"
          << geom->GetWorldVolumeName() << "! [expecting: one!!]";
    } // if nodes
    
    return nErrors;
  } // GeometryTestAlg::testFindWorldVolumes()
  
  
  unsigned int GeometryTestAlg::testFindCryostatVolumes() {
    
    unsigned int nErrors = 0;
    
    std::set<std::string> volume_names;
    volume_names.insert(geom->GetWorldVolumeName());
    volume_names.insert("volCryostat");
    
    std::vector<TGeoNode const*> nodes = geom->FindAllVolumes(volume_names);
    
    mf::LogVerbatim log("GeometryTest");
    log << "Found " << nodes.size() << " world and cryostat volumes:";
    for (TGeoNode const* node: nodes) {
      TGeoVolume const* pVolume = node->GetVolume();
      log << "\n - '" << pVolume->GetName() << "' (a "
        << pVolume->GetShape()->GetName() << ")";
    } // for
    if (nodes.size() != (1 + geom->Ncryostats())) {
      ++nErrors;
      mf::LogError("GeometryTest")
        << "Found " << nodes.size() << " world and cryostat volumes! "
        "[expecting: 1 world and " << geom->Ncryostats() << " cryostats]";
    } // if nodes
    
    return nErrors;
  } // GeometryTestAlg::testFindCryostatVolumes()
  
  
  unsigned int GeometryTestAlg::testFindTPCvolumePaths() {
    
    unsigned int nErrors = 0;
    
    // search the full path of all TPCs
    std::set<std::string> volume_names;
    for (geo::TPCGeo const& TPC: geom->IterateTPCs())
      volume_names.insert(TPC.TotalVolume()->GetName());
    
    // get the right answer: how many TPCs?
    const unsigned int NTPCs = geom->TotalNTPC();
    
    std::vector<std::vector<TGeoNode const*>> node_paths
      = geom->FindAllVolumePaths(volume_names);
    
    mf::LogVerbatim log("GeometryTest");
    log << "Found " << node_paths.size() << " TPC volumes:";
    for (auto const& path: node_paths) {
      TGeoNode const* node = path.back();
      TGeoVolume const* pVolume = node->GetVolume();
      log << "\n - '" << pVolume->GetName() << "' (a "
        << pVolume->GetShape()->GetName() << ") with " << (path.size()-1)
        << " ancestors";
      for (TGeoNode const* pNode: path) {
        TGeoVolume const* pVolume = pNode->GetVolume();
        log << "\n      * '" << pVolume->GetName() << "' (a "
          << pVolume->GetShape()->GetName() << ") with a "
          << pNode->GetMatrix()->IsA()->GetName() << " that "
          << (pNode->GetMatrix()->IsTranslation()? "is": "is not")
          << " a simple translation";
      } // for node
    } // for path
    if (node_paths.size() != NTPCs) {
      ++nErrors;
      mf::LogError("GeometryTest")
        << "Found " << node_paths.size() << " TPC volumes! "
        "[expecting: " << NTPCs << " TPCs]";
    } // if missed some
    
    return nErrors;
  } // GeometryTestAlg::testFindTPCvolumePaths()
  
  
  void GeometryTestAlg::testFindVolumes() {
    /*
     * Finds and checks a selected number of volumes by name:
     * - world volume
     * - cryostat volumes
     * - TPCs (returns full paths)
     */
    
    unsigned int nErrors = 0;
    
    nErrors += testFindWorldVolumes();
    nErrors += testFindCryostatVolumes();
    nErrors += testFindTPCvolumePaths();
    
    if (nErrors != 0) {
      throw cet::exception("FindVolumes")
        << "Collected " << nErrors << " errors during FindAllVolumes() test!\n";
    }
    
  } // GeometryTestAlg::testFindVolumes()
  
  
  //......................................................................
  void GeometryTestAlg::testTPC(geo::CryostatID const& cid)
  {
    geo::CryostatGeo const& cryo = geom->Cryostat(cid);

    mf::LogVerbatim("GeometryTest") << "\tThere are " << cryo.NTPC() 
                                    << " TPCs in the detector";
    
    for(size_t t = 0; t < cryo.NTPC(); ++t){
      geo::TPCID const tpcid(cid, t);
      geo::TPCGeo const& tpc = cryo.TPC(tpcid);
      
      mf::LogVerbatim("GeometryTest")
        << "\n\t\tTPC " << tpcid
          << " " << geom->GetLArTPCVolumeName(tpcid) 
          << " has " << tpc.Nplanes() << " planes."
        << "\n\t\tTPC location: ( "
          << tpc.MinX() << " ; " << tpc.MinY() << " ; "<< tpc.MinZ()
          << " ) =>  ( "
          << tpc.MaxX() << " ; " << tpc.MaxY() << " ; "<< tpc.MaxZ()
          << " ) [cm]"
        << "\n\t\tTPC Dimensions: "
          << tpc.Width() << " x " << tpc.Height() << " x " << tpc.Length()
        << "\n\t\tTPC Active Dimensions: " 
          << 2.*tpc.ActiveHalfWidth() << " x " << 2.*tpc.ActiveHalfHeight() << " x " << tpc.ActiveLength()
        << "\n\t\tTPC mass: " << tpc.ActiveMass()
        << "\n\t\tTPC drift distance: " << tpc.DriftDistance();
      
      for(size_t p = 0; p < tpc.Nplanes(); ++p) {
        geo::PlaneID const planeid(tpcid, p);
        geo::PlaneGeo const& plane = tpc.Plane(p);
        auto const planeLocation = tpc.PlaneLocation(p);
        mf::LogVerbatim("GeometryTest")
          << "\t\tPlane " << p << " has " << plane.Nwires() 
            << " wires and is at (x,y,z) = (" 
            << planeLocation[0] << "," 
            << planeLocation[1] << "," 
            << planeLocation[2] 
            << ");"
          << "\n\t\t\tpitch from plane 0 is " << tpc.Plane0Pitch(p) << ";"
          << "\n\t\t\tOrientation " << plane.Orientation()
            << ", View " << plane.ViewName(plane.View())
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
      // pick a position in the middle of the TPC in the world coordinates
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
      
      if (std::abs(middle_coord - double(middle_wire_id.Wire)) > 2e-3) {
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
      
      if (std::abs(next_coord - double(next_wire_id.Wire)) > 2e-3) {
        throw cet::exception("WireCoordAngle")
          << "Position (" << on_next_wire[0]
          << "; " << on_next_wire[1] << "; " << on_next_wire[2]
          << ") is expected to be on wire " << std::string(next_wire_id)
          << " but it has wire coordinate " << next_coord << "\n";
      } // if
      
    } // for
    
  } // GeometryTestAlg::testWireCoordAngle()
  

  //......................................................................
  void GeometryTestAlg::testChannelToROP() const {
    
    // test that an invalid channel yields an invalid ROP
    try {
      readout::ROPID invalidROP = geom->ChannelToROP(raw::InvalidChannelID);
      if (invalidROP.isValid) {
        throw cet::exception("testChannelToROP")
          << "ROP from an invalid channel ("
          << raw::InvalidChannelID << ") is " << std::string(invalidROP)
          << " !?\n";
      } // if invalid rop is not invalid
    }
    catch (cet::exception const& e) {
      mf::LogWarning("testChannelToROP")
        << "Non-compilant ChannelToROP() throws on invalid channel.";
    }
    
    // for each channel, test that its ROP contains it;
    // we assume each ROP contains contiguous channel IDs
    for (raw::ChannelID_t channel = 0; channel < geom->Nchannels(); ++channel) {
      
      readout::ROPID const ropid = geom->ChannelToROP(channel);
      
      auto const firstChannel = geom->FirstChannelInROP(ropid);
      auto const lastChannel = firstChannel + geom->Nchannels(ropid);
      
      if ((channel < firstChannel) || (channel >= lastChannel)) {
        throw cet::exception("testChannelToROP")
          << "Channel " << channel << " comes from ROP " << std::string(ropid)
          << ", which contains only channels from " << firstChannel
          << " to " << lastChannel << " (excluded)\n";
      } // if
      
    } // for channel
    
    
  } // GeometryTestAlg::testChannelToROP()
  
  //......................................................................
  void GeometryTestAlg::testChannelToWire() const
  {
    using std::begin;
    using std::end;
    
    geo::PlaneID lastPlane; // starts invalid
    geo::View_t planeView = geo::kUnknown;
    geo::SigType_t planeSigType = geo::kMysteryType;

    for(geo::WireID testWireID: geom->IterateWireIDs()){
      
      raw::ChannelID_t channel = geom->PlaneWireToChannel(testWireID);
      
      if (!raw::isValidChannelID(channel)) {
        throw cet::exception("BadChannelLookup")
          << "Invalid channel returned for wire " << std::string(testWireID)
          << "\n";
      }
      
      auto const wireIDs = geom->ChannelToWire(channel);
      
      if (wireIDs.empty()) {
        throw cet::exception("BadChannelLookup")
          << "List of wires for channel #" << channel << " is empty;"
          << " it should have contained at least " << std::string(testWireID)
         << "\n";
      }

      if (std::count(begin(wireIDs), end(wireIDs), testWireID) != 1) {
        mf::LogError log("GeometryTest");
        log << wireIDs.size() << " wire IDs associated with channel #"
          << channel << ":";
        for (auto const& wid: wireIDs)
          log << "\n  " << wid;
        throw cet::exception("BadChannelLookup")
          << "Wire " << std::string(testWireID) << " is on channel #" << channel 
          << " but ChannelToWire() does not map the channel to that wire\n";
      }
      
      // currently (LArSoft 6.12) signal type from channel and from plane use
      // the same underlying code, so the following test is not very valuable
      auto const channelSigType = geom->SignalType(channel);
      if (channelSigType != geom->SignalType(testWireID.planeID())) {
        throw cet::exception("BadChannelLookup")
          << "Geometry service claims channel #" << channel
          << " to be of type " << channelSigType
          << " but that the plane of " << std::string(testWireID)
          << " is of type " << geom->SignalType(testWireID.planeID()) << "\n";
      }
      
      auto const channelView = geom->View(channel);
      if (channelView != geom->Plane(testWireID).View()) {
        throw cet::exception("BadChannelLookup")
          << "Geometry service claims channel #" << channel
          << " should be on view " << geom->View(channel)
          << " but the plane of " << std::string(testWireID)
          << " claims to be on view " << geom->Plane(testWireID).View() << "\n";
      }
      
      // check that all the channels on the same plane are consistent
      // (sort of: it's more like "all contiguous wires in the same plane")
      if (testWireID.planeID() == lastPlane) {
        if (planeView != channelView) {
          throw cet::exception("BadChannelLookup")
            << "Geometry service claims channel #" << channel
            << " is on view " << channelView
            << " but its plane " << std::string(lastPlane)
            << " claims to be on view " << planeView << "\n";
        }
        if (planeSigType != channelSigType) {
          throw cet::exception("BadChannelLookup")
            << "Geometry service claims channel #" << channel
            << " is if type " << channelSigType
            << " but its plane " << std::string(lastPlane)
            << " to be of type " << planeSigType << "\n";
        }
      }
      else {
        lastPlane = testWireID.planeID();
        planeView = channelView;
        planeSigType = channelSigType;
      }
      
    } // for wires in detector
    
  } // GeometryTestAlg::testChannelToWire()
  
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
              throw cet::exception("WireOrderProblem")         << "\n\twires do not increase in +z order in"
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
              throw cet::exception("WireOrderProblem")         << "\n\ttop TPC wires do not increase in -y order in"
                                                        << "Cryostat " << cs
                                                        << ", TPC " << t
                                                        << ", Plane " << i
                                                        << ";  at wire " << j << "\n";
            // bottom TPC wires increase in +y
            else if(tpcworld[1] < 0 && xyz[1] < xyzprev[1])
              throw cet::exception("WireOrderProblem")         << "\n\tbottom TPC wires do not increase in +y order in"
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
    for (geo::PlaneGeo const& plane: geom->IteratePlanes()) {
      
      geo::PlaneID const& planeID = plane.ID();
      const unsigned int NWires = plane.Nwires();
      
      const std::array<double, 3> IncreasingWireDir
        = GetIncreasingWireDirection(plane);
      
      LOG_DEBUG("GeoTestWireCoordinate")
        << "The direction of increasing wires for plane " << planeID
        << " (theta=" << plane.Wire(0).ThetaZ() << " pitch="
        << plane.WirePitch() << " orientation="
        << (plane.Orientation() == geo::kHorizontal? "H": "V")
        << (plane.WireIDincreasesWithZ()? "+": "-")
        << ") is ( " << IncreasingWireDir[0] << " ; "
        << IncreasingWireDir[1] << " ; " << IncreasingWireDir[2] << ")";
      
      for (unsigned int w = 0; w < NWires; ++w) {
        
        geo::WireID wireID(planeID, w);
        
        const geo::WireGeo& wire = plane.Wire(w);
        const double pos[3] = {0., 0.0, 0.};
        std::array<double, 3> wire_center;
        wire.LocalToWorld(pos, wire_center.data());
        
        uint32_t nearest = 0;
        std::vector< geo::WireID > wireIDs;
        
        try{
          // The double[] version tested here falls back on the
          // TVector3 version, so this test both.
          nearest = geom->NearestChannel(wire_center.data(), planeID);
          
          // We also want to test the std::vector<double> version
          std::array<double, 3> posWorldV;
          for (int i=0; i<3; ++i) {
            posWorldV[i] = wire_center[i] + 0.001;
          }
          nearest = geom->NearestChannel(posWorldV.data(), planeID);
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
                                                         << std::string(wireID)
                                                         << "\n";
          }
        }
        catch(cet::exception &e){
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
        if(std::find(wireIDs.begin(), wireIDs.end(), wireID) == wireIDs.end()) {
          throw cet::exception("BadPositionToChannel") << "Current WireID ("
                                                       << std::string(wireID) << ") "
                                                       << "has a world position at "
                                                       << wire_center[0] << " " 
                                                       << wire_center[1] << " " 
                                                       << wire_center[2] << "\n"
                                                       << "NearestWire for this position is "
                                                       << geom->NearestWire(wire_center.data(),planeID) << "\n"
                                                       << "NearestChannel is " 
                                                       << nearest << " for " 
                                                       << std::string(wireID) << "\n"
                                                       << "Should be channel "
                                                       << geom->PlaneWireToChannel(wireID);
        } // if good lookup fails
        
        
        // nearest wire, integral and floating point
        try {
          // The test consists in sampling NStep (=5) points between the current
          // wire and the previous/next, following the normal to the wire.
          // We expect WireCoordinate() to reflect the same shift.
          
          // using absolute value just in case (what happens if w1 > w2?)
          const double pitch
            = std::abs(geom->WirePitch(planeID, (w > 0)? w - 1: 1, w));
          
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
                LOG_ERROR("GeoTestWireCoordinate")
                  << "The direction of increasing wires for plane "
                  << std::string(planeID)
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
                  (wire_shifted[1], wire_shifted[2], planeID);
              }
              catch (cet::exception& e) {
                if (hasCategory(e, "NotImplemented")) {
                  LOG_ERROR("GeoTestWireCoordinate")
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
                mf::LogError("GeoTestWireCoordinate")
                  << "wire " << wireID
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
                wire_number_from_wc = geom->NearestWire(wire_shifted, planeID);
              }
              catch (cet::exception& e) {
                throw cet::exception("GeoTestErrorWireCoordinate", "", e)
              //  LOG_ERROR("GeoTestWireCoordinate")
                  << "wire " << std::string(wireID)
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
                e << "wire " << wireID
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
                  LOG_ERROR("GeoTestWireCoordinate") << e.str();
                }
                else {
                  mf::LogVerbatim("GeoTestWireCoordinate") << e.str();
                }
              }
              else if (wire_number_from_wc != expected_wire_number) {
                // In production mode, we don't print anything and throw on error
                throw cet::exception("GeoTestErrorWireCoordinate")
                  << "wire " << std::string(wireID)
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
    } // end loop over planes

    stopWatch.Stop();
    LOG_DEBUG("GeoTestWireCoordinate") << "\tdone testing closest channel";
    stopWatch.Print();
    
    // trigger an exception with NearestChannel
    mf::LogVerbatim("GeoTestWireCoordinate") << "\tattempt to cause an exception to be caught "
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
        LOG_WARNING("GeoTestWireCoordinate")
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
      // get the area spanned by the wires;
      // this area is described as (x,y), regardless the global coordinate names
      lar::util::simple_geo::Area<> covered_area;
      for (geo::PlaneID::PlaneID_t p = 0; p < nPlanes; ++p) {
        auto plane_volume = TPC.Plane(p).Coverage(); // this is a volume
        lar::util::simple_geo::Area<> plane_area(
          { plane_volume.Min().y, plane_volume.Min().z },
          { plane_volume.Max().y, plane_volume.Max().z }
          );
        if (covered_area.isEmpty()) covered_area = plane_area;
        else covered_area.Intersect(plane_area);
      } // for
      
      std::array<double, 3> origin, TPC_center;
      origin.fill(0);
      TPC.LocalToWorld(origin.data(), TPC_center.data());
      const double x = TPC_center[0];
      
      // let's pick a point:
      for (unsigned int iZ = 0; iZ < SplitZ; ++iZ) {
        
        // pick the coordinate in the middle of the iZ-th region:
        const double z = covered_area.Min().y
          + covered_area.DeltaY() * (2*iZ + 1) / (2*SplitZ);
        
        for (unsigned int iY = 0; iY < SplitY; ++iY) {
          // pick the coordinate in the middle of the iY-th region:
          const double y = covered_area.Min().x
            + covered_area.DeltaX() * (2*iY + 1) / (2*SplitY);
          
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
    return fRunTests(test_name);
  } // GeometryTestAlg::shouldRunTests()

}//end namespace
