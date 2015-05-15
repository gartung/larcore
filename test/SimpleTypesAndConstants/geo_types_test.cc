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

// C/C++ standard libraries
#include <iostream>


void TestIDvalidity(geo::CryostatID const& id, bool answer) {
  // - check isValid
  BOOST_CHECK_EQUAL(id.isValid, answer);
  // - check operator!
  BOOST_CHECK_EQUAL(!id, !answer);
  // - check operator bool
  BOOST_CHECK_EQUAL((bool)id, answer);
} // TestIDvalidity()

/// Test comparison operators
template <typename TESTID, typename REFID = TESTID>
void TestCompareSmallerID(TESTID const& id, REFID  const& smaller) {
  BOOST_CHECK(!(id      <  smaller) );
  BOOST_CHECK(!(id      == smaller) );
  BOOST_CHECK(  id      != smaller  );
  BOOST_CHECK(  smaller <       id  );
  BOOST_CHECK(smaller.cmp(id) < 0);
  BOOST_CHECK(id.cmp(smaller) > 0);
} // TestCompareSmallerID()

/// Test comparison operators
template <typename TESTID, typename REFID = TESTID>
void TestCompareSameID(TESTID const& id, REFID  const& same) {
  BOOST_CHECK(!(id      <  same) );
  BOOST_CHECK(  id      == same  );
  BOOST_CHECK(!(id      != same) );
  BOOST_CHECK(!(same <     id) );
  BOOST_CHECK(same.cmp(id) == 0);
  BOOST_CHECK(id.cmp(same) == 0);
} // TestCompareSameID()

/// Test comparison operators
template <typename TESTID>
void TestCompareSelfID(TESTID const& id)
  { return TestCompareSameID(id, id); }

/// Test comparison operators
template <typename TESTID, typename REFID = TESTID>
void TestCompareLargerID(TESTID const& id, REFID  const& larger) {
  BOOST_CHECK(  id     <  larger  );
  BOOST_CHECK(  id     != larger  );
  BOOST_CHECK(!(id     == larger) );
  BOOST_CHECK(!(larger <      id) );
  BOOST_CHECK(larger.cmp(id) > 0);
  BOOST_CHECK(id.cmp(larger) < 0);
} // TestCompareLargerID()


/// Test comparison operators
template <typename TESTID, typename REFID = TESTID>
void TestIDcomparison(
  TESTID const& id,
  REFID  const& smaller,
  REFID  const& same,
  REFID  const& larger
) {
  TestCompareSmallerID(id, smaller);
  TestCompareSameID(id, same);
  TestCompareSelfID(id);
  TestCompareLargerID(id, larger);
} // TestCryostatComparison()


void test_CryostatID_defaultConstructor() {
  
  std::cout << "Testing default-constructed cryostat ID" << std::endl;
  
  geo::CryostatID cid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(cid, false);
  
/* // feature not added
  // test assignment from ID_t
  cid = 1;
  BOOST_CHECK_EQUAL(cid.Cryostat, 1);
*/
  
} // test_CryostatID_defaultConstructor()


void test_CryostatID_directConstructor() {
  
  std::cout << "Testing cryostat ID constructed with an integer" << std::endl;
  
  geo::CryostatID cid(1);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(cid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(cid.Cryostat, geo::CryostatID::CryostatID_t(1));
  
  // test comparison operators
  // (exercise copy constructor too)
  geo::CryostatID smaller_cid(0), same_cid(cid), larger_cid(2);
  
  TestIDcomparison(cid, smaller_cid, same_cid, larger_cid);
  
  
  // make sure the ID with cryostat 0 is fine (it's not a bad ID!)
  std::cout << "Testing cryostat ID constructed with an integer 0" << std::endl;
  
  geo::CryostatID first_cid(0);
  TestIDvalidity(cid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(first_cid.Cryostat, geo::CryostatID::CryostatID_t(0));
  
} // test_CryostatID_directConstructor()



void test_TPCID_defaultConstructor() {
  
  std::cout << "Testing default-constructed TPC ID" << std::endl;
  
  geo::TPCID tid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(tid, false);
  
} // test_TPCID_defaultConstructor()


void test_TPCID_nestedConstructor() {
  
  std::cout << "Testing ID-constructed TPC ID" << std::endl;
  
  geo::CryostatID cid(1);
  geo::TPCID tid(cid, 15);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(tid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(tid.Cryostat, geo::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(tid.TPC,                geo::TPCID::TPCID_t(15));
  
  // test comparison operators (exercise copy constructor too)
  // - with TPC ID
  std::cout << "Testing comparison with TPC ID" << std::endl;
  geo::TPCID smaller_tid(cid, tid.TPC - 1), same_tid(tid),
    larger_tid(cid, tid.TPC + 1);
  
  TestIDcomparison(tid, smaller_tid, same_tid, larger_tid);
  
} // test_TPCID_nestedConstructor()
 

void test_TPCID_directConstructor() {
  
  std::cout << "Testing TPC ID constructed with indices" << std::endl;
  
  geo::TPCID tid(1, 15);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(tid, true);
  
  std::cout << "Testing comparison with same cryostat ID" << std::endl;
  
  geo::TPCID smaller_tid(1, 14), same_tid(1, 15), larger_tid(1, 16);
  TestIDcomparison(tid, smaller_tid, same_tid, larger_tid);
  
  std::cout << "Testing comparison with different cryostat ID" << std::endl;
  geo::TPCID smaller_cid(0, 16), larger_cid(2, 14);
  TestCompareSmallerID(tid, smaller_cid);
  TestCompareLargerID(tid, larger_cid);
  
  // make sure the ID with TPC 0 is fine (it's not a bad ID!)
  std::cout << "Testing TPC ID constructed with a TPC #0" << std::endl;
  
  geo::TPCID first_tid(0, 0);
  TestIDvalidity(first_tid, true);
  
  // - check the ID value
  BOOST_CHECK_EQUAL(first_tid.Cryostat, geo::CryostatID::CryostatID_t(0));
  BOOST_CHECK_EQUAL(first_tid.TPC,                geo::TPCID::TPCID_t(0));
  
  
} // test_TPCID_directConstructor()



void test_PlaneID_defaultConstructor() {
  
  std::cout << "Testing default-constructed plane ID" << std::endl;
  
  geo::PlaneID pid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(pid, false);
  
} // test_PlaneID_defaultConstructor()


void test_PlaneID_nestedConstructor() {
  
  std::cout << "Testing ID-constructed plane ID" << std::endl;
  
  geo::TPCID tid(1, 15);
  geo::PlaneID pid(tid, 32);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(pid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(pid.Cryostat, geo::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(pid.TPC,                geo::TPCID::TPCID_t(15));
  BOOST_CHECK_EQUAL(pid.Plane,          geo::PlaneID::PlaneID_t(32));
  
  // test comparison operators (exercise copy constructor too)
  std::cout << "Testing comparison with plane ID" << std::endl;
  geo::PlaneID smaller_pid(tid, pid.Plane - 1), same_pid(pid),
    larger_pid(tid, pid.Plane + 1);
  
  TestIDcomparison(pid, smaller_pid, same_pid, larger_pid);
  
} // test_PlaneID_nestedConstructor()
 

void test_PlaneID_directConstructor() {
  
  std::cout << "Testing plane ID constructed with indices" << std::endl;
  
  geo::PlaneID pid(1, 15, 32);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(pid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(pid.Cryostat, geo::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(pid.TPC,                geo::TPCID::TPCID_t(15));
  BOOST_CHECK_EQUAL(pid.Plane,          geo::PlaneID::PlaneID_t(32));
  
  std::cout << "Testing comparison with same TPC ID" << std::endl;
  
  geo::PlaneID
    smaller_pid(1, 15, 31), same_pid(1, 15, 32), larger_pid(1, 15, 33);
  TestIDcomparison(pid, smaller_pid, same_pid, larger_pid);
  
  std::cout << "Testing comparison with different TPC ID (1)" << std::endl;
  geo::PlaneID smaller_tid1(1, 14, 33), larger_tid1(1, 16, 31);
  TestCompareSmallerID(pid, smaller_tid1);
  TestCompareLargerID(pid, larger_tid1);
  std::cout << "Testing comparison with different TPC ID (2)" << std::endl;
  geo::PlaneID smaller_tid2(1, 14, 32), larger_tid2(1, 16, 32);
  TestCompareSmallerID(pid, smaller_tid2);
  TestCompareLargerID(pid, larger_tid2);
  
  std::cout << "Testing comparison with different cryostat ID" << std::endl;
  geo::PlaneID smaller_cid1(0, 15, 33), larger_cid1(2, 15, 31);
  TestCompareSmallerID(pid, smaller_cid1);
  TestCompareLargerID(pid, larger_cid1);
  std::cout << "Testing comparison with different cryostat ID (2)" << std::endl;
  geo::PlaneID smaller_cid2(0, 15, 32), larger_cid2(2, 15, 32);
  TestCompareSmallerID(pid, smaller_cid2);
  TestCompareLargerID(pid, larger_cid2);
  
  // make sure the ID with TPC 0 is fine (it's not a bad ID!)
  std::cout << "Testing Plane ID constructed with a Plane #0" << std::endl;
  
  geo::PlaneID first_pid(0, 0, 0);
  TestIDvalidity(first_pid, true);
  
  // - check the ID value
  BOOST_CHECK_EQUAL(first_pid.Cryostat, geo::CryostatID::CryostatID_t(0));
  BOOST_CHECK_EQUAL(first_pid.TPC,                geo::TPCID::TPCID_t(0));
  BOOST_CHECK_EQUAL(first_pid.Plane,          geo::PlaneID::PlaneID_t(0));
  
} // test_PlaneID_directConstructor()



void test_WireID_defaultConstructor() {
  
  std::cout << "Testing default-constructed wire ID" << std::endl;
  
  geo::WireID wid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(wid, false);
  
} // test_WireID_defaultConstructor()


void test_WireID_nestedConstructor() {
  
  std::cout << "Testing ID-constructed wire ID" << std::endl;
  
  geo::PlaneID pid(1, 15, 32);
  geo::WireID wid(pid, 27);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(wid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(wid.Cryostat, geo::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(wid.TPC,                geo::TPCID::TPCID_t(15));
  BOOST_CHECK_EQUAL(wid.Plane,          geo::PlaneID::PlaneID_t(32));
  BOOST_CHECK_EQUAL(wid.Wire,             geo::WireID::WireID_t(27));
  
  // test comparison operators (exercise copy constructor too)
  // - with TPC ID
  std::cout << "Testing comparison with wire ID" << std::endl;
  geo::WireID smaller_wid(pid, wid.Wire - 1), same_wid(wid),
    larger_wid(pid, wid.Wire + 1);
  
  TestIDcomparison(wid, smaller_wid, same_wid, larger_wid);
  
} // test_WireID_nestedConstructor()
 

void test_WireID_directConstructor() {
  
  std::cout << "Testing wire ID constructed with indices" << std::endl;
  
  geo::WireID wid(1, 15, 32, 27);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(wid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(wid.Cryostat, geo::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(wid.TPC,                geo::TPCID::TPCID_t(15));
  BOOST_CHECK_EQUAL(wid.Plane,          geo::PlaneID::PlaneID_t(32));
  BOOST_CHECK_EQUAL(wid.Wire,             geo::WireID::WireID_t(27));
  
  std::cout << "Testing comparison with same TPC ID" << std::endl;
  
  geo::WireID
    smaller_wid(1, 15, 32, 26), same_wid(1, 15, 32, 27),
    larger_wid(1, 15, 32, 28);
  TestIDcomparison(wid, smaller_wid, same_wid, larger_wid);
  
  std::cout << "Testing comparison with different plane ID (1)" << std::endl;
  geo::WireID smaller_pid1(1, 15, 31, 28), larger_pid1(1, 15, 33, 26);
  TestCompareSmallerID(wid, smaller_pid1);
  TestCompareLargerID(wid, larger_pid1);
  std::cout << "Testing comparison with different plane ID (2)" << std::endl;
  geo::WireID smaller_pid2(1, 15, 31, 27), larger_pid2(1, 15, 33, 27);
  TestCompareSmallerID(wid, smaller_pid2);
  TestCompareLargerID(wid, larger_pid2);
  
  std::cout << "Testing comparison with different TPC ID (1)" << std::endl;
  geo::WireID smaller_tid1(1, 14, 32, 28), larger_tid1(1, 16, 32, 26);
  TestCompareSmallerID(wid, smaller_tid1);
  TestCompareLargerID(wid, larger_tid1);
  std::cout << "Testing comparison with different TPC ID (2)" << std::endl;
  geo::WireID smaller_tid2(1, 14, 32, 27), larger_tid2(1, 16, 32, 27);
  TestCompareSmallerID(wid, smaller_tid2);
  TestCompareLargerID(wid, larger_tid2);
  
  std::cout << "Testing comparison with different cryostat ID" << std::endl;
  geo::WireID smaller_cid1(0, 15, 32, 28), larger_cid1(2, 15, 32, 26);
  TestCompareSmallerID(wid, smaller_cid1);
  TestCompareLargerID(wid, larger_cid1);
  std::cout << "Testing comparison with different cryostat ID (2)" << std::endl;
  geo::WireID smaller_cid2(0, 15, 32, 27), larger_cid2(2, 15, 32, 27);
  TestCompareSmallerID(wid, smaller_cid2);
  TestCompareLargerID(wid, larger_cid2);
  
  // make sure the ID with TPC 0 is fine (it's not a bad ID!)
  std::cout << "Testing Plane ID constructed with a Plane #0" << std::endl;
  
  geo::WireID first_wid(0, 0, 0, 0);
  TestIDvalidity(first_wid, true);
  
  // - check the ID value
  BOOST_CHECK_EQUAL(first_wid.Cryostat, geo::CryostatID::CryostatID_t(0));
  BOOST_CHECK_EQUAL(first_wid.TPC,                geo::TPCID::TPCID_t(0));
  BOOST_CHECK_EQUAL(first_wid.Plane,          geo::PlaneID::PlaneID_t(0));
  BOOST_CHECK_EQUAL(first_wid.Wire,             geo::WireID::WireID_t(0));
  
} // test_WireID_directConstructor()



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
  test_TPCID_nestedConstructor();
  test_TPCID_directConstructor();
}

//
// PlaneID test
//
BOOST_AUTO_TEST_CASE(PlaneIDtest) {
  test_PlaneID_defaultConstructor();
  test_PlaneID_nestedConstructor();
  test_PlaneID_directConstructor();
}

//
// WireID test
//
BOOST_AUTO_TEST_CASE(WireIDtest) {
  test_WireID_defaultConstructor();
  test_WireID_nestedConstructor();
  test_WireID_directConstructor();
}

