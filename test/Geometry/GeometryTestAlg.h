/**
 * @file   GeometryTestAlg.h
 * @brief  Unit test for geometry functionalities
 * @date   2011/02/17
 * @author brebel@fnal.gov
 * @see    GeometryTestAlg.cxx
 * 
 * Refactored by Gianluca Petrillo on May 5th, 2015.
 */

// LArSoft includes
#include "SimpleTypesAndConstants/geo_types.h"

// C/C++ standard libraries
#include <string>
#include <set>
#include <vector>
#include <array>

// forward declarations
namespace fhicl {
  class ParameterSet;
}


namespace geo {
  
  // forward declarations
  class GeometryCore;
  class TPCGeo;
  class PlaneGeo;
  
  
  /** **************************************************************************
   * @brief Performs tests on the geometry as seen by Geometry service
   * 
   * Configuration parameters
   * =========================
   * 
   * - **DisableWireBoundaryCheck** (boolean, default: false): the exceptions
   *   thrown when checking wire boundaries are not fatal
   * - **ForgiveExceptions** (list of strings, default: empty): the categories
   *   of exceptions in this list are "forgiven" (non-fatal)
   * - **RunTests** (string list): marks which tests to run;
   *   if the list is empty, all default tests (marked below as such) are run:
   *   + `CheckOverlaps` perform overlap checks
   *   + `Cryostat` (default):
   *   + `ChannelToWire` (default):
   *   + `FindPlaneCenters` (default):
   *   + `Projection` (default):
   *   + `WirePos`: currently disabled
   *   + `NearestWire` (default): tests `WireCoordinate()` and `NearestWire()`
   *   + `WireIntersection` (default): tests `WireIDsIntersect()`
   *   + `WirePitch` (default):
   *   + `PlanePitch` (default):
   *   + `Stepping` (default):
   *   + `PrintWires`: prints *all* the wires in the geometry
   * - **CheckForOverlaps** (boolean, default: false): equivalent to enabling
   *   `CheckOverlaps` in `RunTests`
   * - **PrintWires**: (boolean, default: false): equivalent to enabling
   *   `PrintWires` in `RunTests`
   */
  class GeometryTestAlg {
      public:
    explicit GeometryTestAlg(fhicl::ParameterSet const& pset);
    
    /// Runs the test
    void Configure(geo::GeometryCore const* pGeom) { geom = pGeom; }

    /// Runs the test, returns a number of errors (very unlikely!)
    unsigned int Run();

    /// Returns the direction on plane orthogonal to wires where wire number increases
    static std::array<double, 3> GetIncreasingWireDirection
      (const geo::PlaneGeo& plane);

    static const std::vector<std::string> DefaultTests;

  private:
    geo::GeometryCore const* geom; ///< pointer to geometry service provider

    bool fDisableValidWireIDcheck;  ///< disable test on out-of-world NearestWire()
    std::set<std::string> fNonFatalExceptions;
    std::set<std::string> fRunTests; ///< which tests to run (empty runs all)
    
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
    void testWirePitch();
    void testPlanePitch();
    void testStandardWirePos();
    void testAPAWirePos();
    void testNearestWire();
    void testWireIntersection() const;
    void testStepping();

    bool shouldRunTests(std::string test_name) const;
    
    /// Performs the wire intersection test at a single point
    unsigned int testWireIntersectionAt
      (const TPCID& tpcid, double x, double y, double z) const;
  };
} // namespace geo
