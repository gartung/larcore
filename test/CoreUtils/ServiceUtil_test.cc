/**
 * @file   ServiceUtil_test.cc
 * @brief  Tests the utilities in ServiceUtil.h
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   April 28, 2016
 * @see    ServiceUtil.h
 * 
 * This test takes no command line argument.
 * 
 */

/*
 * Boost Magic: define the name of the module;
 * and do that before the inclusion of Boost unit test headers
 * because it will change what they provide.
 * Among the those, there is a main() function and some wrapping catching
 * unhandled exceptions and considering them test failures, and probably more.
 */
#define BOOST_TEST_MODULE ( ServiceUtil_test )

// LArSoft libraries
#include "larcore/CoreUtils/UncopiableAndUnmoveableClass.h"
#include "larcore/CoreUtils/ServiceUtil.h"

// art libraries
#include "art/Utilities/Exception.h"
#include "art/Framework/Services/Registry/ServiceScope.h"

// Boost libraries
#include <boost/test/auto_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// C/C++ standard libraries
#include <type_traits>


//------------------------------------------------------------------------------
struct MyProvider: protected lar::UncopiableAndUnmoveableClass {};

class MyService {
   MyProvider* prov = nullptr;

      public:
   MyService(MyProvider* ptr = nullptr): prov(ptr) {}

   using provider_type = MyProvider;

   provider_type const* provider() const { return prov; }
  
}; // MyService

namespace lar {
   namespace details {

      template struct ServiceProviderRequirementsChecker<MyProvider>;
      
      template struct ServiceRequirementsChecker<MyService>;
      
   } // namespace details
} // namespace lar




// This is art. Well, kind of.
struct GlobalServicesClass {
   std::unique_ptr<MyService> myServicePtr = nullptr;
}; // GlobalServicesClass
GlobalServicesClass GlobalServices;

namespace art {
   
   namespace detail {
      template <>
      struct ServiceHelper<MyService> {
         static constexpr art::ServiceScope scope_val
           = art::ServiceScope::LEGACY;
      };
   } // namespace detail
   
   template <>
   class ServiceHandle<MyService, art::ServiceScope::LEGACY> {
      MyService* instance;
         public:
      ServiceHandle(): instance(GlobalServices.myServicePtr.get()) {}
      MyService* operator->() const { return instance; }
      MyService& operator*() const { return *instance; }
   }; // ServiceHandle<MyService>
   
} // namespace art


BOOST_AUTO_TEST_CASE(providerFromTest) {
   
   // on the first try, there is no service provider.
   
   GlobalServices.myServicePtr = std::make_unique<MyService>();
   BOOST_CHECK_EXCEPTION(lar::providerFrom<MyService>(), art::Exception,
     [](art::Exception const& e)
       { return e.categoryCode() == art::errors::NotFound; }
     );
   
   // now let's create a "real" provider
   MyProvider prov;
   GlobalServices.myServicePtr = std::make_unique<MyService>(&prov);
   BOOST_CHECK_EQUAL(lar::providerFrom<MyService>(), &prov);
   
   // that's enough; let's clean up
   GlobalServices.myServicePtr.reset();
   
} // BOOST_AUTO_TEST_CASE(larProviderFromTest)



//------------------------------------------------------------------------------