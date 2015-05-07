/**
 * @file   geo_types_test.cc
 * @brief  Test of geo_types.h types
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   May 5th, 2015
 */

// Boost libraries
/*
 * Boost Magic: define the name of the module;
 * and do that before the inclusion of Boost unit test headers
 * because it will change what they provide.
 * Among the those, there is a main() function and some wrapping catching
 * unhandled exceptions and considering them test failures, and probably more.
 * This also makes fairly complicate to receive parameters from the command line
 * (for example, a random seed).
 */
#define BOOST_TEST_MODULE ( geo_types_test )
#include <boost/test/auto_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "SimpleTypesAndConstants/geo_types.h"



void test_CryostatID_defaultConstructor() {
  
  geo::CryostatID cid;
  
  // a default-constructed ID is not valid:
  // - check isValid
  BOOST_CHECK(!cid.isValid);
  // - check operator!
  BOOST_CHECK(!cid);
  // - check operator bool
  BOOST_CHECK(!((bool)cid));
  
/* // feature not added
  // test assignment from ID_t
  cid = 1;
  BOOST_CHECK_EQUAL(cid.Cryostat, 1);
*/
  
} // test_CryostatID_defaultConstructor()


void test_CryostatID_directConstructor() {
  
  using ID_t = geo::CryostatID::ID_t;
  
  geo::CryostatID cid(1);
  
  // an explicitly constructed ID is valid:
  // - check isValid
  BOOST_CHECK(cid.isValid);
  // - check operator!
  BOOST_CHECK(!!cid);
  // - check operator bool
  BOOST_CHECK((bool)cid);
  // - check the ID value
  BOOST_CHECK_EQUAL(cid.Cryostat, ID_t(1));
  
  // test comparison operators
  // (exercise copy constructor too)
  geo::CryostatID smaller_cid(0), same_cid(cid), larger_cid(2);
  
  BOOST_CHECK(  cid        <  larger_cid  );
  BOOST_CHECK(  cid        != larger_cid  );
  BOOST_CHECK(!(cid        == larger_cid) );
  BOOST_CHECK(!(larger_cid <         cid) );
  BOOST_CHECK(larger_cid.cmp(cid) > 0);
  BOOST_CHECK(cid.cmp(larger_cid) < 0);
  
  BOOST_CHECK(!(cid      <  same_cid) );
  BOOST_CHECK(  cid      == same_cid  );
  BOOST_CHECK(!(cid      != same_cid) );
  BOOST_CHECK(!(same_cid <       cid) );
  BOOST_CHECK(same_cid.cmp(cid) == 0);
  BOOST_CHECK(cid.cmp(same_cid) == 0);
  
  BOOST_CHECK(!(cid         <  smaller_cid) );
  BOOST_CHECK(!(cid         == smaller_cid) );
  BOOST_CHECK(  cid         != smaller_cid  );
  BOOST_CHECK(  smaller_cid <          cid  );
  BOOST_CHECK(smaller_cid.cmp(cid) < 0);
  BOOST_CHECK(cid.cmp(smaller_cid) > 0);
  
  
  // make sure the ID with cryostat 0 is fine (it's not a bad ID!)
  geo::CryostatID first_cid(0);
  // - check isValid
  BOOST_CHECK(first_cid.isValid);
  // - check operator!
  BOOST_CHECK(!!first_cid);
  // - check operator bool
  BOOST_CHECK((bool)first_cid);
  // - check the ID value
  BOOST_CHECK_EQUAL(first_cid.Cryostat, ID_t(0));
  
  
} // test_CryostatID_defaultConstructor()



void test_TPCID_defaultConstructor() {
  
  geo::TPCID tid;
  
  // a default-constructed ID is not valid:
  // - check isValid
  BOOST_CHECK(!tid.isValid);
  // - check operator!
  BOOST_CHECK(!tid);
  // - check operator bool
  BOOST_CHECK(!((bool)tid));
  
} // test_TPCID_defaultConstructor()


void test_TPCID_directConstructor() {
  
  using ID_t = geo::CryostatID::ID_t;
  
  geo::CryostatID cid(1);
  geo::TPCID tid(cid, 15);
  
  // TODO
  // an explicitly constructed ID is valid:
  // - check isValid
  BOOST_CHECK(cid.isValid);
  // - check operator!
  BOOST_CHECK(!!cid);
  // - check operator bool
  BOOST_CHECK((bool)cid);
  // - check the ID value
  BOOST_CHECK_EQUAL(cid.Cryostat, ID_t(1));
  
  // test comparison operators
  // (exercise copy constructor too)
  geo::CryostatID smaller_cid(0), same_cid(cid), larger_cid(2);
  
  BOOST_CHECK(  cid        <  larger_cid  );
  BOOST_CHECK(  cid        != larger_cid  );
  BOOST_CHECK(!(cid        == larger_cid) );
  BOOST_CHECK(!(larger_cid <         cid) );
  BOOST_CHECK(larger_cid.cmp(cid) > 0);
  BOOST_CHECK(cid.cmp(larger_cid) < 0);
  
  BOOST_CHECK(!(cid      <  same_cid) );
  BOOST_CHECK(  cid      == same_cid  );
  BOOST_CHECK(!(cid      != same_cid) );
  BOOST_CHECK(!(same_cid <       cid) );
  BOOST_CHECK(same_cid.cmp(cid) == 0);
  BOOST_CHECK(cid.cmp(same_cid) == 0);
  
  BOOST_CHECK(!(cid         <  smaller_cid) );
  BOOST_CHECK(!(cid         == smaller_cid) );
  BOOST_CHECK(  cid         != smaller_cid  );
  BOOST_CHECK(  smaller_cid <          cid  );
  BOOST_CHECK(smaller_cid.cmp(cid) < 0);
  BOOST_CHECK(cid.cmp(smaller_cid) > 0);
  
  
  // make sure the ID with cryostat 0 is fine (it's not a bad ID!)
  geo::CryostatID first_cid(0);
  // - check isValid
  BOOST_CHECK(first_cid.isValid);
  // - check operator!
  BOOST_CHECK(!!first_cid);
  // - check operator bool
  BOOST_CHECK((bool)first_cid);
  // - check the ID value
  BOOST_CHECK_EQUAL(first_cid.Cryostat, ID_t(0));
  
  
} // test_TPCID_directConstructor()





//
// CryostatID test
//
BOOST_AUTO_TEST_CASE(CryostatIDtest) {
  test_CryostatID_defaultConstructor();
  test_CryostatID_directConstructor();
}

//
// TPCID test
//
BOOST_AUTO_TEST_CASE(TPCIDtest) {
  test_TPCID_defaultConstructor();
  test_TPCID_directConstructor();
}

