/**
 * @file    DereferenceIterator_test.cc
 * @brief   Unit test for DereferenceIterator class
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    November 18th, 2016
 * @see     DereferenceIterator.h
 */

// Boost libraries
#define BOOST_TEST_MODULE ( DereferenceIterator_test )
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "larcore/CoreUtils/DereferenceIterator.h"

// C/C++ standard libraries
#include <iterator> // std::back_inserter()
#include <memory> // std::make_unique()
#include <utility> // std::move()
#include <vector>
#include <list>



template <template <typename T, typename...> class SeqCont>
void test_DereferenceIterator() {
  
  using Container_t = SeqCont<std::unique_ptr<size_t>>;
  
  //
  // create the dummy structure (not a very smart one)
  //
  Container_t v;
  
  auto inserter = std::back_inserter(v);
  for (size_t i = 0; i < 10; ++i)
    *(inserter++) = std::move(std::make_unique<size_t>(i));
  
  //
  // test that we can loop and find the correct value in a forward loop
  //
  auto begin = lar::util::beginDereferenceIterator(v);
  auto end = lar::util::endDereferenceIterator(v);
  auto it = begin;
  BOOST_CHECK(it == begin);
  for (size_t i = 0; i < 10; ++i, ++it) {
    
    BOOST_CHECK_EQUAL(*it, i);
    
  } // for i
  BOOST_CHECK(it == end);
  
  //
  // test that we can loop and find the correct value in a reversed loop
  //
  auto rbegin = lar::util::rbeginDereferenceIterator(v);
  auto rend = lar::util::rendDereferenceIterator(v);
  auto rit = rbegin;
  BOOST_CHECK(rit == rbegin);
  for (size_t i = 0; i < 10; ++i, ++rit) {
    
    BOOST_CHECK_EQUAL(*rit, v.size() - i - 1);
    
  } // for i
  BOOST_CHECK(rit == rend);
  
  //
  // test that we can do a ranged-for loop
  //
  size_t index = 0;
  for (size_t& i: lar::util::dereferenceIteratorLoop(v)) {
    
    BOOST_CHECK_EQUAL(i, index);
    
    ++index;
  } // for i
  
  //
  // test that we can do a constant ranged-for loop
  //
  index = 0; // reset
  auto const& cv = v; // constant version
  for (auto& i: lar::util::dereferenceConstIteratorLoop(cv)) {
    
    static_assert(std::is_const<std::remove_reference_t<decltype(i)>>::value,
      "Dereferenced value from constant vector is not constant");
    
    BOOST_CHECK_EQUAL(i, index);
    
    ++index;
  } // for i

  //
  // test that we can do a constant ranged-for loop also on a non-const coll.
  //
  index = 0; // reset
  for (auto& i: lar::util::dereferenceConstIteratorLoop(v)) {
    
    static_assert(std::is_const<std::remove_reference_t<decltype(i)>>::value,
      "Dereferenced value from constant vector is not constant");
    
    BOOST_CHECK_EQUAL(i, index);
    
    ++index;
    
  } // for i

  // 
  // check that we can write in a normal loop
  // 
  for (auto& i: lar::util::dereferenceIteratorLoop(v)) i = 10;
  
  for (auto i: lar::util::dereferenceConstIteratorLoop(cv))
    BOOST_CHECK_EQUAL(i, 10);
  
  
} // test_DereferenceIterator<>()


BOOST_AUTO_TEST_CASE(DereferenceIterator_testcase) {

  test_DereferenceIterator<std::vector>();
  
  test_DereferenceIterator<std::list>();
  
} // BOOST_AUTO_TEST_CASE(DereferenceIterator_testcase)
