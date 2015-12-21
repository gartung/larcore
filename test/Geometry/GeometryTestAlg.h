/**
 * @file   GeometryTestAlg.h
 * @brief  Unit test for geometry functionalities
 * @date   2011/02/17
 * @author brebel@fnal.gov
 * @see    GeometryTestAlg.cxx
 * 
 * Refactored by Gianluca Petrillo on May 5th, 2015.
 */

#ifndef GEO_GEOMETRYTESTALG_H
#define GEO_GEOMETRYTESTALG_H

// LArSoft includes
#include "test/Geometry/NameSelector.h"
#include "larcore/SimpleTypesAndConstants/geo_types.h"

// C/C++ standard libraries
#include <string>
#include <set>
#include <vector>
#include <array>
#include <memory> // std::unique_ptr<>

// forward declarations
namespace fhicl {
  class ParameterSet;
}


namespace geo {
  
  // forward declarations
  class GeometryCore;
  class TPCGeo;
  class PlaneGeo;
  
  namespace details {
    class TestTrackerClassBase;
  } // namespace details
  
  
  /** **************************************************************************
   * @brief Performs tests on the geometry as seen by Geometry service
   * 
   * Configuration parameters
   * =========================
   * 
   * - **DisableWireBoundaryCheck** (boolean, default: false): the exceptions
   *   thrown when checking wire boundaries are not fatal
   * - **ExpectedWirePitches** (list of reals, default: empty):
   *   if specified, marks the expected uniform wire pitch, one entry for each
   *   plane; if there are more planes than elements, the missing pitches are
   *   assumed the same as the one in the last plane; if the parameter is
   *   not specified or empty, no expectation is taken, but the spacing is still
   *   checked to be uniform; exception: for legacy behaviour, default pitches
   *   are provided for some experiments
   * - **ExpectedPlanePitches** (list of reals, default: empty):
   *   if specified, marks the expected uniform plane pitch, one entry for each
   *   plane vs. the following. It works as /ExpectedWirePitches/
   * - **ForgiveExceptions** (list of strings, default: empty): the categories
   *   of exceptions in this list are "forgiven" (non-fatal)
   * - **RunTests** (string list): marks which tests to run; each entry can be
   *   specified with a "+" or "-" prepended, to indicate to add or to remove
   *   the test from the set to be run, respectively. The directives act against
   *   a pre-existing default set that executes all tests, except the ones
   *   explicitly marked as excluded by default in the list below:
   *   + `CheckOverlaps` (not in default) perform overlap checks
   *   + `ThoroughCheck` (not in default) makes ROOT perform full geometry check
   *   + `Cryostat`:
   *   + `ChannelToWire`:
   *   + `FindPlaneCenters`:
   *   + `Projection`:
   *   + `WirePos`: currently disabled
   *   + `WireCoordAngle`: tests geo::PlaneGeo::PhiZ()
   *   + `NearestWire`: tests `WireCoordinate()` and `NearestWire()`
   *   + `WireIntersection`: tests `WireIDsIntersect()`
   *   + `ThirdPlane`: tests `ThirdPlane()`
   *   + `ThirdPlaneSlope`: tests `ThirdPlaneSlope()`
   *   + `WirePitch`:
   *   + `PlanePitch`:
   *   + `Stepping`:
   *   + `PrintWires`: (not in default) prints *all* the wires in the geometry
   *   + `default`: represents the default set (optionally prepended by '@')
   *   + `!` (special): means to forget the tests configured so far; used as the
   *     first test name, removes the default list but leaves unchanged the
   *     default behaviour (the one specified with "+*" or "-*")
   * - **CheckForOverlaps** (boolean, default: false): equivalent to enabling
   *   `+CheckOverlaps` in `RunTests`
   * - **PrintWires**: (boolean, default: false): equivalent to enabling
   *   `+PrintWires` in `RunTests`
   */
  class GeometryTestAlg {
      public:
    explicit GeometryTestAlg(fhicl::ParameterSet const& pset);
    
    /// Virtual destructor
    virtual ~GeometryTestAlg() = default;
    
    /// Runs the test
    virtual void Setup(geo::GeometryCore const& new_geo) { geom = &new_geo; }

    /// Runs the test, returns a number of errors (very unlikely!)
    virtual unsigned int Run();

    /// Returns the direction on plane orthogonal to wires where wire number increases
    static std::array<double, 3> GetIncreasingWireDirection
      (const geo::PlaneGeo& plane);

  private:
    geo::GeometryCore const* geom; ///< pointer to geometry service provider

    bool fDisableValidWireIDcheck;  ///< disable test on out-of-world NearestWire()
    std::set<std::string> fNonFatalExceptions;
    std::vector<double> fExpectedWirePitches; ///< wire pitch on each plane
    std::vector<double> fExpectedPlanePitches; ///< plane pitch on each plane
    
  //  std::unique_ptr<details::TestTrackerClassBase> fRunTests; ///< test filter
    // using as pointer just not to have to write the declaration in the header
    testing::NameSelector fRunTests; ///< test filter
    
    void printChannelSummary();
    void printVolBounds();
    void printDetDim();
    void printWirePos();
    void printWiresInTPC(const TPCGeo& tpc, std::string indent = "") const;
    void printAllGeometry() const;
    void testCryostat();
    void testTPC(unsigned int const& c);
    void testChannelToWire();
    void testFindPlaneCenters();
    void testProject();
    void testWireCoordAngle() const;
    void testWirePitch();
    void testPlanePitch();
    void testStandardWirePos();
    void testAPAWirePos();
    void testNearestWire();
    void testWireIntersection() const;
    void testThirdPlane() const;
    void testThirdPlane_dTdW() const;
    void testStepping();

    bool shouldRunTests(std::string test_name) const;
    
    /// Performs the wire intersection test at a single point
    unsigned int testWireIntersectionAt
      (const TPCID& tpcid, double x, double y, double z) const;
    
    /// Returns dT/dW expected from the specified segment A-to-B
    std::vector<std::pair<geo::PlaneID, double>> ExpectedPlane_dTdW(
      std::array<double, 3> const& A, std::array<double, 3> const& B,
      const double driftVelocity = -0.1
      ) const;
    
    /// Performs the third plane slope test with a single configuration
    unsigned int testThirdPlane_dTdW_at
      (std::vector<std::pair<geo::PlaneID, double>> const& plane_dTdW) const;
    
  };
  
  
  namespace details {
    /// Class telling whether a test needs to be run
    class TestTrackerClassBase {
        public:
      using TestList_t = std::set<std::string>;
      
      virtual ~TestTrackerClassBase() = default;
      
      /// Returns whether the specified test should run
      virtual bool ShouldRun(std::string test_name) const = 0;
      
      /// Checks the test and records the request
      bool operator() (std::string test_name);
      
      /// Allow the specified test to run
      virtual void PleaseRunAlso(std::string test_name) = 0;
      
      /// Returns the tests that have been run
      TestList_t const& RunTests() const { return run; }
      
      /// Returns the tests that have been skipped
      TestList_t const& SkippedTests() const { return skipped; }
      
      /// Returns the tests that have been queried
      TestList_t QueriedTests() const;
      
      /// Checks that the validity of the configuration (after the fact)
      virtual bool CheckQueriesRegistry() const;
      
      /// Prints information about the configuration of the filter
      virtual void PrintConfiguration(std::ostream&) const;
      
        protected:
      TestList_t run; ///< requested tests that should be run
      TestList_t skipped; ///< requested tests that should be skipped
      
      virtual void RecordRequest(std::string test_name, bool bRun);
      
      /// Checks the test and records the request
      virtual bool Query(std::string test_name);
      
      /// Adds a vector of tests into a test set
      static void CopyList
        (TestList_t& dest, std::vector<std::string> const& from);
    }; // class TestTrackerClassBase
    
  } // namespace details
  
  
} // namespace geo

#endif // GEO_GEOMETRYTESTALG_H
