/**
 * @file   DumpUtils_test.cc
 * @brief  Unit test for utilities in DumpUtils.h
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   February 28, 2017
 * 
 * 
 */

// Boost libraries
#define BOOST_TEST_MODULE DumpUtils_test
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "larcore/CoreUtils/DumpUtils.h"

// ROOT libraries
#include <TVector3.h>

// C/C++ standard libraries
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <sstream>


//------------------------------------------------------------------------------
void ArrayTest_STLarray() {
  std::ostringstream sstr;
  std::array<float, 5> const array = { 1., 2., 3., 4., 6. };
  sstr << lar::dump::array<5>(array);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
}

void ArrayTest_STLvector() {
  std::ostringstream sstr;
  std::vector<float> const array = { 1., 2., 3., 4., 6. };
  sstr << lar::dump::array<5>(array);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
}

void ArrayTest_carray() {
  std::ostringstream sstr;
  float const array[5] = { 1., 2., 3., 4., 6. };
  sstr << lar::dump::array<5>(array);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
}

void ArrayTest_cptr() {
  std::ostringstream sstr;
  float const array[5] = { 1., 2., 3., 4., 6. };
  float const* ptr = array;
  sstr << lar::dump::array<5>(ptr);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
}

void ArrayTest_array() {
  std::ostringstream sstr;
  double array[5] = { 1., 2., 3., 4., 6. };
  sstr << lar::dump::array<5>(array);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
}

void ArrayTest_ptr() {
  std::ostringstream sstr;
  double array[5] = { 1., 2., 3., 4., 6. };
  double* ptr = array;
  sstr << lar::dump::array<5>(ptr);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
}

void ArrayTest() {
  
  ArrayTest_STLarray();
  ArrayTest_STLvector();
  ArrayTest_array();
  ArrayTest_carray();
  ArrayTest_ptr();
  ArrayTest_cptr();
  
} // Vector3Dtest()


//------------------------------------------------------------------------------
void ArrayDocumentationTest() {
  
  std::ostringstream sstr;
  
  /* This is the code as in the documentation:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * double const data[5] = { 1., 2., 3., 4., 6. };
   * std::cout << "Position: " << lar::dump::array<5>(data) << std::endl;
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * which is promised to return:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * Position: { 1; 2; 3; 4; 6 }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   */

  double const data[5] = { 1., 2., 3., 4., 6. };
  std::cout << "Position: " << lar::dump::array<5>(data) << std::endl;

  sstr << lar::dump::array<5>(data);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 3; 4; 6 }");
  
} // ArrayDocumentationTest()


//------------------------------------------------------------------------------
void Vector3Dtest_TVector3() {
  std::ostringstream sstr;
  TVector3 pos( 1., 2., 4. );
  sstr << lar::dump::vector3D(pos);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 4 }");
}

void Vector3Dtest_carray() {
  std::ostringstream sstr;
  float const array[3] = { 1., 2., 4. };
  sstr << lar::dump::vector3D(array);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 4 }");
}

void Vector3Dtest_cptr() {
  std::ostringstream sstr;
  float const array[3] = { 1., 2., 4. };
  float const* ptr = array;
  sstr << lar::dump::vector3D(ptr);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 4 }");
}

void Vector3Dtest_array() {
  std::ostringstream sstr;
  double array[3] = { 1., 2., 4. };
  sstr << lar::dump::vector3D(array);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 4 }");
}

void Vector3Dtest_ptr() {
  std::ostringstream sstr;
  double array[3] = { 1., 2., 4. };
  double* ptr = array;
  sstr << lar::dump::vector3D(ptr);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 1; 2; 4 }");
}


void Vector3Dtest() {
  
  Vector3Dtest_TVector3();
  Vector3Dtest_array();
  Vector3Dtest_carray();
  Vector3Dtest_ptr();
  Vector3Dtest_cptr();
  
} // Vector3Dtest()


//------------------------------------------------------------------------------
void Vector3DdocumentationTest() {
  
  std::ostringstream sstr;
  
  /* This is the code as in the documentation:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * TVector3 pos;
   * std::cout << "Position: " << lar::dump::vector3D(pos) << std::endl;
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * which is promised to return:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * Position: { 0, 0, 0 }
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   */

  TVector3 pos;
  std::cout << "Position: " << lar::dump::vector3D(pos) << std::endl;

  sstr << lar::dump::vector3D(pos);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 0; 0; 0 }");
  
} // Vector3DdocumentationTest()


//------------------------------------------------------------------------------

#include <array>

namespace lar {
  namespace dump {
    
    // specialization for 3D std::array<>
    template <typename T>
    struct VectorDumper<std::array<T, 3U>> {
      
      using Vector_t = std::array<T, 3U>;
      
      Vector_t const& a;
      
      VectorDumper(Vector_t const& a): a(a) {}
      
      template <typename Stream>
      void operator() (Stream&& out) const
        { out << "{ " << a[0] << "; " << a[1] << "; " << a[2] << " }"; }
      
    }; // struct VectorDumper<array>
    
    
  } // namespace dump
} // namespace lar


void Vector3DspecializationTest() {
  
  std::ostringstream sstr;

  std::array<float, 3U> v;
  v.fill(0.0);
  sstr << lar::dump::vector3D(v);
  BOOST_CHECK_EQUAL(sstr.str(), "{ 0; 0; 0 }");
  
} // Vector3DspecializationTest()



//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_DumpArrays) {
  
  ArrayTest();
  
  ArrayDocumentationTest();
  
} // BOOST_AUTO_TEST_CASE(test_DumpVectors)


BOOST_AUTO_TEST_CASE(test_DumpVectors) {
  
  Vector3Dtest();
  
  Vector3DdocumentationTest();
  
  Vector3DspecializationTest();
  
} // BOOST_AUTO_TEST_CASE(test_DumpVectors)

