/**
 * @file    RealComparison_test.cc
 * @brief   Unit test for RealComparisons class
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    November 4th, 2016
 * @see     RealComparisons.h
 */

// Boost libraries
#define BOOST_TEST_MODULE ( RealComparisons_test )
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "larcore/CoreUtils/RealComparisons.h"

// C/C++ standard libraries
#include <string>
#include <ostream>


BOOST_AUTO_TEST_CASE(test_RealComparisons) {
  
  // instantiate a RealComparisons with two classes
  lar::util::RealComparisons<float> check(1e-5);
  
  double const sqrt2 = std::sqrt(2);
  
  // check zero()
  double const epsilon = 2.0 - (sqrt2 * sqrt2);
  BOOST_CHECK( check.zero(epsilon));
  BOOST_CHECK( check.zero(0.0));
  BOOST_CHECK( check.zero(1e-5));
  BOOST_CHECK( check.zero(-1e-5));
  BOOST_CHECK(!check.zero(1.01e-5));
  BOOST_CHECK(!check.zero(-1.01e-5 * 1.01));
  
  // check nonZero()
  BOOST_CHECK(!check.nonZero(epsilon));
  BOOST_CHECK(!check.nonZero(0.0));
  BOOST_CHECK(!check.nonZero(1e-5));
  BOOST_CHECK(!check.nonZero(-1e-5));
  BOOST_CHECK( check.nonZero(1.01e-5));
  BOOST_CHECK( check.nonZero(-1.01e-5 * 1.01));
  
  // check equal()
  BOOST_CHECK(!check.equal(sqrt2, 1.4142));
  BOOST_CHECK( check.equal(sqrt2, 1.414213));
  
  // check strictlyNegative()
  BOOST_CHECK(!check.strictlyNegative(+1e-5 + 1e-7));
  BOOST_CHECK(!check.strictlyNegative(+1e-5 - 1e-7));
  BOOST_CHECK(!check.strictlyNegative(0.0));
  BOOST_CHECK(!check.strictlyNegative(-1e-5 + 1e-7));
  BOOST_CHECK( check.strictlyNegative(-1e-5 - 1e-7));
  
  // check strictlyPositive()
  BOOST_CHECK( check.strictlyPositive(+1e-5 + 1e-7));
  BOOST_CHECK(!check.strictlyPositive(+1e-5 - 1e-7));
  BOOST_CHECK(!check.strictlyPositive(0.0));
  BOOST_CHECK(!check.strictlyPositive(-1e-5 + 1e-7));
  BOOST_CHECK(!check.strictlyPositive(-1e-5 - 1e-7));
  
  // check nonNegative()
  BOOST_CHECK( check.nonNegative(+1e-5 + 1e-7));
  BOOST_CHECK( check.nonNegative(+1e-5 - 1e-7));
  BOOST_CHECK( check.nonNegative(0.0));
  BOOST_CHECK( check.nonNegative(-1e-5 + 1e-7));
  BOOST_CHECK(!check.nonNegative(-1e-5 - 1e-7));
  
  // check nonPositive()
  BOOST_CHECK(!check.nonPositive(+1e-5 + 1e-7));
  BOOST_CHECK( check.nonPositive(+1e-5 - 1e-7));
  BOOST_CHECK( check.nonPositive(0.0));
  BOOST_CHECK( check.nonPositive(-1e-5 + 1e-7));
  BOOST_CHECK( check.nonPositive(-1e-5 - 1e-7));
  
  // check within()
  BOOST_CHECK(!check.within(sqrt2, 0., 1.41420));
  BOOST_CHECK( check.within(sqrt2, 0., 1.41421));
  BOOST_CHECK( check.within(sqrt2, 1.41422, 2.));
  BOOST_CHECK(!check.within(sqrt2, 1.41423, 2.));
  
  // check inverted limits
  BOOST_CHECK(!check.within(sqrt2, 1.41421, 0.));
  BOOST_CHECK(check.withinSorted(sqrt2, 1.41421, 0.));
  
} // BOOST_AUTO_TEST_CASE(test_RealComparisons)
