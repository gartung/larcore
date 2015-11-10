/**
 * @file   ServiceUtil.h
 * @brief  Utilities related to art service access
 * @author Jonathan Paley (jpaley@fnal.gov)
 * 
 * This library is currently a pure header.
 * 
 */

#ifndef COREUTILS_SERVICEUTIL
#define COREUTILS_SERVICEUTIL

// framework libraries
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Utilities/Exception.h"
#include "cetlib/demangle.h"

// C/C++ standard libraries
#include <type_traits>
#include <typeinfo>


namespace lar {
  
  /**
   * @brief Returns a constant pointer to the provider of specified service
   * @tparam T type of the service
   * @return a constant pointer to the provider of specified service
   * @throws art::Exception (category art::errors::NotFound) if pointer is null
   * 
   * This function relies on the following service and provider interface:
   * - provider is not movable nor copyable
   * - service contains a type "provider_type" defined as the class of the
   *   service provider
   * - service contains a method "provider()" that returns a non-null pointer
   *   to a service provider; the service provider is owned and managed by
   *   the service, and the caller is not responsible of regulating the object
   *   lifetime, nor it should attempt to
   * 
   * Violations of the protocol yield compilation errors (in case non-compliance
   * can be statically detected), or throw of exceptions.
   * 
   * Example of usage:
   *     
   *     auto const* geom = lar::providerFrom<geo::Geometry>();
   *     
   * retrieves the service provider for LArSoft geometry.
   * This requires the inclusion of "Geometry/Geometry.h" header, where the
   * service is declared. Typically, both ServiceUtil.h and the header of the
   * provider class are included in the service header.
   * 
   */
  template <class T>
    typename T::provider_type const* providerFrom()
    {
      // static check on provider class: not copyable nor movable
      static_assert(
        !std::is_copy_constructible<typename T::provider_type>::value,
        "Data provider classes must not be copyable"
        );
      static_assert(
        !std::is_copy_assignable<typename T::provider_type>::value,
        "Data provider classes must not be copyable"
        );
      static_assert(
        !std::is_move_constructible<typename T::provider_type>::value,
        "Data provider classes must not be movable"
        );
      static_assert(
        !std::is_move_assignable<typename T::provider_type>::value,
        "Data provider classes must not be movable"
        );
      
      // retrieve the provider
      art::ServiceHandle<T> h;
      typename T::provider_type const* pProvider = h->provider();
      if (!pProvider) {
        throw art::Exception(art::errors::NotFound)
          << "Service <" << cet::demangle(typeid(T).name())
          << "> offered a null provider";
      }
      
      return pProvider;
      
    } // providerFrom()
  
} // namespace lar

#endif //#COREUTILS_SERVICEUTIL
