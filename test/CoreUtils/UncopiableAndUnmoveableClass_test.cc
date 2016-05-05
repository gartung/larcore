/**
 * @file   UncopiableAndUnmoveableClass_test.cc
 * @brief  Tests the content of UncopiableAndUnmoveableClass.h
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   April 28, 2016
 * @see    UncopiableAndUnmoveableClass.h
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
#define BOOST_TEST_MODULE ( UncopiableAndUnmoveableClass_test )

// LArSoft libraries
#include "larcore/CoreUtils/UncopiableAndUnmoveableClass.h"

// Boost libraries
#include <boost/test/auto_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// C/C++ standard libraries
#include <type_traits>


//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(UncopiableAndUnmoveableClassTest) {
   
   // check lar::UncopiableAndUnmoveableClass class itself
   BOOST_CHECK
     (!std::is_copy_constructible<lar::UncopiableAndUnmoveableClass>::value);
   BOOST_CHECK
     (!std::is_copy_assignable<lar::UncopiableAndUnmoveableClass>::value);
   BOOST_CHECK
     (!std::is_move_constructible<lar::UncopiableAndUnmoveableClass>::value);
   BOOST_CHECK
     (!std::is_move_assignable<lar::UncopiableAndUnmoveableClass>::value);
   
   
   // check a class derived from lar::UncopiableAndUnmoveableClass class
   struct Derived: protected lar::UncopiableAndUnmoveableClass {};
   
   BOOST_CHECK(!std::is_copy_constructible<Derived>::value);
   BOOST_CHECK(!std::is_copy_assignable   <Derived>::value);
   BOOST_CHECK(!std::is_move_constructible<Derived>::value);
   BOOST_CHECK(!std::is_move_assignable   <Derived>::value);
   
   
   // check a class derived from lar::UncopiableAndUnmoveableClass class
   // and made moveable
   struct MoveableDerived: protected lar::UncopiableAndUnmoveableClass {
      MoveableDerived(MoveableDerived&&):
        lar::UncopiableAndUnmoveableClass() {}
   };
   
   BOOST_CHECK(!std::is_copy_constructible<MoveableDerived>::value);
   BOOST_CHECK(!std::is_copy_assignable   <MoveableDerived>::value);
   BOOST_CHECK( std::is_move_constructible<MoveableDerived>::value);
   BOOST_CHECK(!std::is_move_assignable   <MoveableDerived>::value);
   
   
   // check a class derived from lar::UncopiableAndUnmoveableClass class
   // and made both copy- and move-assignable
   struct AssignableDerived: protected lar::UncopiableAndUnmoveableClass {
      AssignableDerived& operator=(AssignableDerived const&) { return *this; }
      AssignableDerived& operator=(AssignableDerived&&) { return *this; }
   };
   
   BOOST_CHECK(!std::is_copy_constructible<AssignableDerived>::value);
   BOOST_CHECK( std::is_copy_assignable   <AssignableDerived>::value);
   BOOST_CHECK(!std::is_move_constructible<AssignableDerived>::value);
   BOOST_CHECK( std::is_move_assignable   <AssignableDerived>::value);
   
} // BOOST_AUTO_TEST_CASE(larProviderFromTest)


//------------------------------------------------------------------------------