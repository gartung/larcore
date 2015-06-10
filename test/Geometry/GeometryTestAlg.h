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
   *   + `ThirdPlane` (default): tests `ThirdPlane()`
   *   + `ThirdPlaneSlope` (default): tests `ThirdPlaneSlope()`
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
    
    /// Virtual destructor
    virtual ~GeometryTestAlg() = default;
    
    /// Runs the test
    virtual void Setup(geo::GeometryCore const& new_geo) { geom = &new_geo; }

    /// Runs the test, returns a number of errors (very unlikely!)
    virtual unsigned int Run();

    /// Returns the direction on plane orthogonal to wires where wire number increases
    static std::array<double, 3> GetIncreasingWireDirection
      (const geo::PlaneGeo& plane);

    static const std::vector<std::string> DefaultTests;

  private:
    geo::GeometryCore const* geom; ///< pointer to geometry service provider

    bool fDisableValidWireIDcheck;  ///< disable test on out-of-world NearestWire()
    std::set<std::string> fNonFatalExceptions;
    std::set<std::string> fRunTests; ///< which tests to run (empty runs all)
    std::vector<double> fExpectedWirePitches; ///< wire pitch on each plane
    std::vector<double> fExpectedPlanePitches; ///< plane pitch on each plane
    
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
} // namespace geo

#endif // GEO_GEOMETRYTESTALG_H
