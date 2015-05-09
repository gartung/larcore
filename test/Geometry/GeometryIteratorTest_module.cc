/**
 * @file   GeometryIteratorTest_module.cc
 * @brief  Tests the correct iteration of the geo::Geometry iterators
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   August 25, 2014
 */

// LArSoft includes
#include "test/Geometry/GeometryIteratorTestAlg.h"
#include "Geometry/Geometry.h"

// Framework includes
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

// C/C++ standard library
#include <memory> // std::unique_ptr<>


namespace art { class Event; } // art::Event declaration

namespace geo {
  /**
   * @brief Performs tests on the geometry as seen by Geometry service
   * 
   * Configuration parameters
   * =========================
   * 
   * See GeometryIteratorTestAlg.
   */
  class GeometryIteratorTest: public art::EDAnalyzer {
      public:
    explicit GeometryIteratorTest(fhicl::ParameterSet const& pset);
    
    virtual void analyze(art::Event const&) {}
    virtual void beginJob();
    
      private:
    
    std::unique_ptr<geo::GeometryIteratorTestAlg> tester; ///< the test algorithm
    
  }; // class GeometryIteratorTest
} // namespace geo


//******************************************************************************
namespace geo {
  
  //......................................................................
  GeometryIteratorTest::GeometryIteratorTest(fhicl::ParameterSet const& pset)
    : EDAnalyzer(pset)
    , tester(new geo::GeometryIteratorTestAlg(pset))
  {
  } // GeometryIteratorTest::GeometryIteratorTest()
  
  
  //......................................................................
  void GeometryIteratorTest::beginJob()
  {
    art::ServiceHandle<geo::Geometry> geom;
    
    // 1. we set it up with the geometry from the environment
    tester->Setup(*geom);
    
    // 2. then we run it!
    tester->Run();
    
  } // GeometryTest::beginJob()
  
  
  //......................................................................
  DEFINE_ART_MODULE(GeometryIteratorTest)
  
} // namespace geo
