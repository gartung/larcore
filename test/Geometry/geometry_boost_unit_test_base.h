/**
 * @file   geometry_boost_unit_test_base.h
 * @brief  Base class for geometry tests using Boost unit test library
 * @date   May 22th, 2015
 * @author petrillo@fnal.gov
 * 
 * Provides an environment for easy set up of a Geometry-aware test.
 * Keep in mind that, as much as I could push on flexibility, the channel
 * mapping algorithm must be hard-coded and, using Boost unit test,
 * the configuration file location must be hard coded too
 * (or you can specify a file as first parameter in the command line, or use the
 * configuration provided by default, which is hard-coded).
 * 
 * For an example of usage, see larcore/test/Geometry/geometry_iterator_test.cxx
 */


#ifndef TEST_GEOMETRY_BOOST_UNIT_TEST_BASE_H
#define TEST_GEOMETRY_BOOST_UNIT_TEST_BASE_H

// LArSoft libraries
#include "test/Geometry/geometry_unit_test_base.h"

// Boost libraries
#include <boost/test/unit_test.hpp> // framework::master_test_suite()

// C/C++ standard libraries
#include <string>


namespace testing {
  
  /** **************************************************************************
   * @brief Class holding a configuration for a Boost test fixture
   * @tparam CONFIGURATIONCLASS a base configuration class
   * @see BasicGeometryEnvironmentConfiguration, GeometryTesterEnvironment
   *
   * This class needs to be fully constructed by the default constructor
   * in order to be useful as Boost unit test fixture.
   * It is supposed to be passed as a template parameter to another class
   * that can store an instance of it and extract configuration information
   * from it.
   * 
   * This template just adds to the standard construction of the wrapped class
   * a configuration that reads the parameters from the command line.
   */
  template <typename CONFIGURATIONCLASS>
  struct BoostCommandLineConfiguration: public CONFIGURATIONCLASS 
  {
    using Base_t = CONFIGURATIONCLASS;
    
    /// Default constructor; this is what is used in Boost unit test
    BoostCommandLineConfiguration(): Base_t()
      { ParseCommandLineFromBoost(); }
    
    /// Constructor; accepts the name as parameter
    BoostCommandLineConfiguration(std::string name): Base_t(name)
      { ParseCommandLineFromBoost(); }
    
      protected:
    
    /// Parses arguments as delivered by Boost
    void ParseCommandLineFromBoost()
      {
        Base_t::ParseCommandLine(
          boost::unit_test::framework::master_test_suite().argc,
          boost::unit_test::framework::master_test_suite().argv
          );
      }
    
  }; // class BoostCommandLineConfiguration<>
  
  
  
  /** **************************************************************************
   * @brief Environment for a shared geometry test
   * @tparam ConfigurationClass a class providing compile-time configuration
   * 
   * This class is derived from GeometryTesterEnvironment.
   * 
   * It redefines the way the geometry is created: a new instance will reuse
   * the default geometry if it exists already.
   * This is suited for Boost unit test fixtures for multiple test suites,
   * when the caller does not want to reinitialize the geometry on each suite,
   * accepting that there will always be only one and the same geometry.
   * 
   * The environment provides just everything the base class does;
   * the requirements are also the same as for the base class.
   */
  template <typename ConfigurationClass>
  class SharedGeometryTesterEnvironment:
    public GeometryTesterEnvironment<ConfigurationClass>
  {
    using Base_t = GeometryTesterEnvironment<ConfigurationClass>;
    using SharedGeoPtr_t = typename Base_t::SharedGeoPtr_t;
    
      public:
    
    //@{
    /// Reimplement the constructors to delay Setup(),
    /// so that it's aware of the overridden virtual functions
    SharedGeometryTesterEnvironment(bool bSetup = true): Base_t(false)
      { if (bSetup) Base_t::Setup(); }
    SharedGeometryTesterEnvironment
      (ConfigurationClass const& cfg_obj, bool bSetup = true):
      Base_t(cfg_obj, false)
      { if (bSetup) Base_t::Setup(); }
    SharedGeometryTesterEnvironment(ConfigurationClass&& cfg_obj, bool bSetup = true):
      Base_t(cfg_obj, false)
      { if (bSetup) Base_t::Setup(); }
    //@}
    
    
    /// Creates a new geometry
    virtual SharedGeoPtr_t CreateNewGeometry() const override
      {
        SharedGeoPtr_t geo_ptr = Base_t::SharedGlobalGeometry();
        return geo_ptr? geo_ptr: Base_t::CreateNewGeometry();
      }
    
  }; // class SharedGeometryTesterEnvironment<>
  
  
} // namespace testing

#endif // TEST_GEOMETRY_BOOST_UNIT_TEST_BASE_H
