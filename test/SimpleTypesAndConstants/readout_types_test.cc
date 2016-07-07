/**
 * @file   readout_types_test.cc
 * @brief  Test of readout_types.h types
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   June 19th, 2015
 */

// Define the following non-zero to exclude include code that is required
// not to be compilable
#ifndef READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS
#  define READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS 1
#endif // !READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS

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
#define BOOST_TEST_MODULE ( readout_types_test )
#include <boost/test/auto_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK(), BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "larcore/SimpleTypesAndConstants/readout_types.h"


//------------------------------------------------------------------------------
// compile-time tests:
//
// IDs must not be convertible to integers
static_assert(
  !std::is_convertible
    <readout::CryostatID, readout::CryostatID::CryostatID_t>::value,
  "readout::CryostatID can be implicitly converted to an integral type"
  );

static_assert(
  !std::is_convertible
    <readout::TPCsetID, readout::CryostatID::CryostatID_t>::value,
  "readout::TPCsetID can be implicitly converted to an integral type"
  );
static_assert(
  !std::is_convertible<readout::TPCsetID, readout::TPCsetID::TPCsetID_t>::value,
  "readout::TPCsetID can be implicitly converted to an integral type"
  );

static_assert(
  !std::is_convertible
    <readout::ROPID, readout::CryostatID::CryostatID_t>::value,
  "readout::ROPID can be implicitly converted to an integral type"
  );
static_assert(
  !std::is_convertible<readout::ROPID, readout::TPCsetID::TPCsetID_t>::value,
  "readout::ROPID can be implicitly converted to an integral type"
  );
static_assert(
  !std::is_convertible<readout::ROPID, readout::ROPID::ROPID_t>::value,
  "readout::ROPID can be implicitly converted to an integral type"
  );


//------------------------------------------------------------------------------
// run-time tests:
//
void TestIDvalidity(readout::CryostatID const& id, bool answer) {
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
  
  BOOST_MESSAGE("Testing default-constructed cryostat ID");
  
  readout::CryostatID cid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(cid, false);
  
/* // feature not added
  // test assignment from ID_t
  cid = 1;
  BOOST_CHECK_EQUAL(cid.Cryostat, 1);
*/
  
} // test_CryostatID_defaultConstructor()


void test_CryostatID_directConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing cryostat ID constructed with an integer");
  
  readout::CryostatID cid(1);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(cid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(cid.Cryostat, readout::CryostatID::CryostatID_t(1));
  
  // test comparison operators
  // (exercise copy constructor too)
  readout::CryostatID smaller_cid(0), same_cid(cid), larger_cid(2);
  
  TestIDcomparison(cid, smaller_cid, same_cid, larger_cid);
  
  
  // make sure the ID with cryostat 0 is fine (it's not a bad ID!)
  BOOST_TEST_CHECKPOINT("Testing cryostat ID constructed with an integer 0");
  
  readout::CryostatID first_cid(0);
  TestIDvalidity(cid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(first_cid.Cryostat, readout::CryostatID::CryostatID_t(0));
  
} // test_CryostatID_directConstructor()



void test_TPCsetID_defaultConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing default-constructed TPC set ID");
  
  readout::TPCsetID sid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(sid, false);
  
} // test_TPCsetID_defaultConstructor()


void test_TPCsetID_integralConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing integral-constructed TPC set ID");
  
#if READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS
  BOOST_TEST_MESSAGE("  (test skipped)");
#else
  readout::TPCsetID sid(1);
  
  BOOST_TEST_MESSAGE("TPCsetID(1) = " << std::string(sid));
  
  readout::TPCsetID::TPCsetID_t what = sid;
  
  BOOST_TEST_MESSAGE("int(TPCsetID(1)) = " << what);
  
#endif // READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS
  
} // test_TPCsetID_integralConstructor()


void test_TPCsetID_nestedConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing ID-constructed TPC set ID");
  
  readout::CryostatID cid(1);
  readout::TPCsetID sid(cid, 15);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(sid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(sid.Cryostat, readout::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(sid.TPCset,       readout::TPCsetID::TPCsetID_t(15));
  
  // test comparison operators (exercise copy constructor too)
  // - with TPC ID
  BOOST_TEST_CHECKPOINT("Testing comparison with TPC set ID");
  readout::TPCsetID smaller_sid(cid, sid.TPCset - 1), same_sid(sid),
    larger_sid(cid, sid.TPCset + 1);
  
  TestIDcomparison(sid, smaller_sid, same_sid, larger_sid);
  
} // test_TPCsetID_nestedConstructor()
 

void test_TPCsetID_directConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing TPC set ID constructed with indices");
  
  readout::TPCsetID sid(1, 15);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(sid, true);
  
  BOOST_TEST_CHECKPOINT("Testing comparison with same cryostat ID");
  
  readout::TPCsetID smaller_sid(1, 14), same_sid(1, 15), larger_sid(1, 16);
  TestIDcomparison(sid, smaller_sid, same_sid, larger_sid);
  
  BOOST_TEST_CHECKPOINT("Testing comparison with different cryostat ID");
  readout::TPCsetID smaller_cid(0, 16), larger_cid(2, 14);
  TestCompareSmallerID(sid, smaller_cid);
  TestCompareLargerID(sid, larger_cid);
  
  // make sure the ID with TPC set 0 is fine (it's not a bad ID!)
  BOOST_TEST_CHECKPOINT("Testing TPC set ID constructed with a TPC set #0");
  
  readout::TPCsetID first_sid(0, 0);
  TestIDvalidity(first_sid, true);
  
  // - check the ID value
  BOOST_CHECK_EQUAL(first_sid.Cryostat, readout::CryostatID::CryostatID_t(0));
  BOOST_CHECK_EQUAL(first_sid.TPCset,       readout::TPCsetID::TPCsetID_t(0));
  
  
} // test_TPCsetID_directConstructor()



void test_ROPID_defaultConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing default-constructed ROP ID");
  
  readout::ROPID rid;
  
  // a default-constructed ID is invalid:
  TestIDvalidity(rid, false);
  
} // test_ROPID_defaultConstructor()


void test_ROPID_integralConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing integral-constructed ROP ID");
  
#if READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS
  BOOST_TEST_MESSAGE("  (test skipped)");
#else
  readout::ROPID rid(1);
  
  BOOST_TEST_MESSAGE("ROPID(1) = " << std::string(rid));
  
  readout::ROPID::ROPID_t what = rid;
  
  BOOST_TEST_MESSAGE("int(ROPID(1)) = " << what);
  
#endif // READOUT_TYPES_TEST_SKIP_COMPILATION_ERRORS
  
} // test_ROPID_integralConstructor()


void test_ROPID_nestedConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing ID-constructed ROP ID");
  
  readout::TPCsetID sid(1, 15);
  readout::ROPID rid(sid, 32);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(rid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(rid.Cryostat, readout::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(rid.TPCset,       readout::TPCsetID::TPCsetID_t(15));
  BOOST_CHECK_EQUAL(rid.ROP,                readout::ROPID::ROPID_t(32));
  
  // test comparison operators (exercise copy constructor too)
  BOOST_TEST_CHECKPOINT("Testing comparison with ROP ID");
  readout::ROPID smaller_rid(sid, rid.ROP - 1), same_rid(rid),
    larger_rid(sid, rid.ROP + 1);
  
  TestIDcomparison(rid, smaller_rid, same_rid, larger_rid);
  
} // test_ROPID_nestedConstructor()
 

void test_ROPID_directConstructor() {
  
  BOOST_TEST_CHECKPOINT("Testing ROP ID constructed with indices");
  
  readout::ROPID rid(1, 15, 32);
  
  // an explicitly constructed ID is valid:
  TestIDvalidity(rid, true);
  
  // check the ID value
  BOOST_CHECK_EQUAL(rid.Cryostat, readout::CryostatID::CryostatID_t( 1));
  BOOST_CHECK_EQUAL(rid.TPCset,       readout::TPCsetID::TPCsetID_t(15));
  BOOST_CHECK_EQUAL(rid.ROP,                readout::ROPID::ROPID_t(32));
  
  BOOST_TEST_CHECKPOINT("Testing comparison with same TPC set ID");
  
  readout::ROPID
    smaller_rid(1, 15, 31), same_rid(1, 15, 32), larger_rid(1, 15, 33);
  TestIDcomparison(rid, smaller_rid, same_rid, larger_rid);
  
  BOOST_TEST_CHECKPOINT("Testing comparison with different TPC set ID (1)");
  readout::ROPID smaller_sid1(1, 14, 33), larger_sid1(1, 16, 31);
  TestCompareSmallerID(rid, smaller_sid1);
  TestCompareLargerID(rid, larger_sid1);
  BOOST_TEST_CHECKPOINT("Testing comparison with different TPC set ID (2)");
  readout::ROPID smaller_sid2(1, 14, 32), larger_sid2(1, 16, 32);
  TestCompareSmallerID(rid, smaller_sid2);
  TestCompareLargerID(rid, larger_sid2);
  
  BOOST_TEST_CHECKPOINT("Testing comparison with different cryostat ID");
  readout::ROPID smaller_cid1(0, 15, 33), larger_cid1(2, 15, 31);
  TestCompareSmallerID(rid, smaller_cid1);
  TestCompareLargerID(rid, larger_cid1);
  BOOST_TEST_CHECKPOINT("Testing comparison with different cryostat ID (2)");
  readout::ROPID smaller_cid2(0, 15, 32), larger_cid2(2, 15, 32);
  TestCompareSmallerID(rid, smaller_cid2);
  TestCompareLargerID(rid, larger_cid2);
  
  // make sure the ID with TPC 0 is fine (it's not a bad ID!)
  BOOST_TEST_CHECKPOINT("Testing ROP ID constructed with a ROP #0");
  
  readout::ROPID first_rid(0, 0, 0);
  TestIDvalidity(first_rid, true);
  
  // - check the ID value
  BOOST_CHECK_EQUAL(first_rid.Cryostat, readout::CryostatID::CryostatID_t(0));
  BOOST_CHECK_EQUAL(first_rid.TPCset,       readout::TPCsetID::TPCsetID_t(0));
  BOOST_CHECK_EQUAL(first_rid.ROP,                readout::ROPID::ROPID_t(0));
  
} // test_ROPID_directConstructor()




//
// CryostatID test
//
BOOST_AUTO_TEST_CASE(CryostatIDtest) {
  test_CryostatID_defaultConstructor();
  test_CryostatID_directConstructor();
}

//
// TPCsetID test
//
BOOST_AUTO_TEST_CASE(TPCsetIDtest) {
  test_TPCsetID_defaultConstructor();
  test_TPCsetID_nestedConstructor();
  test_TPCsetID_directConstructor();
  test_TPCsetID_integralConstructor();
}

//
// ROPID test
//
BOOST_AUTO_TEST_CASE(ROPIDtest) {
  test_ROPID_defaultConstructor();
  test_ROPID_nestedConstructor();
  test_ROPID_directConstructor();
  test_ROPID_integralConstructor();
}

