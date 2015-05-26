/**
 * @file   geometry_unit_test_base.h
 * @brief  Base class for objects initializing a geometry
 * @date   May 7th, 2015
 * @author petrillo@fnal.gov
 * 
 * Provides an environment for easy set up of a Geometry-aware test.
 * Keep in mind that, as much as I could push on flexibility, the channel
 * mapping algorithm must be hard-coded and, if using Boost unit test,
 * the configuration file location must be hard coded too
 * (or you can use the provided configuration).
 * 
 * For an example of usage, see larcore/test/Geometry/geometry_iterator_test.cxx
 * 
 * Currently provides:
 * - BasicGeometryEnvironmentConfiguration: a test environment configuration
 * - TestSharedGlobalResource: mostly internal use
 * - GeometryTesterEnvironment: a prepacked geometry-aware test environment
 * 
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
#include "cetlib/filesystem.h" // cet::is_absolute_filepath()
#include "cetlib/filepath_maker.h"
#include "cetlib/search_path.h"

// C/C++ standard libraries
#include <iostream> // for output before message facility is set up
#include <string>
#include <memory> // std::unique_ptr<>
#include <map>


namespace testing {
  
  namespace details {
    
    /// Reads and makes available the command line parameters
    class CommandLineArguments {
        public:
      /// Constructor: automatically parses from Boost arguments
      CommandLineArguments() { Clear(); }
    
      /// Constructor: parses from specified arguments
      CommandLineArguments(int argc, char** argv)
        { ParseArguments(argc, argv); }
    
      /// Parses arguments
      void ParseArguments(int argc, char** argv);
      
      /// Returns the name of the executable as started
      std::string Executable() const { return exec_name; }
      
      /// Returns the list of non-Boost-test arguments on the command line
      std::vector<std::string> const& Arguments() const { return args; }
      
      /// Returns whether we have arguments up to the iArg-th (0-based)
      bool hasArgument(size_t iArg) const { return iArg < args.size(); }
      
      /// Returns the value of the iArg-th (0-based; no range check!)
      std::string const& Argument(size_t iArg) const { return args[iArg]; }
      
        private:
      std::string exec_name; ///< name of the test executable (from argv[0])
      std::vector<std::string> args; ///< command line arguments (from argv[0])
      
      /// Erases the stored arguments
      void Clear() { exec_name.clear(); args.clear(); }

    }; // class CommandLineArguments
    
    
    void CommandLineArguments::ParseArguments(int argc, char** argv) {
      Clear();
      if (argc == 0) return;
      
      exec_name = argv[0];
      
      args.resize(argc - 1);
      std::copy(argv + 1, argv + argc, args.begin());
      
    } // CommandLineArguments:ParseArguments()
    
  } // namespace details
  
  
  /** **************************************************************************
   * @brief Class holding a configuration for a test environment
   * @tparam CHANNELMAP the class used for channel mapping
   * @see GeometryTesterEnvironment
   *
   * This class needs to be fully constructed by the default constructor
   * in order to be useful as Boost unit test fixture.
   * It is supposed to be passed as a template parameter to another class
   * that can store an instance of it and extract configuration information
   * from it.
   */
  template <typename CHANNELMAP>
  struct BasicGeometryEnvironmentConfiguration {
    using ChannelMapClass = CHANNELMAP;
    
    /// Default constructor; this is what is used in Boost unit test
    BasicGeometryEnvironmentConfiguration() { DefaultInit(); }
    
    /// Constructor: acquires parameters from the command line
    BasicGeometryEnvironmentConfiguration(int argc, char** argv):
      BasicGeometryEnvironmentConfiguration()
      { ParseCommandLine(argc, argv); }
    
    /// Constructor; accepts the name as parameter
    BasicGeometryEnvironmentConfiguration(std::string name):
      BasicGeometryEnvironmentConfiguration()
      { SetApplicationName(name); }
    
    BasicGeometryEnvironmentConfiguration
      (int argc, char** argv, std::string name):
      BasicGeometryEnvironmentConfiguration(argc, argv)
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
    
    /// A string describing the default parameter set to configure the test
    std::string DefaultTesterConfiguration() const { return test_default_cfg; }
    
    /// Returns the name of the executable as started
    std::string ExecutablePath() const { return arguments.Executable(); }
    
    /// Returns the list of non-Boost-test arguments on the command line
    std::vector<std::string> const& EexcutableArguments() const
      { return arguments.Arguments(); }
    
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
    
    /// Sets a string describing the default parameter set to configure the test
    void SetDefaultTesterConfiguration(std::string cfg)
      { test_default_cfg = cfg; }
    
    ///@}
    
    
      protected:
    std::string appl_name; ///< name of the application
    std::string config_path; ///< configuration file path
    std::string geo_pset; ///< FHiCL path to geometry configuration
    std::string geo_default_cfg; ///< default geometry configuration as string
    std::string test_pset; ///< FHiCL path to test algorithm configuration
    std::string test_default_cfg; ///< default test configuration as string
    
    /// Extracts arguments from the command line, uses first one as config path
    void ParseCommandLine(int argc, char** argv)
      {
        arguments.ParseArguments(argc, argv);
        if (arguments.hasArgument(0))
          SetConfigurationPath(arguments.Argument(0)); // first argument
      }
    
    /// Initialize with some default values
    void DefaultInit()
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
        SetTesterParameterSetPath("physics.analyzers.geotest");
        SetDefaultTesterConfiguration(R"(
          physics: {
            analyzers: {
              geotest: {}
            } # analyzers
          } # physics
          )");
      } // DefaultInit()
    
      private:
    details::CommandLineArguments arguments; ///< command line arguments
    
  }; // class BasicGeometryEnvironmentConfiguration<>
  
  
  
  /** **************************************************************************
   * @brief Utility class providing singleton objects to the derived classes
   * @tparam RES the type of object (include constantness if needed)
   * 
   * The object is expected to be shared.
   */
  template <typename RES>
  class TestSharedGlobalResource {
     using Resource_t = RES;
     
       public:
     using ResourcePtr_t = std::shared_ptr<Resource_t>;
     
     /// @name Add and share resources
     /// @{
     
     /// Adds a shared resource to the resource registry
     static void AddSharedResource(std::string res_name, ResourcePtr_t res_ptr)
       { Resources.emplace(res_name, res_ptr); }
     
     /// Adds a shared resource to the resource registry (empty name)
     static void AddDefaultSharedResource(ResourcePtr_t res_ptr)
       { AddSharedResource(std::string(), res_ptr); }
     
     /// Registers a shared resource only if none exists yet
     template <typename... Args>
     static ResourcePtr_t ProvideSharedResource
       (std::string res_name, ResourcePtr_t res_ptr)
       {
         if (hasResource(res_name)) return ResourcePtr_t();
         AddSharedResource(res_name, res_ptr);
         return res_ptr;
       }
     
     /// Creates a shared resource as default only if none exists yet
     template <typename... Args>
     static ResourcePtr_t ProvideDefaultSharedResource(ResourcePtr_t res_ptr)
       { return ProvideSharedResource(std::string(), res_ptr); }
     
     //@{
     /// Adds a shared resource only if it is old_res_ptr
     static bool ReplaceSharedResource(
       std::string res_name,
       Resource_t const* old_res_ptr, ResourcePtr_t res_ptr
       )
       {
         ResourcePtr_t current_res_ptr = ShareResource();
         if (current_res_ptr.get() != old_res_ptr) return false;
         AddSharedResource(res_name, res_ptr);
         return true;
       }
     static bool ReplaceSharedResource
       (std::string res_name, ResourcePtr_t old_res_ptr, ResourcePtr_t res_ptr)
       { return ReplaceSharedResource(res_name, old_res_ptr.get(), res_ptr); }
     //@}
     
     //@{
     /// Adds a shared resource as default resource only if it is old_res_ptr
     static bool ReplaceDefaultSharedResource
       (Resource_t const* old_res_ptr, ResourcePtr_t res_ptr)
       { return ReplaceSharedResource(std::string(), old_res_ptr, res_ptr); }
     static bool ReplaceDefaultSharedResource
       (ResourcePtr_t old_res_ptr, ResourcePtr_t res_ptr)
       { return ReplaceSharedResource(std::string(), old_res_ptr, res_ptr); }
     //@}
     
     /// Constructs and registers a new resource with a specified name
     template <typename... Args>
     static ResourcePtr_t CreateResource(std::string res_name, Args&&... args)
       {
         ResourcePtr_t res_ptr(new Resource_t(std::forward<Args>(args)...));
         AddSharedResource(res_name, res_ptr);
         return res_ptr;
       }
     
     /// Constructs and registers a new resource with no name
     template <typename... Args>
     static void CreateDefaultResource(Args&&... args)
       { CreateResource(std::string(), std::forward<Args>(args)...); }
     
     
     /// Creates a shared resource only if none exists yet
     template <typename... Args>
     static ResourcePtr_t ProposeSharedResource
       (std::string res_name, Args&&... args)
       {
         return hasResource(res_name)?
           ResourcePtr_t():
           CreateResource(res_name, std::forward<Args>(args)...);
       }
     
     /// Creates a shared resource as default only if none exists yet
     template <typename... Args>
     static ResourcePtr_t ProposeDefaultSharedResource(Args&&... args)
       {
         return ProposeSharedResource
           (std::string(), std::forward<Args>(args)...);
       }
     
     /// @}
     
     /// @name Resource access
     /// @{
     
     /// Returns whether a resource exists
     /// @throws std::out_of_range if not available
     static bool hasResource(std::string name = "")
       {
         auto iRes = Resources.find(name);
         return (iRes != Resources.end()) && bool(iRes->second);
       }
     
     /// Retrieves the specified resource for sharing (nullptr if none)
     static ResourcePtr_t ShareResource(std::string name = "")
       {
         auto iRes = Resources.find(name);
         return (iRes == Resources.end())? ResourcePtr_t(): iRes->second;
       }
     
     /// Retrieves the specified resource, or throws if not available
     static Resource_t& Resource(std::string name = "")
       { return *(Resources.at(name).get()); }
     
     /// @}
     
     /// Destroys the specified resource (does nothing if no such resource)
     static Resource_t& DestroyResource(std::string name = "")
       { Resources.erase(name); }
     
       private:
     static std::map<std::string, ResourcePtr_t> Resources;
     
  }; // class TestSharedGlobalResource<>
  
  
  template <typename RES>
  std::map<std::string, typename TestSharedGlobalResource<RES>::ResourcePtr_t>
  TestSharedGlobalResource<RES>::Resources;
  
  
  /** **************************************************************************
   * @brief Environment for a geometry test
   * @tparam ConfigurationClass a class providing compile-time configuration
   * 
   * The test environment is set up on construction.
   * 
   * The environment provides:
   * - Geometry() method to access geometry (as a constant pointer)
   * - Parameters() method returning the complete FHiCL configuration
   * - TesterParameters() method returning the configuration for the test
   * 
   * This class or a derived one can be used as global fixture for unit tests
   * that require the presence of geometry (in the form of geo::GeometryCore
   * instance).
   * 
   * Unfortunately Boost does not give any control on the initialization of the
   * object, so everything must be ready to go as hard coded.
   * The ConfigurationClass class tries to alleviate that.
   * That is another, small static class that GeometryTesterEnvironment uses to
   * get its parameters.
   * 
   * The requirements for the ConfigurationClass are:
   * - `ChannelMapClass`: concrete type of channel mapping algorithm class
   * - `std::string ApplicationName()`: the application name
   * - `std::string ConfigurationPath()`: path to the configuration file
   * - `std::string GeometryParameterSetPath()`: FHiCL path to the configuration
   *   of the geometry; in art is `"services.Geometry"`
   * - `std::string TesterParameterSetPath()`: FHiCL path to the configuration
   *   of the geometry
   * - `std::string DefaultGeometryConfiguration()` returning a FHiCL string
   *   to be parsed to extract the default geometry configuration
   * - `std::string DefaultTesterConfiguration()` returning a FHiCL string
   *   to be parsed to extract the default test configuration
   * 
   * Whether the configuration comes from a file or from the two provided
   * defaults, it is always expected within the parameter set paths:
   * the default configuration must also contain that path.
   * 
   * Note that there is no room for polymorphism here since the setup happens
   * on construction.
   * Some methods are declared virtual in order to allow to tweak some steps
   * of the set up, but it's not trivial to create a derived class that works
   * correctly: the derived class must declare a new default constructor,
   * and that default constructor must call the protected constructor
   * (GeometryTesterEnvironment<ConfigurationClass>(no_setup))
   */
  template <typename ConfigurationClass>
  class GeometryTesterEnvironment: private details::CommandLineArguments {
    
    /// this implements the singleton interface
    using GeoResources_t = TestSharedGlobalResource<geo::GeometryCore const>;
    
      public:
    using SharedGeoPtr_t = GeoResources_t::ResourcePtr_t;
    
    /**
     * @brief Constructor: sets everything up and declares the test started
     * 
     * The configuration is from a default-constructed ConfigurationClass.
     * This is suitable for use as Boost unit test fixture.
     */
    GeometryTesterEnvironment(bool bSetup = true) { if (bSetup) Setup(); }
    
    //@{
    /**
     * @brief Setup from a configuration
     * @param configurer an instance of ConfigurationClass
     * 
     * The configuration is from the specified configurer class.
     * 
     * This constructor allows to use a non-default-constructed configuration.
     * This can't be used (at best of my knowledge) when using this class as
     * Boost unit test fixture.
     * 
     * In the r-value-reference constructor, the configurer is moved.
     */
    GeometryTesterEnvironment
      (ConfigurationClass const& cfg_obj, bool bSetup = true):
      config(cfg_obj)
      { if (bSetup) Setup(); }
    GeometryTesterEnvironment(ConfigurationClass&& cfg_obj, bool bSetup = true):
      config(cfg_obj)
      { if (bSetup) Setup(); }
    //@}
    
    /// Destructor: closing remarks
    virtual ~GeometryTesterEnvironment();
    
    
    //@{
    /// Returns a pointer to the geometry
    geo::GeometryCore const* Geometry() const { return geom.get(); }
    SharedGeoPtr_t SharedGeometry() const { return geom; }
    //@}
    
    /// Returns the full configuration
    fhicl::ParameterSet const& Parameters() const { return geometry_params; }
    
    /// Returns the configuration of the test
    fhicl::ParameterSet const& TesterParameters() const
      { return tester_params; }
    
    
    /// Returns the current global geometry instance
    /// @throws std::out_of_range if not present
    static geo::GeometryCore const* GlobalGeometry()
      { return &GeoResources_t::Resource(); }
    
    /// Returns the current global geometry instance (may be nullptr if none)
    static SharedGeoPtr_t SharedGlobalGeometry()
      { return GeoResources_t::ShareResource(); }
    
    
      protected:
    
    using ChannelMapClass = typename ConfigurationClass::ChannelMapClass;
    
    
    /// The complete initialization, ran at construction by default
    virtual void Setup();
    
    /// Reads and translates the configuration
    virtual void Configure();
    
    /// Provides a default configuration
    virtual fhicl::ParameterSet DefaultParameters() const;
    
    //@{
    /// Sets up the message facility
    virtual void SetupMessageFacility
      (fhicl::ParameterSet const& pset, std::string appl_name = "") const;
    virtual void SetupMessageFacility() const
      { SetupMessageFacility(Parameters(), config.ApplicationName()); }
    //@}
    
    /// Creates a new geometry
    virtual SharedGeoPtr_t CreateNewGeometry() const;
    
    //@{
    /// Get ownership of the specified geometry and registers it as global
    virtual void RegisterGeometry(SharedGeoPtr_t new_geom);
    virtual void RegisterGeometry(geo::GeometryCore const* new_geom)
      { RegisterGeometry(SharedGeoPtr_t(new_geom)); }
    //@}
    
    /// Sets up the geometry (creates and registers it)
    virtual void SetupGeometry();
    
    /// Fills the test configuration from file or from default
    void ExtractTestParameters();
    
    /// Creates a full configuration for the test
    virtual fhicl::ParameterSet DefaultTestParameters() const;
    
    /**
     * @brief Fills the test configuration from file or from default
     * 
     * If a FHiCL configuration file is specified, the configuration of the test
     * is read from it according to the parameter set path of the test.
     * Otherwise, it is parsed from the default one provided by the configurer.
     */
    /// Parses from file and returns a FHiCL data structure
    static fhicl::ParameterSet ParseParameters(std::string config_path);
    
      private:
    
    void FillArgumentsFromCommandLine();
    
    ConfigurationClass config; ///< instance of the configurer
    
    SharedGeoPtr_t geom; ///< pointer to the geometry
    
    fhicl::ParameterSet geometry_params; ///< full configuration of the test
    fhicl::ParameterSet tester_params; ///< configuration of the test
    
  }; // class GeometryTesterEnvironment<>
  
  
  
  //****************************************************************************
  namespace details {
    // Class to implement FHiCL file search.
    // This is badly ripped off from ART, but we need to stay out of it
    // so I have to replicate that functionality.
    // I used the same class name.
    class FirstAbsoluteOrLookupWithDotPolicy: public cet::filepath_maker {
        public:
      FirstAbsoluteOrLookupWithDotPolicy(std::string const& paths):
        first(true), after_paths(paths)
        {}
      
      virtual std::string operator() (std::string const& filename);
      
      void reset() { first = true; }
      
        private:
      bool first; ///< whether we are waiting for the first query
      cet::search_path after_paths; ///< path for the other queries
      
    }; // class FirstAbsoluteOrLookupWithDotPolicy
  
  
    std::string FirstAbsoluteOrLookupWithDotPolicy::operator()
      (std::string const &filename)
    {
      if (first) {
        first = false;
        if (cet::is_absolute_filepath(filename)) return filename;
        return cet::search_path("./:" + after_paths.to_string())
          .find_file(filename);
      } else {
        return after_paths.find_file(filename);
      }
    } // FirstAbsoluteOrLookupWithDotPolicy::operator()
  } // namespace details
  
  
  //****************************************************************************
  template <typename ConfigurationClass>
  GeometryTesterEnvironment<ConfigurationClass>::~GeometryTesterEnvironment() {
    
    mf::LogInfo("Test") << config.ApplicationName() << " completed.";
    
  } // GeometryTesterEnvironment<>::~GeometryTesterEnvironment()
  
  
  /** **************************************************************************
   * @brief Creates a full configuration for the test
   * @return a parameters set with the complete configuration
   */
  template <typename ConfigurationClass>
  fhicl::ParameterSet
  GeometryTesterEnvironment<ConfigurationClass>::DefaultParameters() const {
    // get the default configuration from the configurer
    const std::string GeometryConfigurationString
      = config.DefaultGeometryConfiguration();
    
    fhicl::ParameterSet global_pset;
    fhicl::make_ParameterSet(GeometryConfigurationString, global_pset);
    
    return global_pset;
  } // GeometryTesterEnvironment<>::DefaultParameters()
  
  
  /** **************************************************************************
   * @brief Returns the configuration from a FHiCL file
   * @param config_path full path of the FHiCL configuration file
   * @return a parameters set with the complete configuration from the file
   */
  template <typename ConfigurationClass>
  fhicl::ParameterSet
  GeometryTesterEnvironment<ConfigurationClass>::ParseParameters
    (std::string config_path)
  {
    // configuration file lookup policy
    char const* fhicl_env = getenv("FHICL_FILE_PATH");
    std::string search_path = fhicl_env? std::string(fhicl_env) + ":": ".:";
    details::FirstAbsoluteOrLookupWithDotPolicy policy(search_path);
    
    // parse a configuration file; obtain intermediate form
    fhicl::intermediate_table table;
    fhicl::parse_document(config_path, policy, table);
    
    // translate into a parameter set
    fhicl::ParameterSet global_pset;
    fhicl::make_ParameterSet(table, global_pset);
    
    return global_pset;
  } // GeometryTesterEnvironment<>::ParseParameters()
  
  
  /** **************************************************************************
   * @brief Fills the configuration
   * 
   * The complete configuration (message facility and services) is read and
   * saved, hence accessible by Parameters() method.
   * 
   * The configuration file path is taken by default from the first argument
   * of the test.
   * If that first argument is not present or empty, the default configuration
   * path is received from the configurer.
   * If the configuration path is still empty, a hard-coded configuration
   * is used; otherwise, the FHiCL file specified in that path is parsed and
   * used as full configuration.
   */
  template <typename ConfigurationClass>
  void GeometryTesterEnvironment<ConfigurationClass>::Configure() {
    std::string config_path = config.ConfigurationPath();
    geometry_params = config_path.empty()?
      DefaultParameters(): ParseParameters(config_path);
  } // GeometryTesterEnvironment::Configure()
  
  
  /** **************************************************************************
   * @brief Sets the message facility up
   * 
   * Message facility configuration is expected in "services.message" parameter
   * set. If not there, the default configuration is used.
   */
  template <typename ConfigurationClass>
  void GeometryTesterEnvironment<ConfigurationClass>::SetupMessageFacility
    (fhicl::ParameterSet const& pset, std::string appl_name /* = "" */) const
  {
    fhicl::ParameterSet mf_pset;
    if (!pset.get_if_present("services.message", mf_pset)) {
      // a destination which will react to all messages from DEBUG up
      std::string MessageFacilityConfiguration = R"(
      debugModules: [ '*' ]
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
  //  mf::LogProblem("MessageFacility") << "Error messages are shown.";
  //  mf::LogPrint("MessageFacility") << "Warning messages are shown.";
  //  mf::LogVerbatim("MessageFacility") << "Info messages are shown.";
  //  mf::LogTrace("MessageFacility") << "Debug messages are shown.";
  //  LOG_TRACE("MessageFacility")
  //    << "LOG_TRACE/LOG_DEBUG messages are not compiled away.";
    mf::LogInfo("MessageFacility") << "MessageFacility started.";
    mf::SetModuleName("main");
  } // GeometryTesterEnvironment::SetupMessageFacility()
  
  
  /** **************************************************************************
   * @brief Sets the geometry of the standard detector up
   * 
   * This function sets up the geometry according to the provided information:
   * - the configuration must contain enough information to locate the geometry
   *   description file
   * - we trust that that geometry works well with the ChannelMapClass specified
   *   in ConfigurationClass
   * 
   */
  template <typename ConfigurationClass>
  typename GeometryTesterEnvironment<ConfigurationClass>::SharedGeoPtr_t
  GeometryTesterEnvironment<ConfigurationClass>::CreateNewGeometry() const
  {
    
    std::string GeometryParameterSetPath
      = !config.GeometryParameterSetPath().empty()?
      config.GeometryParameterSetPath(): "services.Geometry";
    //
    // create the new geometry service provider
    //
    fhicl::ParameterSet GeoConfig
      = Parameters().template get<fhicl::ParameterSet>
        (GeometryParameterSetPath);
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
    fhicl::ParameterSet SortingParameters = GeoConfig.get<fhicl::ParameterSet>
      ("SortingParameters", fhicl::ParameterSet());
    std::shared_ptr<geo::ChannelMapAlg> pChannelMap
      (new ChannelMapClass(SortingParameters));
    
    // connect the channel map with the geometry, that shares ownsership
    // (we give up ours at the end of this method)
    new_geom->ApplyChannelMap(pChannelMap);
    
    return SharedGeoPtr_t(new_geom);
  } // GeometryTesterEnvironment<>::CreateNewGeometry()
  
  
  template <typename ConfigurationClass>
  void GeometryTesterEnvironment<ConfigurationClass>::RegisterGeometry
    (SharedGeoPtr_t new_geom)
  {
    // update the current geometry, that becomes owner;
    // also update the global one if it happens to be already our previous
    // (in this case, it becomes co-owner)
    SharedGeoPtr_t my_old_geom = geom;
    geom = new_geom;
    // if the global geometry is already the one we register, don't bother
    if (SharedGlobalGeometry() != new_geom)
      GeoResources_t::ReplaceDefaultSharedResource(my_old_geom, new_geom);
  } // GeometryTesterEnvironment<>::RegisterGeometry()
  
  
  
  template <typename ConfigurationClass>
  void GeometryTesterEnvironment<ConfigurationClass>::SetupGeometry() {
    RegisterGeometry(CreateNewGeometry());
  } // GeometryTesterEnvironment<>::SetupGeometry()
  
  
  
  template <typename ConfigurationClass>
  fhicl::ParameterSet
  GeometryTesterEnvironment<ConfigurationClass>::DefaultTestParameters() const
  {
    // get the default configuration from the configurer
    const std::string ConfigurationString
      = config.DefaultTesterConfiguration();
    
    fhicl::ParameterSet test_pset;
    fhicl::make_ParameterSet(ConfigurationString, test_pset);
    
    return test_pset;
  } // GeometryTesterEnvironment<>::DefaultTestParameters()
  
  
  template <typename ConfigurationClass>
  void GeometryTesterEnvironment<ConfigurationClass>::ExtractTestParameters() {
    
    // first get the general configuration from which to extract the parameters:
    fhicl::ParameterSet const* pSrcCfg = nullptr;
    fhicl::ParameterSet DefaultCfg; // temporary storage, possibly unused
    if (config.ConfigurationPath().empty()) {
      // if no configuration file: save and use the default test configuration
      DefaultCfg = DefaultTestParameters();
      pSrcCfg = &DefaultCfg;
    }
    else // configuration from file is present: just use it
      pSrcCfg = &Parameters();
    
    // then, extract the parameter set from the configuration
    std::string const GeometryTestParameterSetPath
      = config.TesterParameterSetPath();
    tester_params = GeometryTestParameterSetPath.empty()
      ? Parameters()
      : pSrcCfg->get<fhicl::ParameterSet>
        (GeometryTestParameterSetPath, fhicl::ParameterSet())
      ;
  } // GeometryTesterEnvironment<>::ExtractTestParameters()
  
  
  
  template <typename ConfigurationClass>
  void GeometryTesterEnvironment<ConfigurationClass>::Setup(
  ) {
    
    //
    // get the configuration
    //
    Configure();
    
    //
    // set up the message facility
    //
    SetupMessageFacility();
    
    //
    // Optionally print the configuration
    //
    {
      mf::LogInfo msg("Configuration");
      msg << "Complete configuration (";
      if (config.ConfigurationPath().empty()) msg << "default";
      else msg << "'" << config.ConfigurationPath() << "'";
      msg << "):\n" << Parameters().to_indented_string(1);
    }
    
    //
    // set up the geometry
    //
    SetupGeometry();
    
    //
    // save the specific configuration of the test
    //
    ExtractTestParameters();
    
    
    mf::LogInfo("Test") << config.ApplicationName() << " setup complete.";
    
  } // GeometryTesterEnvironment<>::Setup()
  
  
} // namespace testing

#endif // TEST_GEOMETRY_UNIT_TEST_BASE_H
