/**
 * @file   DumpUtils_test.cc
 * @brief  Unit test for utilities in DumpUtils.h
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   February 28, 2017
 * 
 * 
 */

// Boost libraries
#define BOOST_TEST_MODULE ( RealComparisons_test )
#include <cetlib/quiet_unit_test.hpp> // BOOST_AUTO_TEST_CASE()
#include <boost/test/test_tools.hpp> // BOOST_CHECK_EQUAL()

// LArSoft libraries
#include "larcore/CoreUtils/DumpUtils.h"

// ROOT libraries
#include <TVector3.h>

// C/C++ standard libraries
#include <iostream>
#include <string>
#include <sstream>


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
BOOST_AUTO_TEST_CASE(test_DumpVectors) {
  
  Vector3DdocumentationTest();
  
  Vector3DspecializationTest();
  
} // BOOST_AUTO_TEST_CASE(test_DumpVectors)

