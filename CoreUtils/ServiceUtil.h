#ifndef COREUTILS_SERVICEUTIL
#define COREUTILS_SERVICEUTIL
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include <type_traits>

namespace lar{

  template <class T>
    typename T::provider_type const* providerFrom()
    {
      static_assert(
		    (! std::is_copy_constructible<typename T::provider_type>::value
		     && ! std::is_copy_assignable<typename T::provider_type>::value
		     && ! std::is_move_constructible<typename T::provider_type>::value
		     && ! std::is_move_assignable<typename T::provider_type>::value),
		    "Data provider classes must not be copiable or moveable"
		    );
      art::ServiceHandle<T> h;
      return h->provider();
      
    }
} // namespace lar

#endif //#COREUTILS_SERVICEUTIL
