/**
 * @file    ProviderPack.h
 * @brief   Data structure containing constant pointers to classes
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    November 20th, 2015
 * @version 1.0
 * 
 * This is a header-only library.
 * It depends only on standard C++ and does not require additional linkage.
 */


#ifndef DETECTORINFO_PROVIDERPACK_H
#define DETECTORINFO_PROVIDERPACK_H


// C/C++ standard library
#include <tuple>
#include <type_traits>
#include <limits>


namespace lar {
  
  namespace details {
    
    template <typename... Types>
    struct has_duplicate_types;
    
    
    template <typename Key, typename... Types>
    struct index_with_type_impl;
    
    /**
     * @brief Hosts the index of type FindType in the list of types AmongTypes
     * @tparam FindType the type to find
     * @tparam AmongTypes the list of types
     * 
     * If FindType type is not any of AmongTypes types, a compilation error
     * message (static assertion) is issued.
     * Otherwise, the class has a `value` member pointing to the index of
     * FindType among AmongTypes.
     * This value is suct that this assertion is valid:
     *     
     *     std::is_same<
     *       FindType,
     *       typename std::tuple_element<
     *         index_with_type<FindType, AmongTypes...>::value,
     *         std::tuple<AmongTypes...>
     *         >
     *       >::value
     *     
     */
    template <typename FindType, typename... AmongTypes>
    struct index_with_type;

    
    /// Implementation detail for the extraction constructor
    template
      <typename DestPack, typename SourcePack, typename... ExtractProviders>
    struct SetFrom;
 
  } // namespace details
  
  
  /** **************************************************************************
   * @brief Container for a list of pointers to providers
   * @tparam Providers types of the providers in the parameter pack
   * 
   * The pointers are stored as constant.
   * Note that this container can host any type of objects, and it has 
   * "provider" in the name because the reason it was written was to provide
   * a fast way to specify a set of LArSoft service providers.
   * The only limitation is that there should be only one object per type.
   * Pointed objects are not owned by this class.
   *     
   *     A a;
   *     B b;
   *     C c;
   *     D d;
   *     ProviderPack<A, B, C> pack(&a, &b, &c);
   *     
   *     // obtain a constant pointerto b from pack:
   *     B const* b_ptr = pack.get<B>();
   *     
   *     if (pack.has<D>()) std::cerr << "Unexpected!" << std::endl;
   *     
   * (note that the latter check can be coded as a static assert)
   */
  template <typename... Providers>
  class ProviderPack {
    static_assert(!details::has_duplicate_types<Providers...>::value,
      "Providers in ProviderPack are repeated");
    
    using this_type = ProviderPack<Providers...>; ///< alias of this class
    
    /// type used for storage of the pointers
    using tuple_type = std::tuple<Providers const*...>;
    
      public:
    
    /// Default constructor: a null provider pointer for each type
    ProviderPack() = default;
    
    /// Constructor: stores a provider pointer for each type
    ProviderPack(Providers const* ...provider_ptrs): providers(provider_ptrs...)
      {}
    
    /**
     * @brief Constructor: extracts the providers from another parameter pack
     * @tparam OtherProviders list of the providers of the source provider pack
     * @param from where to copy the information from
     * 
     * This constructor requires all the providers we need to be present
     * in the source provider pack.
     */
    template<typename... OtherProviders>
    ProviderPack(ProviderPack<OtherProviders...> const& from)
      {
        details::SetFrom
          <this_type, ProviderPack<OtherProviders...>, Providers...>
          (*this, from);
      }

    /**
     * @brief Constructor: picks the providers from the specified ones
     * @tparam OtherProviders list of the type of providers offered
     * @param providers all the providers needed (or more)
     * 
     * This constructor will pick, among the offered providers, the ones that
     * are needed.
     */
    template<typename... OtherProviders>
    ProviderPack(OtherProviders const*... providers)
      {
        details::SetFrom
          <this_type, ProviderPack<OtherProviders...>, Providers...>
          (*this, ProviderPack<OtherProviders...>(providers...));
      }

    /**
     * @brief Constructor: picks the providers from a pack plus specified ones
     * @tparam FromPack parameter pack to start from
     * @tparam OtherProviders list of the type of providers offered
     * @param fromPack providers to be picked
     * @param providers all the remaining providers needed (or more)
     * @see expandProviderPack()
     * 
     * This constructor will pick all the providers from the specified pack,
     * and the ones from the other providers.
     * This constructor can be used to "expand" from another provider:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *     A a;
     *     B b;
     *     C c;
     *     D d;
     *     ProviderPack<A, D> pack(&a, &d);
     *     ProviderPack<A, B, C, D> largerPack(pack, &c, &b);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     */
    template<typename... PackProviders, typename... OtherProviders>
    ProviderPack(
      ProviderPack<PackProviders...> const& fromPack,
      OtherProviders const*... providers
      );
    
    
    /// Returns the provider with the specified type
    template <typename Provider>
    Provider const* get() const
      {
        return std::get<details::index_with_type<Provider, Providers...>::value>
          (providers);
      } // get<>()
    
    
    /// Sets the provider with the specified type
    template <typename Provider>
    void set(Provider const* provider_ptr)
      {
        std::get<details::index_with_type<Provider, Providers...>::value>
          (providers)
          = provider_ptr;
      } // set<>()
    
    /// Returns whether there is a provider with the specified type
    template <typename Provider>
    static constexpr bool has()
      {
        return details::index_with_type_impl<Provider, Providers...>::value
          < sizeof...(Providers);
      } // has<>()
    
    
    /// Returns whether other provider pack has all the same providers as this
    template <typename... OtherProviders>
    bool operator== (ProviderPack<OtherProviders...> const& other) const;
    
    /// Returns whether other provider pack and this have different providers
    template <typename... OtherProviders>
    bool operator!= (ProviderPack<OtherProviders...> const& other) const;
    
    
    /**
     * @brief Returns whether all our providers are in the OfferedProviders list
     * @tparam OfferedProviders list of offered providers
     * 
     * This static function returns true if all the providers in this provider
     * pack are included among the OfferedProviders list. That list can contain
     * additional provider types, which will not affect the result.
     * 
     * Usage example:
     *     
     *     using providers_t
     *       = lar::ProviderPack<geo::GeometryCore, detinfo::LArProperties>;
     *     static_assert(
     *       providers_t::containsProviders
     *         <detinfo::LArProperties, detinfo::DetectorProperties>(),
     *       "Not all the required providers are present."
     *       );
     *     
     * In this example, the assertion will fail because of the absence of
     * `detinfo::DetectorProperties` from providers_t.
     */
    template <typename... OtherProviders>
    static constexpr bool containsProviders();
    
      private:
    
    tuple_type providers; ///< container of the pointers, type-safe
    
  }; // class ProviderPack
  
  
  /**
   * @brief Function to create a ProviderPack from the function arguments
   * @tparam Providers types of the providers in the parameter pack
   * @param providers constant pointers to the providers
   * @return a ProviderPack object containing all the specified providers
   *
   * This is an convenience function to reduce the typing needed to instantiate
   * a ProviderPack. Example:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *     A a;
   *     B b;
   *     auto pack = makeProviderPack(&a, &b);
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * creates a `ProviderPack<A, B>`.
   */
  template <typename... Providers>
  ProviderPack<Providers...> makeProviderPack(Providers const* ...providers)
    { return ProviderPack<Providers...>(providers...); }
  
  
  /**
   * @brief Function to create a ProviderPack by adding to another
   * @tparam PackProviders types of the providers in the original parameter pack
   * @tparam MoreProviders types of the providers to be added
   * @param pack parameter pack with the first providers
   * @param providers constant pointers to the other providers to be added
   * @return a ProviderPack object containing all the specified providers
   *
   * This is an convenience function to reduce the typing needed to instantiate
   * a ProviderPack. Use it like:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   *     A a;
   *     B b;
   *     C c;
   *     D d;
   *     auto pack = makeProviderPack(&a, &d);
   *     auto largerPack = expandProviderPack(pack, &c, &b);
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * creates a `ProviderPack<A, D, C, B>` including all the four objects.
   */
  template <typename... PackProviders, typename... MoreProviders>
  ProviderPack<PackProviders..., MoreProviders...> expandProviderPack(
    ProviderPack<PackProviders...> const& pack,
    MoreProviders const* ...providers
    )
    { return { pack, providers... }; }
  
  
} // namespace lar


//------------------------------------------------------------------------------
//--- Implementation details
//---
namespace lar {
  namespace details {
    
    //--------------------------------------------------------------------------
    //--- has_type, has_duplicate_types, are_same_types
    //---
    template <typename Target, typename... Types>
    struct has_type:
      public std::integral_constant
        <bool, index_with_type_impl<Target, Types...>::value < sizeof...(Types)>
      {};
    
    
    //--------------------------------------------------------------------------
    template <typename Key, typename... Types>
    struct has_duplicate_types<Key, Types...>:
      public std::integral_constant
        <bool, has_type<Key, Types...>() || has_duplicate_types<Types...>()>
      {};
    
    template <>
    struct has_duplicate_types<>: public std::false_type {};
    
    
    //--------------------------------------------------------------------------
    template <typename... Types>
    struct are_types_contained;
    
    template <typename... Types>
    struct are_same_types {
      
      template <typename... AsTypes>
      static constexpr bool as()
        {
          return (sizeof...(Types) == sizeof...(AsTypes))
            && are_types_contained<Types...>::template in<AsTypes...>();
        }
      
    }; // are_same_types
    
    
    template <typename First, typename... OtherTypes>
    struct are_types_contained<First, OtherTypes...> {
      template <typename... AsTypes>
      static constexpr bool in()
        {
          return are_types_contained<OtherTypes...>::template in<AsTypes...>()
            && has_type<First, AsTypes...>();
        }
    };
    
    template <typename First>
    struct are_types_contained<First> {
      template <typename... AsTypes>
      static constexpr bool in()
        { return has_type<First, AsTypes...>(); }
    };

    
    template <typename T>
    struct is_provider_pack: public std::false_type {};
    
    template <typename... Providers>
    struct is_provider_pack<ProviderPack<Providers...>>: public std::true_type
      {};
    
    
    template <typename APack, typename BPack>
    struct have_same_provider_types: public std::false_type {};
    
    template <typename... AProviders, typename... BProviders>
    struct have_same_provider_types
      <ProviderPack<AProviders...>, ProviderPack<BProviders...>>
      : public std::integral_constant
        <bool, are_same_types<AProviders...>::template as<BProviders...>()>
      {};
    
    
    //--------------------------------------------------------------------------
    //--- index_with_type
    //---
    /// Generic template, to glue the two special cases;
    /// in general, this will have in value field the index in Types of the type
    /// Key, or a number larger than the number if types in Types if Key type
    /// is not present among Types.
    template <typename Key, typename... Types>
    struct index_with_type_impl;
    
    // common case: one or more types
    template <typename Key, typename FirstType, typename... Types>
    struct index_with_type_impl<Key, FirstType, Types...>:
      public std::integral_constant <size_t,
        std::is_same<Key, FirstType>::value
          ? 0
          : index_with_type_impl<Key, Types...>::value + 1
        >
      {};
    
    // special case: no type; this means failure
    template <typename Key>
    struct index_with_type_impl<Key>: public std::integral_constant <size_t, 0U>
      {};
    
    
    template <typename FindType, typename... AmongTypes>
    struct index_with_type:
      public std::integral_constant
        <size_t, index_with_type_impl<FindType, AmongTypes...>::value>
    {
      static_assert(
        index_with_type<FindType, AmongTypes...>::value < sizeof...(AmongTypes),
        "Required type is not present"
        );
    }; // index_with_type
    
    //--------------------------------------------------------------------------
    //--- SetFrom
    //---
    template <
      typename DestPack, typename SourcePack,
      typename FirstProvider, typename... OtherProviders
      >
    struct SetFrom<DestPack, SourcePack, FirstProvider, OtherProviders...> {
      SetFrom(DestPack& pack, SourcePack const& from)
        {
          pack.set(from.template get<FirstProvider>());
          SetFrom<DestPack, SourcePack, OtherProviders...>(pack, from);
        }
    }; // SetFrom<First, Others...>
    
    template <typename DestPack, typename SourcePack>
    struct SetFrom<DestPack, SourcePack> {
      SetFrom(DestPack&, SourcePack const&) {}
    };

    //--------------------------------------------------------------------------
    //--- Compare
    //---
    template <typename Provider, typename APack, typename BPack>
    bool haveSameProvider(APack const& a, BPack const& b) {
      static_assert(is_provider_pack<APack>() && is_provider_pack<BPack>(),
        "This class needs two ProviderPack template types.");
      return a.template get<Provider>() == b.template get<Provider>();
    } // haveSameProvider()
    
    
    template <typename APack, typename BPack>
    struct ProviderPackComparerBase {
      
      static_assert(have_same_provider_types<APack, BPack>(),
        "The specified provider packs have different types.");
      
    }; // ProviderPackComparerBase
    
    
    template <typename APack, typename BPack, typename... Providers>
    struct ProviderPackComparer;
    
    template
      <typename APack, typename BPack, typename First, typename... Others>
    struct ProviderPackComparer<APack, BPack, First, Others...>
      : ProviderPackComparerBase<APack, BPack>
    {
      static bool compare (APack const& a, BPack const& b)
        {
          return haveSameProvider<First>(a, b)
            && ProviderPackComparer<APack, BPack, Others...>::compare(a, b);
        }
    }; // ProviderPackComparer<APack, BPack, First, Others...>
    
    template
      <typename APack, typename BPack, typename First>
    struct ProviderPackComparer<APack, BPack, First>
      : ProviderPackComparerBase<APack, BPack>
    {
      static bool compare (APack const& a, BPack const& b)
        { return haveSameProvider<First>(a, b); }
    }; // ProviderPackComparer<APack, BPack, First>
    
    
    //--------------------------------------------------------------------------

  } // namespace details
  
  
  //----------------------------------------------------------------------------
  //--- ProviderPack
  //---
  
  template <typename... Providers>
  template<typename... PackProviders, typename... OtherProviders>
  ProviderPack<Providers...>::ProviderPack(
    ProviderPack<PackProviders...> const& fromPack,
    OtherProviders const*... providers
    )
  {
    
    // verify that the list of providers in argument is the exact one we need
    static_assert(
      details::are_same_types<Providers...>
        ::template as<PackProviders..., OtherProviders...>(),
      "The providers types in the arguments do not match the ones needed."
      );
    
    // copy all the providers from the provider pack
    details::SetFrom
      <this_type, ProviderPack<PackProviders...>, PackProviders...>
      (*this, fromPack);
    
    // put the other providers in a temporary parameter pack, and copy it
    // (this is convenience, a direct implementation would be probably better)
    details::SetFrom
      <this_type, ProviderPack<OtherProviders...>, OtherProviders...>
      (*this, makeProviderPack(providers...));
    
  } // ProviderPack<Providers...>::ProviderPack(ProviderPack, OtherProviders...)
  
  
  template <typename... Providers>
  template <typename... OtherProviders>
  bool ProviderPack<Providers...>::operator==
    (ProviderPack<OtherProviders...> const& other) const
  {
    return details::ProviderPackComparer<
      ProviderPack<Providers...>, ProviderPack<OtherProviders...>, Providers...
      >::compare(*this, other);
  }
  
  template <typename... Providers>
  template <typename... OtherProviders>
  bool ProviderPack<Providers...>::operator!=
    (ProviderPack<OtherProviders...> const& other) const
    { return !(*this == other); }
  
 
  //----------------------------------------------------------------------------
  template <typename... Providers>
  template <typename... OfferedProviders>
  constexpr bool ProviderPack<Providers...>::containsProviders() {
    return details::are_types_contained<Providers...>
      ::template in<OfferedProviders...>();
  } // ProviderPack<>::containsProviders()
  
  
  //----------------------------------------------------------------------------
  
} // namespace lar

#endif // DETECTORINFO_PROVIDERPACK_H
