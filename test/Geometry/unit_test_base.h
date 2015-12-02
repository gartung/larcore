/**
 * @file   unit_test_base.h
 * @brief  Base class for unit tests using FHiCL configuration
 * @date   December 1st, 2015
 * @author petrillo@fnal.gov
 * 
 * Provides an environment for easy set up of a message-facility-aware test.
 * 
 * For an example of how to expand it to host services,
 * see larcore/test/Geometry/geometry_unit_test_base.h
 * 
 * Currently provides:
 * - BasicEnvironmentConfiguration: a test environment configuration
 * - TestSharedGlobalResource: mostly internal use
 * - TesterEnvironment: a prepacked test environment
 * 
 */


#ifndef TEST_UNIT_TEST_BASE_H
#define TEST_UNIT_TEST_BASE_H

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
#include <stdexcept> // std::logic_error


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
   *
   * This class needs to be fully constructed by the default constructor
   * in order to be useful as Boost unit test fixture.
   * It is supposed to be passed as a template parameter to another class
   * that can store an instance of it and extract configuration information
   * from it.
   */
  struct BasicEnvironmentConfiguration {
    
    /// Default constructor; this is what is used in Boost unit test
    BasicEnvironmentConfiguration() { DefaultInit(); }
    
    /// Constructor: acquires parameters from the command line
    BasicEnvironmentConfiguration(int argc, char** argv):
      BasicEnvironmentConfiguration()
      { ParseCommandLine(argc, argv); }
    
    /// Constructor; accepts the name as parameter
    BasicEnvironmentConfiguration(std::string name):
      BasicEnvironmentConfiguration()
      { SetApplicationName(name); }
    
    BasicEnvironmentConfiguration(int argc, char** argv, std::string name):
      BasicEnvironmentConfiguration(argc, argv)
      { SetApplicationName(name); }
    
    /// @{
    /// @name Access to configuration
    /// Name of the application
    std::string ApplicationName() const { return appl_name; }
    
    /// Path to the configuration file
    std::string ConfigurationPath() const { return config_path; }
    
    /// FHiCL path for the configuration of the test algorithm
    std::string TesterParameterSetPath(std::string name) const
      {
        auto iPath = test_paths.find(name);
        return (iPath == test_paths.end())
          ? ("physics.analyzers." + name): iPath->second;
      }
    
    /// Name of the test algorithm instance
    std::string MainTesterParameterSetName() const { return main_test_name; }
    
    /// FHiCL path for the configuration of the test algorithm
    std::string MainTesterParameterSetPath() const
      {
        return MainTesterParameterSetName().empty()
          ? "": TesterParameterSetPath(MainTesterParameterSetName());
      }
    
    /// FHiCL path for the configuration of the service
    std::string ServiceParameterSetPath(std::string name) const
      {
        auto iPath = service_paths.find(name);
        return (iPath == service_paths.end())
          ? ("services." + name): iPath->second;
      } // ServiceParameterSetPath()
    
    /// A string describing default parameter set to configure specified test
    std::string DefaultTesterConfiguration(std::string tester_name) const
      { return analyzers_default_cfg.at(tester_name); }
    
    /// A string describing the default parameter set to configure the test
    std::string DefaultServiceConfiguration(std::string service_name) const
      { return services_default_cfg.at(service_name); }
    
    /// A string describing the full default parameter set
    std::string DefaultConfiguration() const
      { return BuildDefaultConfiguration(); }
    
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
    
    /// Sets the FHiCL name for the configuration of the test algorithm
    void SetMainTesterParameterSetName(std::string name)
      { main_test_name = name; }
    
    /// Sets the FHiCL path for the configuration of a test algorithm
    void SetTesterParameterSetPath(std::string test_name, std::string path)
      { test_paths[test_name] = path; }
    
    /// Sets the FHiCL path for the configuration of the main test algorithm
    void SetMainTesterParameterSetPath(std::string path)
      {
        if (MainTesterParameterSetName().empty()) {
          throw std::logic_error
             ("Request setting configuration of non-existent main tester");
        }
        SetTesterParameterSetPath(MainTesterParameterSetName(), path);
      }
    
    /// Sets the FHiCL path for the configuration of a test algorithm
    void SetServiceParameterSetPath(std::string service_name, std::string path)
      { service_paths[service_name] = path; }
    
    /// Adds a default configuration for the specified service
    void AddDefaultServiceConfiguration
      (std::string service_name, std::string service_cfg)
      { services_default_cfg.emplace(service_name, service_cfg); }
    
    /// Adds a default configuration for the specified tester
    void AddDefaultTesterConfiguration
      (std::string tester_name, std::string tester_cfg)
      { analyzers_default_cfg.emplace(tester_name, tester_cfg); }
    
    /// Adds a default configuration for the main tester
    void AddDefaultTesterConfiguration(std::string tester_cfg)
      {
        if (MainTesterParameterSetName().empty()) {
          throw std::logic_error
             ("Request adding configuration of non-existent main tester");
        }
        AddDefaultTesterConfiguration(MainTesterParameterSetName(), tester_cfg);
      }
    
    ///@}
    
    
    
      protected:
    using ConfigurationMap_t = std::map<std::string, std::string>;
    using PathMap_t = std::map<std::string, std::string>;
    
    std::string appl_name; ///< name of the application
    std::string config_path; ///< configuration file path
    std::string main_test_name; ///< name of main test algorithm
    std::string main_test_path; ///< path of main test algorithm configuration
    
    /// Returns the default test name
    static std::string DefaultApplicationName() { return "Test"; }
    
    /// Configuration of all the services
    ConfigurationMap_t services_default_cfg;
    /// Configuration of all the analyzer modules
    ConfigurationMap_t analyzers_default_cfg;
    
    /// Set of paths for tester configuration
    PathMap_t test_paths;
    /// Set of paths for service configuration
    PathMap_t service_paths;
    
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
        SetApplicationName(DefaultApplicationName());
        SetMainTesterParameterSetName("");
        // a destination which will react to all messages from DEBUG up:
        AddDefaultServiceConfiguration("message",
          R"(
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
         )");
      } // DefaultInit()
    
    /// A string describing the full default parameter set
    std::string BuildDefaultServiceConfiguration() const
      { return BuildServiceConfiguration(services_default_cfg); }
    
    /// A string describing the full default parameter set
    std::string BuildDefaultTestConfiguration() const
      { return BuildTestConfiguration(analyzers_default_cfg); }
    
    /// A string describing the full default parameter set
    std::string BuildDefaultConfiguration() const
      {
        return BuildConfiguration(services_default_cfg, analyzers_default_cfg);
      }
    
    
    /// A string with the service section from service parameter sets
    static std::string BuildServiceConfiguration
      (ConfigurationMap_t const& services)
      {
        std::string cfg;
        cfg += "\nservices: {";
        for (auto const& service_info: services) {
          cfg += "\n  " + service_info.first + ": {";
          cfg += "\n" + service_info.second;
          cfg += "\n  } # " + service_info.first;
        } // for services
        cfg +=
          "\n} # services"
          "\n";
        return cfg;
      } // BuildServiceConfiguration()
    
    /// A string with the physics section from analyzer parameter sets
    static std::string BuildTestConfiguration
      (ConfigurationMap_t const& analyzers)
      {
        std::string cfg;
        cfg +=
          "\nphysics: {"
          "\n  analyzers: {"
          ;
        for (auto const& module_info: analyzers) {
          cfg += "\n  " + module_info.first + ": {";
          cfg += "\n" + module_info.second;
          cfg += "\n  } # " + module_info.first;
        } // for analyzers
        cfg +=
          "\n  } # analyzers"
          "\n} # physics";
        return cfg;
      } // BuildServiceConfiguration()
    
    /// A string describing the full default parameter set
    static std::string BuildConfiguration
      (ConfigurationMap_t const& services, ConfigurationMap_t const& modules)
      {
        std::string cfg;
        cfg += BuildServiceConfiguration(services);
        cfg += BuildTestConfiguration(modules);
        return cfg;
      } // BuildConfiguration()
    
      private:
    details::CommandLineArguments arguments; ///< command line arguments
    
  }; // class BasicEnvironmentConfiguration<>
  
  
  
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
   * @brief Environment for a test
   * @tparam ConfigurationClass a class providing compile-time configuration
   * 
   * The test environment is set up on construction.
   * 
   * The environment provides:
   * - Parameters() method returning the complete FHiCL configuration
   * - TesterParameters() method returning the configuration for the test
   * 
   * This class or a derived one can be used as global fixture for unit tests.
   * 
   * Unfortunately Boost does not give any control on the initialization of the
   * object, so everything must be ready to go as hard coded.
   * The ConfigurationClass class tries to alleviate that.
   * That is another, small static class that TesterEnvironment uses to
   * get its parameters.
   * 
   * The requirements for the ConfigurationClass are:
   * - `std::string ApplicationName()`: the application name
   * - `std::string ConfigurationPath()`: path to the configuration file
   * - `std::string MainTesterParameterSetName()`: name of the
   *   configuration of the main test (commodity)
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
   * (TesterEnvironment<ConfigurationClass>(no_setup))
   */
  template <typename ConfigurationClass>
  class TesterEnvironment: private details::CommandLineArguments {
    
      public:
    using Configuration_t = ConfigurationClass;
    
    /**
     * @brief Constructor: sets everything up and declares the test started
     * 
     * The configuration is from a default-constructed ConfigurationClass.
     * This is suitable for use as Boost unit test fixture.
     */
    TesterEnvironment(bool bSetup = true) { if (bSetup) Setup(); }
    
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
    TesterEnvironment(Configuration_t const& cfg_obj, bool bSetup = true):
      config(cfg_obj)
      { if (bSetup) Setup(); }
    TesterEnvironment(Configuration_t&& cfg_obj, bool bSetup = true):
      config(cfg_obj)
      { if (bSetup) Setup(); }
    //@}
    
    /// Destructor: closing remarks
    virtual ~TesterEnvironment();
    
    
    /// Returns the full configuration
    fhicl::ParameterSet const& Parameters() const { return params; }
    
    /// Returns the configuration of the specified service
    fhicl::ParameterSet ServiceParameters(std::string service_name) const
      {
        return params.get<fhicl::ParameterSet>
          (config.ServiceParameterSetPath(service_name));
      }
    
    /// Returns the configuration of the specified test
    fhicl::ParameterSet TesterParameters(std::string test_name) const
      {
        return params.get<fhicl::ParameterSet>
          (config.TesterParameterSetPath(test_name));
      }
    
    /// Returns the configuration of the main test (undefined if no main test)
    fhicl::ParameterSet TesterParameters() const
      {
        if (config.MainTesterParameterSetName().empty()) return {};
        else return TesterParameters(config.MainTesterParameterSetName());
      }
    
    
    static fhicl::ParameterSet CompileParameterSet(std::string cfg);
    
      protected:
    
    /// Returns a read-only version of the configuration
    Configuration_t const& Config() const { return config; }
    
    /// The complete initialization, ran at construction by default
    virtual void Setup();
    
    /// Reads and translates the configuration
    virtual void Configure();
    
    /**
     * @brief Creates a full configuration for the test
     * @return a parameters set with the complete configuration
     */
    virtual fhicl::ParameterSet DefaultParameters() const
      { return CompileParameterSet(config.DefaultConfiguration()); }
    
    //@{
    /// Sets up the message facility
    virtual void SetupMessageFacility
      (fhicl::ParameterSet const& pset, std::string appl_name = "") const;
    virtual void SetupMessageFacility() const
      { SetupMessageFacility(Parameters(), config.ApplicationName()); }
    //@}
    
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
    Configuration_t config; ///< instance of the configurer
    
    
    void FillArgumentsFromCommandLine();
    
    fhicl::ParameterSet params; ///< full configuration of the test
    
  }; // class TesterEnvironment<>
  
  
  
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
  TesterEnvironment<ConfigurationClass>::~TesterEnvironment() {
    
    mf::LogInfo("Test") << config.ApplicationName() << " completed.";
    
  } // TesterEnvironment<>::~TesterEnvironment()
  
  
  /** **************************************************************************
   * @brief Compiles a parameter set from a string
   * @return a parameters set with the complete configuration
   */
  template <typename ConfigurationClass>
  fhicl::ParameterSet
  TesterEnvironment<ConfigurationClass>::CompileParameterSet(std::string cfg) {
    fhicl::ParameterSet global_pset;
    fhicl::make_ParameterSet(cfg, global_pset);
    return global_pset;
  } // TesterEnvironment<>::CompileParameterSet()
  
  
  /** **************************************************************************
   * @brief Returns the configuration from a FHiCL file
   * @param config_path full path of the FHiCL configuration file
   * @return a parameters set with the complete configuration from the file
   */
  template <typename ConfigurationClass>
  fhicl::ParameterSet
  TesterEnvironment<ConfigurationClass>::ParseParameters
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
  } // TesterEnvironment<>::ParseParameters()
  
  
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
  void TesterEnvironment<ConfigurationClass>::Configure() {
    std::string config_path = config.ConfigurationPath();
    params = config_path.empty()?
      DefaultParameters(): ParseParameters(config_path);
  } // TesterEnvironment::Configure()
  
  
  /** **************************************************************************
   * @brief Sets the message facility up
   * 
   * Message facility configuration is expected in "services.message" parameter
   * set. If not there, the default configuration is used.
   */
  template <typename ConfigurationClass>
  void TesterEnvironment<ConfigurationClass>::SetupMessageFacility
    (fhicl::ParameterSet const& pset, std::string appl_name /* = "" */) const
  {
    fhicl::ParameterSet mf_pset;
    
    if
      (!pset.get_if_present(config.ServiceParameterSetPath("message"), mf_pset))
    {
      mf_pset
        = CompileParameterSet(config.DefaultServiceConfiguration("message"));
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
  } // TesterEnvironment::SetupMessageFacility()
  
  
  
  template <typename ConfigurationClass>
  void TesterEnvironment<ConfigurationClass>::Setup(
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
    
    
    mf::LogInfo("Test") << config.ApplicationName() << " base setup complete.";
    
  } // TesterEnvironment<>::Setup()
  
  
} // namespace testing

#endif // TEST_UNIT_TEST_BASE_H
