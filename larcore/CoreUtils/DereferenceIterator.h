/**
 * @file   DereferenceIterator.h
 * @brief  Offer iterators automatically dereferencing their values
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   November 18, 2016
 * 
 * This is a header only template library.
 * 
 * It offers:
 * 
 * - iterators (but you probably do not want to instantiate them directly):
 *   DereferenceIterator, DereferenceConstIterator
 * - iterator wrappers:
 *   makeDereferenceIterator(), makeDereferenceConstIterator()
 * - begin and end iterator extractors from collections:
 *   beginDereferenceIterator(), endDereferenceIterator(),
 *   rbeginDereferenceIterator(), rendDereferenceIterator(),
 *   cbeginDereferenceIterator(), cendDereferenceIterator(),
 *   crbeginDereferenceIterator(), crendDereferenceIterator()
 * - functions to enable range-for loops:
 *   dereferenceIteratorLoop(),  dereferenceIteratorReverseLoop(),
 *   dereferenceConstIteratorLoop(), dereferenceConstIteratorReverseLoop()
 *
 */

#ifndef LARCORE_COREUTILS_DEREFERENCEITERATOR_H
#define LARCORE_COREUTILS_DEREFERENCEITERATOR_H

// C/C++ standard libraries
#include <utility> // std::move(), std::declval()
#include <type_traits> // std::add_lvalue_reference_t<>, std::add_pointer_t<>


namespace lar {
  namespace util {
    
    namespace detail {
      
      template <typename BeginIter, typename EndIter>
      class IteratorBox {
        BeginIter b;
        EndIter e;
        
          public:
        IteratorBox(BeginIter b, EndIter e): b(b), e(e) {}
        
        BeginIter const& begin() const { return b; }
        EndIter const& end() const { return e; }
        
      }; // IteratorBox<>
      
      template <typename BeginIter, typename EndIter>
      auto makeIteratorBox(BeginIter b, EndIter e)
        { return IteratorBox<BeginIter, EndIter>(b, e); }
      
      
      
      /// Base class for dereferencing iterators
      template <typename Iter, typename Value>
      class DereferenceIteratorBase {
        using iterator_t = Iter; ///< wrapped iterator type
        using this_t = DereferenceIteratorBase<Iter, Value>; ///< this type
        
        iterator_t iter; ///< wrapper iterator
        
          public:
        /// Tag used for initialization
        struct initialize_tag {};
        
        /// @{
        /// @name Type definitions for standard iterators
            
        using difference_type = typename iterator_t::difference_type;
        using value_type = Value;
        using pointer = std::add_pointer_t<value_type>;
        using reference = std::add_lvalue_reference_t<value_type>;
        using iterator_category = typename iterator_t::iterator_category;
        
        /// @}
        
        
        /// Default constructor
        DereferenceIteratorBase() = default;
        
        /// Constructor: copies the specified iterator in
        DereferenceIteratorBase(Iter const& iter, initialize_tag)
          : iter(iter) {}
        
        /// Constructor: acquires the specified iterator
        DereferenceIteratorBase(Iter&& iter, initialize_tag)
          : iter(std::move(iter)) {}
        
        /// @brief Generic copy constructor
        /// @tparam OtherIter base iterator: must be assignable to iterator_t
        /// @tparam OtherValue value type: must be convertible into value_type
        template <typename OtherIter, typename OtherValue>
        DereferenceIteratorBase
          (DereferenceIteratorBase<iterator_t, OtherValue> const& other)
          : iter(other.iter)
          {
            static_assert(std::is_convertible<OtherValue, value_type>::value,
              "Copying from a iterator with incompatible value"
              );
          }
        
        /// @brief Generic move constructor
        /// @tparam OtherIter base iterator: must be assignable to iterator_t
        /// @tparam OtherValue value type: must be convertible into value_type
        template <typename OtherIter, typename OtherValue>
        DereferenceIteratorBase
          (DereferenceIteratorBase<iterator_t, OtherValue>&& other)
          : iter(std::move(other.iter))
          {
            static_assert(std::is_convertible<OtherValue, value_type>::value,
              "Moving from a iterator with incompatible value"
              );
          }
        
        
        /// Returns a reference to the data pointed by the original iterator
        reference operator* () const { return **iter; }
        
        /// Returns a reference to the data pointed by the original iterator
        pointer operator-> () const { return *iter; }
        
        /// Returns a reference to the i-th element after the pointed one
        reference operator[] (difference_type i) const { return *(iter[i]); }
        
        /// Prefix increment operator
        this_t& operator++() { ++iter; return *this; }
        
        /// Prefix decrement operator
        this_t& operator--() { --iter; return *this; }
        
        /// Postfix increment operator
        this_t operator++(int) { return this_t(iter++, {}); }
        
        /// Postfix decrement operator
        this_t operator--(int) { return this_t(iter--, {}); }
        
        /// @{
        /// Arithmetic operators (symmetric)
        this_t operator+ (difference_type offset) const
          { return { iter + offset }; }
        this_t operator- (difference_type offset) const
          { return { iter - offset }; }
        
        this_t& operator+= (difference_type offset)
          { iter += offset; return *this; }
        this_t& operator-= (difference_type offset)
          { iter -= offset; return *this; }
        
        /// @}
        
        /// Returns the difference from another iterator
        difference_type operator- (this_t const& other) const
          { return iter - other.iter; }
        
        /// Bonus: returns true if the pointer is not dereferentiable
        bool is_null() const { return iterator_t::operator* () == nullptr; }
        
        /// @{
        /// @name Comparison operators between iterators
        
        bool operator== (this_t const& other) const
          { return other.iter == iter; }
        bool operator!= (this_t const& other) const
          { return other.iter != iter; }
        bool operator<= (this_t const& other) const
          { return other.iter <= iter; }
        bool operator>= (this_t const& other) const
          { return other.iter >= iter; }
        bool operator<  (this_t const& other) const
          { return other.iter < iter; }
        bool operator>  (this_t const& other) const
          { return other.iter < iter; }
          
        /// @}
        
      }; // class DereferenceIteratorBase
      
      /// Swapped addition operator
      template <typename Iter, typename Value>
      auto operator+ (
        typename DereferenceIteratorBase<Iter, Value>::difference_type offset,
        DereferenceIteratorBase<Iter, Value> const& iter
        )
        { return iter + offset; }
      
      
    } // namespace detail
    
    
    /** ************************************************************************
     * @brief An iterator wrapper to dereference pointed values
     * @tparam Iter type of iterator being wrapped
     * @see beginDereferenceIterator(), endDereferenceIterator(),
     *      dereferenceIteratorLoop()
     * 
     * This iterator is useful to iterate a collection of pointers accessing
     * directly the pointed values.
     * 
     * It's probably easier to use cbeginDereferenceIterator(),
     * cendDereferenceIterator() and dereferenceConstIteratorLoop() rather than
     * instantiating this class directly.
     * 
     * @note Note the bizarre construction mechanism, needed to differentiate
     *       from copy constructor. This allows nesting iterators.
     */
    template <typename Iter>
    using DereferenceConstIterator = detail::DereferenceIteratorBase<
      Iter,
      std::add_const_t<std::decay_t<decltype(**(std::declval<Iter>()))>>
      >;
      
    /**
     * @brief An iterator wrapper to dereferencing pointed values as constant
     * @tparam Iter type of iterator being wrapped
     * @see DereferenceConstIterator
     * 
     * This class behaves like DereferenceConstIterator, except that it returns
     * mutable references to values.
     */
    template <typename Iter>
    using DereferenceIterator = detail::DereferenceIteratorBase<
      Iter,
      std::decay_t<decltype(**(std::declval<Iter>()))>
      >;
    
    
    
    /**
     * @brief Returns a dereference iterator to the begin of specified container
     * @tparam Iter type of the iterator to be wrapped (may be constant)
     * @param iter the iterator to be wrapped
     * @return a dereference iterator of iter
     * @see beginDereferenceIterator(), endDereferenceIterator(),
     *      rbeginDereferenceIterator(), rendDereferenceIterator(),
     *      dereferenceIteratorLoop(), dereferenceIteratorReverseLoop()
     * 
     * If the type is constant, the iterator will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     */
    template <typename Iter>
    auto makeDereferenceIterator(Iter&& iter)
      { return DereferenceIterator<Iter>(std::forward<Iter>(iter), {}); }
    
    /**
     * @brief Returns a dereference iterator to the begin of specified container
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return a dereference iterator of cont.begin()
     * 
     * If the type is constant, the iterator will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     */
    template <typename Cont>
    auto beginDereferenceIterator(Cont& cont)
      { return makeDereferenceIterator(cont.begin()); }
    
    /**
     * @brief Returns a dereference iterator to the end of specified container
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return a dereference iterator of cont.end()
     * 
     * If the type is constant, the iterator will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     */
    template <typename Cont>
    auto endDereferenceIterator(Cont& cont)
      { return makeDereferenceIterator(cont.end()); }
    
    /**
     * @brief Returns a dereference reverse iterator to the begin of container
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return a dereference iterator of cont.rbegin()
     * 
     * If the type is constant, the iterator will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     */
    template <typename Cont>
    auto rbeginDereferenceIterator(Cont& cont)
      { return makeDereferenceIterator(cont.rbegin()); }
    
    /**
     * @brief Returns a dereference reverse iterator to the end of container
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return a dereference iterator of cont.rend()
     * 
     * If the type is constant, the iterator will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     */
    template <typename Cont>
    auto rendDereferenceIterator(Cont& cont)
      { return makeDereferenceIterator(cont.rend()); }
    
    /**
     * @brief Returns an object enabling a dereferencing range-for loop
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return an object enabling a dereferencing range-for loop
     * @see dereferenceIteratorReverseLoop()
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * std::vector<std::unique_ptr<int>> v;
     * 
     * // fill the vector
     * 
     * for (int& i: dereferenceIteratorLoop(v)) {
     *   
     *   std::cout << i << std::endl;
     *   
     * } // for i
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * If the type is constant, the loop variable will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     * 
     */
    template <typename Cont>
    auto dereferenceIteratorLoop(Cont& cont)
      { 
        return detail::makeIteratorBox
          (beginDereferenceIterator(cont), endDereferenceIterator(cont));
      } // dereferenceIteratorLoop()
    
    
    /**
     * @brief Returns an object enabling a dereferencing reverse range-for loop
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return an object enabling a dereferencing range-for loop
     * @see dereferenceIteratorLoop()
     * 
     * This function is similar to dereferenceIteratorLoop(), but the order of
     * iteration is reversed.
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * std::vector<std::unique_ptr<int>> v;
     * 
     * // fill the vector
     * 
     * for (int& i: dereferenceIteratorReverseLoop(v)) {
     *   
     *   std::cout << i << std::endl;
     *   
     * } // for i
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * If the type is constant, the loop variable will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     * 
     */
    template <typename Cont>
    auto dereferenceIteratorReverseLoop(Cont& cont)
      { 
        return detail::makeIteratorBox
          (rbeginDereferenceIterator(cont), rendDereferenceIterator(cont));
      } // dereferenceIteratorReverseLoop()
    
    
    /// @{
    /// @name Constant iterator functions
    /// 
    /// See the documentation of the similar non-const ones.
    
    /// @see makeDereferenceIterator
    template <typename Iter>
    auto makeDereferenceConstIterator(Iter&& iter)
      { return DereferenceConstIterator<Iter>(std::forward<Iter>(iter), {}); }
    
    /// @see beginDereferenceIterator
    template <typename Cont>
    auto cbeginDereferenceIterator(Cont& cont)
      { return makeDereferenceConstIterator(cont.cbegin()); }
    
    /// @see endDereferenceIterator
    template <typename Cont>
    auto cendDereferenceIterator(Cont& cont)
      { return makeDereferenceConstIterator(cont.cend()); }
    
    /// @see rbeginDereferenceIterator
    template <typename Cont>
    auto crbeginDereferenceIterator(Cont& cont)
      { return makeDereferenceConstIterator(cont.crbegin()); }
    
    /// @see crendDereferenceIterator
    template <typename Cont>
    auto crendDereferenceIterator(Cont& cont)
      { return makeDereferenceConstIterator(cont.crend()); }
    
    /**
     * @brief Returns an object enabling a dereferencing range-for loop
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return an object enabling a dereferencing range-for loop
     * @see dereferenceIteratorReverseLoop()
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * std::vector<std::unique_ptr<int>> v;
     * 
     * // fill the vector
     * 
     * for (int const& i: dereferenceConstIteratorLoop(v)) {
     *   
     *   std::cout << i << std::endl;
     *   
     * } // for i
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * This wrapper forces the iteration variable to be constant.
     */
    template <typename Cont>
    auto dereferenceConstIteratorLoop(Cont& cont)
      { 
        return detail::makeIteratorBox
          (cbeginDereferenceIterator(cont), cendDereferenceIterator(cont));
      } // dereferenceConstIteratorLoop()
    
    
    /**
     * @brief Returns an object enabling a dereferencing reverse range-for loop
     * @tparam Cont type of the container (may be constant)
     * @param cont container to extract the iterator from
     * @return an object enabling a dereferencing range-for loop
     * @see dereferenceConstIteratorLoop()
     * 
     * This function is similar to dereferenceConstIteratorLoop(), but the order
     * of iteration is reversed.
     * 
     * Example of usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * std::vector<std::unique_ptr<int>> v;
     * 
     * // fill the vector
     * 
     * for (int const& i: dereferenceConstIteratorReverseLoop(v)) {
     *   
     *   std::cout << i << std::endl;
     *   
     * } // for i
     * 
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * If the type is constant, the loop variable will likely be constant too.
     * This wrapper is quite blind to the meaning of iterators and their
     * constantness.
     * 
     */
    template <typename Cont>
    auto dereferenceConstIteratorReverseLoop(Cont& cont)
      { 
        return detail::makeIteratorBox
          (crbeginDereferenceIterator(cont), crendDereferenceIterator(cont));
      } // dereferenceConstIteratorReverseLoop()
    
    /// @}
    
  } // namespace util
} // namespace lar


#endif // LARCORE_COREUTILS_DEREFERENCEITERATOR_H
