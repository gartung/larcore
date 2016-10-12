/**
 * @file   ProviderTestHelpers.h
 * @brief  Helper classes to be used together with LArSoft's unit test
 * @date   May 10th, 2016
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @see    ProviderList.h
 * 
 * 
 * Currently provides:
 * - `ProviderSetupClass` class (and `setupProvider()` helper) to be specialised
 *   to support the setup of a specific service provider
 * - `SimpleEnvironmentSetupClass` class (and `simpleEnvironmentSetup()` helper)
 *   to be specialised to support the one-step set up of the provider in a
 *   LArSoft tester environment
 * 
 * This is a pure template header. It does not require additional libraries to
 * be linked.
 */


#ifndef LARCORE_TESTUTILS_PROVIDERTESTHELPERS_H
#define LARCORE_TESTUTILS_PROVIDERTESTHELPERS_H 1


// C/C++ standard libraries
#include <functional> // std::function<>
#include <memory> // std::unique_ptr<>, std::make_unique()
#include <string>
#include <utility> // std::forward()


/// LArSoft test utilities
namespace testing {
  
  
  /// Generic setup provider function type
  template <typename Prov, typename... Args>
  using setupProvider_t = std::function<std::unique_ptr<Prov>(Args...)>;


  /// A default implementation for provider setup class
  template <typename Prov>
  struct DefaultSetupProviderClass {
    
    /// Instantiates a new provider with specified arguments for constructor
    template <typename... Args>
    static std::unique_ptr<Prov> setup(Args&&... args)
      { return std::make_unique<Prov>(std::forward<Args>(args)...); }
    
  }; // DefaultSetupProviderClass()


  /** **************************************************************************
   * @brief Class to create and set up a new provider.
   * @tparam Prov type of provider being set up
   * 
   * The class `ProviderSetupClass<Prov>` is used by ProviderList to create and
   * set up a new service provider. The class must have a static method
   * `setup()`, compatible with setupProvider_t signature: it must return
   * a fully set up provider contained in a std::unique_ptr<Prov>.
   * 
   * An example of implementation is `DefaultSetupProviderClass`, that is in
   * fact the default implementation.
   * 
   * An example to implement a specific setup for a provider, `MyProvider`:
   *     
   *     template <>
   *     struct ProviderSetupClass<MyProvider> {
   *       
   *       static std::unique_ptr<MyProvider> setup
   *         (MyProvider::Config const& cfg)
   *         { return std::make_unique<MyProvider>(fhicl::Table<Config>(cfg)); }
   *       
   *     }; // ProviderSetupClass<MyProvider>
   *     
   * calling a MyProvider(fhicl::Table<Config> const&) constructor.
   * 
   */
  template <typename Prov>
  struct ProviderSetupClass: public DefaultSetupProviderClass<Prov> {};
  
  
  /**
   * @brief Function calling ProviderSetupClass<>::setup for the specified provider
   * @tparam Prov type of the provider to be created and set up
   * @tparam Args types of arguments to the setup function
   * @param args arguments to the setup function
   * @return a pointer to the newly created and set up provider
   * @see ProviderSetup
   * 
   * This function provides a function-like interface to the ProviderSetup
   * classes. An example of its use to create an instance of `MyProvider` is:
   *     
   *     auto prov = testing::setupProvider<MyProv>
   *       (pset.get<fhicl::ParameterSet>("services.MyProviderService"));
   *     
   * if `MyProvider` provider has a constructor taking a fhicl::ParameterSet.
   */
  template <typename Prov, typename... Args>
  std::unique_ptr<Prov> setupProvider(Args&&... args)
    { return ProviderSetupClass<Prov>::setup(std::forward<Args>(args)...); }
  
  
  /** **************************************************************************
   * @brief Environment helper to set up a service provider
   * @tparam Prov type of provider being set up
   * @tparam TestEnv type of environment to set up
   * @see simpleEnvironmentSetup()
   * 
   * The `setup()` static method of this class implements a simple set up of
   * a service provider Prov in an existing testing environment:
   *     
   *     static Prov* setup(TestEnv&);
   *     
   * If such a class is available for a given provider, the environment
   * testing::TesterEnvironment offers the one-stop `SimpleProviderSetup()`
   * method for the complete set up of that provider.
   * 
   * The setup() function will typically:
   * 
   * * find a suitable configuration for the service
   * * instantiate and set up the provider
   * * register the provider and possibly the interface it implements into the
   *   environment
   * 
   * The environment is expected to expose an interface equivalent to the one
   * of `testing::TesterEnvironment`.
   * 
   * See SimpleEnvironmentStandardSetupClass for a class suitable as base class
   * for your own implementation.
   * 
   * To implement a setup for MyProvider, the specialisation can follow the
   * pattern:
   *     
   *     template <typename TestEnv>
   *     struct SimpleEnvironmentSetupClass<MyProvider, TestEnv> {
   *       
   *       static MyProvider* setup(TestEnv& env)
   *         {
   *           // create and set up the provider, e.g.
   *           auto prov = std::make_unique<MyProvider>(
   *             // constructor arguments...
   *             );
   *           // yield and register the provider to the environment
   *           MyProvider* prov_ptr = env.AcquireProvider<MyProvider>(prov);
   *           return prov_ptr;
   *         } // setup()
   *       
   *     }; // SimpleEnvironmentSetupClass<MyProvider>
   *     
   * Note that the class must be fully constructed by the default constructor,
   * in order for it to work with the tester environment.
   */
  template <typename Prov, typename TestEnv>
  struct SimpleEnvironmentSetupClass;


  /**
   * @brief Basic implementation of a environment setup helper.
   * @tparam Prov type of provider being set up
   * @tparam TestEnv type of environment to set up
   * @tparam Interface (optional) type of interface being implemented
   * @see SimpleEnvironmentSetupClass()
   * 
   * The `setup()` static method of this class implements a simple set up of
   * a service provider Prov in an existing testing environment.
   * 
   * The setup() function:
   * 
   * * finds a suitable configuration for the service from the environment
   * * instantiates and sets up the provider
   * * registers the provider and the interface it implements into the
   *   environment
   * 
   * The environment is expected to expose an interface equivalent to the one
   * of `testing::TesterEnvironment`.
   * Use this as:
   *     
   *     template <typename TestEnv>
   *     struct SimpleEnvironmentSetupClass<MyProv, TestEnv>
   *       : public SimpleEnvironmentStandardSetupClass<MyProv, TestEnv, MyInterface>
   *     {
   *       SimpleEnvironmentSetupClass():
   *         SimpleEnvironmentStandardSetupClass<MyProvider, TestEnv, MyInterface>("MyService")
   *         {}
   *     };
   *     
   * to register a provider of type MyProvider that implements a provider
   * interface MyInterface, reading its configuration from the service
   * configuration of `"MyService"` (that is, `services.MyService`).
   * 
   * The environment is then expected to react to both MyProvider and
   * MyInterface types:
   *     
   *     env.Provider<MyProvider>();
   *     env.Provider<MyInterface>();
   *     
   * both return a pointer to the same provider.
   * If instead MyInterface is `void` (default), no interface is registered.
   * 
   * @note This class itself is *not* suitable for direct use in the test
   * environment, lacking a default constructor. The setup class can inherit
   * from it, as in the example above.
   */
  template <typename Prov, typename Interface, typename TestEnv>
  inline Prov* SimpleEnvironmentStandardSetupByName
    (TestEnv& env, std::string service_name)
    {
      return
        env.template SetupProviderFromServiceFor<Interface, Prov>(service_name);
    }
    
  // overload for providers with no shared interface
  template <typename Prov, typename TestEnv>
  inline Prov* SimpleEnvironmentStandardSetupByName
    (TestEnv& env, std::string service_name)
    { return env.template SetupProviderFromService<Prov>(service_name); }


  /**
   * @brief Sets up a provider in a specified test environment
   * @tparam Prov type of the provider to be set up
   * @tparam TestEnv type of the environment
   * @param env the environment to be set up
   * @return a pointer to the newly set up provider
   * @see SimpleEnvironmentSetupClass
   * 
   * This function performs the over-simple set up of a provider.
   * The set up is completely delegated to a SimpleEnvironmentSetupClass
   * class. No such class is provided by default, and service implementers
   * must write their own to allow this one to work.
   */
  template <typename Prov, typename TestEnv>
  Prov* simpleEnvironmentSetup(TestEnv& env)
    { return SimpleEnvironmentSetupClass<Prov, TestEnv>::setup(env); }
  
  
} // namespace testing

#endif // LARCORE_TESTUTILS_PROVIDERTESTHELPERS_H
