/**
 * @file   geometry_standardchannelmapping_test.cxx
 * @brief  Unit test of channel mapping on a standard detector
 * @date   June 26th, 2015
 * @author petrillo@fnal.gov
 * 
 * Usage: just run the executable.
 * Or plug a FHiCL file in the command line.
 */

// Boost test libraries; we want to define this stuff as soon as possible
#define BOOST_TEST_MODULE GeometryStandardChannelMappingTest

// LArSoft libraries
#include "test/Geometry/geometry_unit_test_base.h"
#include "test/Geometry/ChannelMapStandardTestAlg.h"
#include "larcore/TestUtils/boost_unit_test_base.h"
#include "larcore/Geometry/GeometryCore.h"
#include "larcore/Geometry/ChannelMapStandardAlg.h"

// utility libraries
#include "messagefacility/MessageLogger/MessageLogger.h"

// C/C++ standard libraries
#include <string>

//------------------------------------------------------------------------------
//---  The test environment
//---

// we define here all the configuration that is needed;
// in the specific, the type of the channel mapping and a proper test name,
// used for output only; BasicGeometryEnvironmentConfiguration can read the
// configuration file name from command line, and
// BoostCommandLineConfiguration<> makes it initialize in time for Boost
// to catch it when instanciating the fixture.
struct StandardGeometryConfiguration:
  public testing::BoostCommandLineConfiguration<
    testing::BasicGeometryEnvironmentConfiguration<geo::ChannelMapStandardAlg>
    >
{
  /// Constructor: overrides the application name
  StandardGeometryConfiguration()
    { SetApplicationName("GeometryStandardChannelMappingTest"); }
}; // class StandardGeometryConfiguration

/*
 * Our fixture is based on GeometryTesterEnvironment, configured with the object
 * above.
 * It provides to the testing environment:
 * - `Tester()`, returning a configured instance of the test algorithm;
 * - `GlobalTester()`, (static) returning a global configured instance of the
 *   test algorithm.
 * 
 * The testing::TestSharedGlobalResource<> facility provides a singleton
 * instance of the tester algorithm, shared through the whole program and with
 * this object too.
 * 
 * This sharing allows the fixture to be used either as global or as per-suite.
 * In the former case, the BOOST_AUTO_TEST_CASE's will access the global test
 * algotithm instance through the static call to
 * `GeometryStandardChannelMappingTestFixture::GlobalTester()`; in the latter
 * case, it will access the local tester via the member function `Tester()`.
 * In this case, whether `GlobalTester()` and `Tester()` point to the same
 * tester depends on Boost unit test implementation.
 */
class GeometryStandardChannelMappingTestFixture:
  private testing::GeometryTesterEnvironment<StandardGeometryConfiguration>
{
  using Tester_t = geo::ChannelMapStandardTestAlg;
  
  using TesterRegistry_t = testing::TestSharedGlobalResource<Tester_t>;

    public:
  
  /// Constructor: initialize the tester with the Geometry from base class
  GeometryStandardChannelMappingTestFixture()
    {
      // create a new tester
      tester_ptr = std::make_shared<Tester_t>(TesterParameters());
      tester_ptr->Setup(*Geometry()); // Geometry() is inherited
      // if no tester is default yet, share ours:
      TesterRegistry_t::ProvideDefaultSharedResource(tester_ptr);
    }
  
  /// Retrieves the local tester
  Tester_t& Tester() { return *(tester_ptr.get()); }
  
  /// Retrieves the global tester
  static Tester_t& GlobalTester() { return TesterRegistry_t::Resource(); }
  
    private:
  std::shared_ptr<Tester_t> tester_ptr; ///< our tester (may be shared)
}; // class GeometryStandardChannelMappingTestFixture



//------------------------------------------------------------------------------
//---  The tests
//---
//
// Note on Boost fixture options:
// - BOOST_FIXTURE_TEST_SUITE will provide the tester as a always-present data
//   member in the environment, as "Tester()"; but a new fixture, with a new
//   geometry and a new tester, is initialized on each test case
// - BOOST_GLOBAL_FIXTURE does not provide tester access, so one has to get it
//   as GeometryStandardChannelMappingTestFixture::GlobalTester(); on the other
//   hand, the fixture is initialized only when a new global one is explicitly
//   created.
//

// BOOST_FIXTURE_TEST_SUITE(TestEnv, GeometryStandardChannelMappingTestFixture)
BOOST_GLOBAL_FIXTURE(GeometryStandardChannelMappingTestFixture)

/*
BOOST_AUTO_TEST_CASE( AllTests )
{
  GlobalTester().Run();
} // BOOST_AUTO_TEST_CASE( AllTests )
*/

BOOST_AUTO_TEST_CASE( TPCsetMappingTestCase )
{
  GeometryStandardChannelMappingTestFixture::GlobalTester().TPCsetMappingTest();
} // BOOST_AUTO_TEST_CASE( TPCsetMappingTestCase )

BOOST_AUTO_TEST_CASE( ROPMappingTestCase )
{
  GeometryStandardChannelMappingTestFixture::GlobalTester().ROPMappingTest();
} // BOOST_AUTO_TEST_CASE( ROPMappingTestCase )

BOOST_AUTO_TEST_CASE( ChannelMappingTestCase )
{
  GeometryStandardChannelMappingTestFixture::GlobalTester()
    .ChannelMappingTest();
} // BOOST_AUTO_TEST_CASE( ChannelMappingTestCase )


// BOOST_AUTO_TEST_SUITE_END()
