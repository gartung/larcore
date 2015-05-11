/**
 * @file   geometry_unit_test_base.h
 * @brief  Base class for objects initializing a geometry
 * @date   May 7th, 2015
 * @author petrillo@fnal.gov
 * 
 * Provides an environment for easy set up of a Geometry-aware test environment.
 * Keep in mind that, as much as I could push on flexibility, the channel
 * mapping algorithm must be hard-coded and, if using Boost unit test,
 * the configuration file location must be hard coded too
 * (or you can use the provided configuration).
 * 
 * For an example of usage, see larcore/test/Geometry/geometry_iterator_test.cxx
 */


#ifndef TEST_GEOMETRY_UNIT_TEST_BASE_H
#define TEST_GEOMETRY_UNIT_TEST_BASE_H

// LArSoft libraries
#include "Geometry/GeometryCore.h"
#include "Geometry/ChannelMapAlg.h"

// utility libraries
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/intermediate_table.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "fhiclcpp/parse.h"
// #include "fhiclcpp/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// CET libraries
#include "cetlib/filepath_maker.h"

// C/C++ standard libraries
#include <iostream> // for output before message facility is set up
#include <string>
#include <memory> // std::unique_ptr<>


namespace testing {
  
  /**
   * @brief Class holding a configuration for a fixture
   * @tparam CHANNELMAP the class used for channel mapping
   * @see GeometryTesterFixture
   *
   * This class needs to be fully constructed by the default constructor
   * in order to be useful as Boost unit test fixture.
   * It is supposed to be passed as a template parameter to another class
   * that can store an instance of it and extract configuration information
   * from it.
   */
  template <typename CHANNELMAP>
  struct BasicGeometryFixtureConfigurer {
    using ChannelMapClass = CHANNELMAP;
    
    /// Default constructor; this is what is used in Boost unit test
    BasicGeometryFixtureConfigurer()
      {
        SetApplicationName("GeometryTest");
        SetGeometryParameterSetPath("services.Geometry");
        SetDefaultGeometryConfiguration(R"(
            services: {
              Geometry: {
                SurfaceY:        200.  # in cm, vertical distance to the surface
                Name:            "lartpcdetector"
                GDML:            "LArTPCdetector.gdml"
                ROOT:            "LArTPCdetector.gdml"
                SortingParameters: {}  # empty parameter set for default
              } # Geometry
            } # services
          )");
      } // BasicGeometryFixtureConfigurer()
    
    /// Constructor; accepts the name as parameter
    BasicGeometryFixtureConfigurer(std::string name):
      BasicGeometryFixtureConfigurer()
      { SetApplicationName(name); }
    
    /// @{
    /// @name Access to configuration
    /// Name of the application
    std::string ApplicationName() const { return appl_name; }
    
    /// Path to the configuration file
    std::string ConfigurationPath() const { return config_path; }
    
    /// FHiCL path for the geometry configuration
    std::string GeometryParameterSetPath() const { return geo_pset; }
    
    /// A string describing the default parameter set to configure geometry
    std::string DefaultGeometryConfiguration() const { return geo_default_cfg; }
    
    /// FHiCL path for the configuration of the test algorithm
    std::string TesterParameterSetPath() const { return test_pset; }
    ///@}
    
    
    /// @{
    /// @name Set configuration
    
    /// Sets the name of the application
    void SetApplicationName(std::string name) { appl_name = name; }
    
    /// Sets the path to the configuration file
    void SetConfigurationPath(std::string path) { config_path = path; }
    
    /// Sets the FHiCL path for the geometry configuration
    void SetGeometryParameterSetPath(std::string path) { geo_pset = path; }
    
    /// Sets a string describing the default parameter set to configure geometry
    void SetDefaultGeometryConfiguration(std::string cfg)
      { geo_default_cfg = cfg; }
    
    /// Sets the FHiCL path for the configuration of the test algorithm
    void SetTesterParameterSetPath(std::string path) { test_pset = path; }
    
    ///@}
    
    
      protected:
    std::string appl_name; ///< name of the application
    std::string config_path; ///< configuration file path
    std::string geo_pset; ///< FHiCL path to geometry configuration
    std::string geo_default_cfg; ///< default geometry configuration as string
    std::string test_pset; ///< FHiCL path to test algorithm configuration
    
  }; // class BasicGeometryFixtureConfigurer<>
  
  
  
  
  
  /** **************************************************************************
   * @brief Environment for a geometry test
   * @tparam ConfigurerClass a class providing compile-time configuration
   * 
   * The test environment is set up on construction.
   * 
   * The environment provides:
   * - Geometry() method to access geometry (as a constant pointer)
   * - Configuration data member with the complete FHiCL configuration
   * - TesterConfiguration data member with the FHiCL configuration for the test
   * 
   * This class or a derived one can be used as global fixture for unit tests
   * that require the presence of geometry (in the form of geo::GeometryCore
   * instance).
   * 
   * Unfortunately Boost does not give any control on the initialization of the
   * object, so everything must be ready to go as hard coded.
   * The Configurer class tries to alleviate that.
   * That is another, small static class that GeometryTesterFixture uses to
   * get its parameters.
   * 
   * The requirements for the Configurer are:
   * - `ChannelMapClass`: concrete type of channel mapping algorithm class
   * - `std::string ApplicationName()`: the application name
   * - `std::string ConfigurationPath()`: path to the configuration file
   * - `std::string GeometryParameterSetPath()`: FHiCL path to the configuration
   *   of the geometry; in art is `"services.Geometry"`
   * - `std::string TesterParameterSetPath()`: FHiCL path to the configuration
   *   of the geometry
   * 
   * A default configuration is available if the `ConfigurationPath()` is empty.
   * 
   * Note that there is no room for polymorphism here since the setup happens
   * on construction.
   * Some methods are declared virtual in order to allow to tweak some steps
   * of the set up, but it's not trivial to create a derived class that works
   * correctly: the derived class must declare a new default constructor,
   * and that default constructor must call the protected constructor
   * (GeometryTesterFixture<ConfigurerClass>(no_setup))
   */
  template <typename ConfigurerClass>
  class GeometryTesterFixture {
      public:
    
    /**
     * @brief Constructor: sets everything up and declares the test started
     * 
     * The configuration is from a default-constructed ConfigurerClass.
     * This is suitable for use as Boost unit test fixture.
     */
    GeometryTesterFixture() { Setup(); }
    
    //@{
    /**
     * @brief Setup from a configuration
     * @param configurer an instance of ConfigurerClass
     * 
     * The configuration is from the specified configurer class.
     * 
     * This constructor allows to use a non-default-constructed configuration.
     * This can't be used (at best of my knowledge) when using this class as
     * Boost unit test fixture.
     * 
     * In the r-value-reference constructor, the configurer is moved.
     */
    GeometryTesterFixture(ConfigurerClass const& cfg_obj):
      Configurer(cfg_obj)
      { Setup(); }
    GeometryTesterFixture(ConfigurerClass&& cfg_obj):
      Configurer(cfg_obj)
      { Setup(); }
    //@}
    
    /// Destructor: closing remarks
    virtual ~GeometryTesterFixture();
    
    
    /// Returns a pointer to the geometry
    geo::GeometryCore const* Geometry() const { return geom.get(); }
    
    fhicl::ParameterSet Configuration; ///< full configuration of the test
    fhicl::ParameterSet TesterConfiguration; ///< configuration of the test
    
    /// Returns the current global geometry instance (may be nullptr)
    static geo::GeometryCore const* GlobalGeometry()
      { return global_geometry.get(); }
    
      protected:
    using ChannelMapClass = typename ConfigurerClass::ChannelMapClass;
    
    typedef struct {} DoNotSetUp_t;
    
    ConfigurerClass Configurer; ///< instance of the configurer
    
    std::shared_ptr<const geo::GeometryCore> geom; ///< pointer to the geometry
    
    /**
     * @brief Constructor: does not perform setup
     * 
     * This constructor is thought for derived classes.
     * A derived class will need to use this constructor for the base class,
     * and then call explicitly the setup in the constructor body.
     * 
     * It's easy to misuse polymorphism in this situation, so be very sure you
     * understand this class and its context of use.
     */
    GeometryTesterFixture(DoNotSetUp_t) {}
    
    
    /// The complete initialization, ran at construction
    virtual void Setup();
    
    /// Reads and translates the configuration
    virtual void Configure();
    
    /// Provides a default configuration
    virtual fhicl::ParameterSet DefaultConfiguration() const;
    
    //@{
    /// Sets up the message facility
    virtual void SetupMessageFacility
      (fhicl::ParameterSet const& pset, std::string appl_name = "") const;
    virtual void SetupMessageFacility() const
      { SetupMessageFacility(Configuration, Configurer.ApplicationName()); }
    //@}
    
    /// Creates a new geometry
    virtual geo::GeometryCore const* CreateNewGeometry() const;
    
    /// Getw ownership of the specified geometry and registers it as global
    void RegisterGeometry(geo::GeometryCore const* new_geom);
    
    /// Sets up the geometry (creates and registers it)
    void SetupGeometry();
    
    /// Parses from file and returns a FHiCL data structure
    static fhicl::ParameterSet ParseConfiguration(std::string config_path);
    
    /// A handy constant for specifying the special constructor
    static DoNotSetUp_t const no_setup;
    
    /// A global instance of geometry
    static std::shared_ptr<const geo::GeometryCore> global_geometry;
    
  }; // class GeometryTesterFixture<>
  
  
  // we are abusing templates here... this should be "extern"
  template <typename ConfigurerClass>
  std::shared_ptr<const geo::GeometryCore>
  GeometryTesterFixture<ConfigurerClass>::global_geometry = nullptr;
  
  template <typename ConfigurerClass>
  typename GeometryTesterFixture<ConfigurerClass>::DoNotSetUp_t const
    GeometryTesterFixture<ConfigurerClass>::no_setup;
  
  
  //****************************************************************************
  template <typename ConfigurerClass>
  GeometryTesterFixture<ConfigurerClass>::~GeometryTesterFixture() {
    
    mf::LogInfo("Test") << Configurer.ApplicationName() << " completed.";
    
  } // GeometryTesterFixture<>::~GeometryTesterFixture()
  
  
  /** **************************************************************************
   * @brief Creates a full configuration for the test
   * @return a parameters set with the complete configuration
   */
  template <typename ConfigurerClass>
  fhicl::ParameterSet
  GeometryTesterFixture<ConfigurerClass>::DefaultConfiguration() const
  {
    // get the default configuration from the configurer
    const std::string GeometryConfigurationString
      = Configurer.DefaultGeometryConfiguration();
    
    fhicl::ParameterSet global_pset;
    fhicl::make_ParameterSet(GeometryConfigurationString, global_pset);
    
    return global_pset;
  } // GeometryTesterFixture<>::DefaultConfiguration()
  
  
  /** **************************************************************************
   * @brief Returns the configuration from a FHiCL file
   * @param config_path full path of the FHiCL configuration file
   * @return a parameters set with the complete configuration from the file
   */
  template <typename ConfigurerClass>
  fhicl::ParameterSet GeometryTesterFixture<ConfigurerClass>::ParseConfiguration
    (std::string config_path)
  {
    // simple file lookup policy: assume the file name specification is complete
    cet::filepath_maker policy;
    
    // parse a configuration file; obtain intermediate form
    fhicl::intermediate_table table;
    fhicl::parse_document(config_path, policy, table);
    
    // translate into a parameter set
    fhicl::ParameterSet global_pset;
    fhicl::make_ParameterSet(table, global_pset);
    
    return global_pset;
  } // GeometryTesterFixture<>::ParseConfiguration()
  
  
  /** **************************************************************************
   * @brief Returns the geometry configuration
   * @param config_path full path of the FHiCL configuration file
   * @return a parameters set with the complete configuration from the file
   * 
   * If config_path is empty, a hard-coded configuration is used.
   */
  template <typename ConfigurerClass>
  void GeometryTesterFixture<ConfigurerClass>::Configure() {
    
    std::string const config_path = Configurer.ConfigurationPath();
    Configuration = config_path.empty()?
      DefaultConfiguration(): ParseConfiguration(config_path);
    
  } // GeometryTesterFixture::Configure()
  
  
  /** **************************************************************************
   * @brief Sets the message facility up
   * 
   * Message facility configuration is expected in "services.message" parameter
   * set. If not there, the default configuration is used.
   */
  template <typename ConfigurerClass>
  void GeometryTesterFixture<ConfigurerClass>::SetupMessageFacility
    (fhicl::ParameterSet const& pset, std::string appl_name /* = "" */) const
  {
    fhicl::ParameterSet mf_pset;
    if (!pset.get_if_present("services.message", mf_pset)) {
      // a destination which will react to all messages from DEBUG up
      std::string MessageFacilityConfiguration = R"(
      destinations : {
        stdout: {
          type:      cout
          threshold: DEBUG
          categories: {
            default: {
              limit: -1
            }
          } // categories
        } // stdout
      } // destinations
      statistics: cout
      )";
      fhicl::make_ParameterSet(MessageFacilityConfiguration, mf_pset);
      std::cout << "Using default message facility configuration:\n"
        << mf_pset.to_indented_string(1) << std::endl;
    } // if no configuration is available
    
    mf::StartMessageFacility(mf::MessageFacilityService::SingleThread, mf_pset);
    if (!appl_name.empty()) mf::SetApplicationName(appl_name);
    mf::SetContext("Initialization");
    mf::LogInfo("MessageFacility") << "MessageFacility started.";
    mf::SetModuleName("main");
  } // GeometryTesterFixture::SetupMessageFacility()
  
  
  /** **************************************************************************
   * @brief Sets the geometry of the standard detector up
   * 
   * This function sets up the geometry according to the provided information:
   * - the configuration must contain enough information to locate the geometry
   *   description file
   * - we trust that that geometry works well with the ChannelMapClass specified
   *   in ConfigurerClass
   * 
   */
  template <typename ConfigurerClass>
  geo::GeometryCore const*
  GeometryTesterFixture<ConfigurerClass>::CreateNewGeometry() const
  {
    
    std::string GeometryParameterSetPath
      = !Configurer.GeometryParameterSetPath().empty()?
      Configurer.GeometryParameterSetPath(): "services.Geometry";
    //
    // create the new geometry service provider
    //
    fhicl::ParameterSet GeoConfig
      = Configuration.get<fhicl::ParameterSet>(GeometryParameterSetPath);
    geo::GeometryCore* new_geom = new geo::GeometryCore(GeoConfig);
    
    std::string RelativePath = GeoConfig.get< std::string>("RelativePath", "");
    
    std::string
      GDMLFileName = RelativePath + GeoConfig.get<std::string>("GDML"),
      ROOTFileName = RelativePath + GeoConfig.get<std::string>("ROOT");
    
    // Search all reasonable locations for the geometry file;
    // we see if by any chance art's FW_SEARCH_PATH directory is set and try
    // there;
    // if not, we do expect the path to be complete enough for ROOT to cope.
    cet::search_path sp("FW_SEARCH_PATH");
    
    std::string ROOTfile;
    if (!sp.find_file(ROOTFileName, ROOTfile)) ROOTfile = ROOTFileName;
    
    // we really don't care of GDML file, since we are not going to run Geant4
    std::string GDMLfile;
    if (!sp.find_file(GDMLFileName, GDMLfile))
      mf::LogWarning("CreateNewGeometry") << "GDML file not found.";
    
    // initialize the geometry with the files we have found
    new_geom->LoadGeometryFile(GDMLfile, ROOTfile);
    
    
    //
    // create the new channel map
    //
    fhicl::ParameterSet SortingParameters
      = GeoConfig.get<fhicl::ParameterSet>("SortingParameters");
    std::shared_ptr<geo::ChannelMapAlg> pChannelMap
      (new ChannelMapClass(SortingParameters));
    
    // connect the channel map with the geometry, that shares ownsership
    // (we give up ours at the end of this method)
    new_geom->ApplyChannelMap(pChannelMap);
    
    return new_geom;
  } // GeometryTesterFixture<>::CreateNewGeometry()
  
  
  template <typename ConfigurerClass>
  void GeometryTesterFixture<ConfigurerClass>::RegisterGeometry
    (geo::GeometryCore const* new_geom)
  {

    // update the current geometry, that becomes owner;
    // also update the global one if it happens to be already our previous
    // (in this case, it becomes co-owner)
    const bool bUpdateGlobal = !global_geometry || (global_geometry == geom);
    geom.reset(new_geom);
    if (bUpdateGlobal) global_geometry = geom;
    
  } // GeometryTesterFixture<>::SetupGeometry()
  
  
  template <typename ConfigurerClass>
  void GeometryTesterFixture<ConfigurerClass>::SetupGeometry() {
    RegisterGeometry(CreateNewGeometry());
  } // GeometryTesterFixture<>::SetupGeometry()
  
  
  
  /** **************************************************************************
   * @brief Performs the complete set up
   * @param ConfigurationFilePath path to the FHiCL configuration file
   * @param GeometryTestParameterSetPath FHiCL path to the configuration of the
   *        geometry test (default: `physics.analysers.geotest`)
   * @param GeometryParameterSetPath FHiCL path to the configuration of the
   *        geometry (default: `services.Geometry`)
   */
  template <typename ConfigurerClass>
  void GeometryTesterFixture<ConfigurerClass>::Setup(
  ) {
    
    //
    // get the configuration
    //
    Configure();
    
    //
    // set up the message facility
    //
    SetupMessageFacility(Configuration, "GeometryIterator_test");
    
    //
    // Optionally print the configuration
    //
    mf::LogInfo("Configuration") << "Complete configuration:\n"
      << Configuration.to_indented_string(1);
    
    //
    // set up the geometry
    //
    SetupGeometry();
    
    //
    // save the specific configuration of the test
    //
    std::string const GeometryTestParameterSetPath
      = Configurer.TesterParameterSetPath();
    if (!GeometryTestParameterSetPath.empty()) {
      TesterConfiguration = Configuration.get<fhicl::ParameterSet>
        (GeometryTestParameterSetPath, fhicl::ParameterSet());
    }
    
    mf::LogInfo("Test") << Configurer.ApplicationName() << " setup complete.";
    
  } // GeometryTesterFixture<>::Setup()
  
  
} // namespace testing

#endif // TEST_GEOMETRY_UNIT_TEST_BASE_H
