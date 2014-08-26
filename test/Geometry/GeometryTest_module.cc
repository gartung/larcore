/**
 * @file   GeometryIteratorTest.cc
 * @brief  Tests the correct iteration of the geo::Geometry iterators
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   August 25, 2014
 */



#include <cmath>
#include <vector>
#include <string>
#include <iostream>

// LArSoft includes
#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/Geometry.h"

// Framework includes
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"


namespace geo {
  class GeometryIteratorTest: public art::EDAnalyzer {
      public:
    explicit GeometryIteratorTest(fhicl::ParameterSet const& pset);

    virtual void analyze(art::Event const&) {}
    virtual void beginJob();

      private:

    unsigned int testTPCiterator();
  }; // class GeometryIteratorTest
} // namespace geo


//------------------------------------------------------------------------------
namespace geo{

  //......................................................................
  GeometryIteratorTest::GeometryIteratorTest(fhicl::ParameterSet const& pset) 
    : EDAnalyzer(pset)
    {}


  //......................................................................
  void GeometryIteratorTest::beginJob() {
    unsigned int nErrors = 0;
    try{
      nErrors += testTPCiterator();

    }
    catch (cet::exception &e) {
      mf::LogWarning("GeometryIteratorTest") << "exception caught: \n" << e;
      ++nErrors;
    }
    if (nErrors)
      mf::LogError("GeometryIteratorTest") << nErrors << " errors collected!";
    else {
      mf::LogError("GeometryIteratorTest")
        << "Test was completed with no errors";
    }
  } // GeometryIteratorTest::beginJob()


  //......................................................................
  unsigned int GeometryIteratorTest::testTPCiterator() {
    art::ServiceHandle<geo::Geometry> geom;
    
    unsigned int nErrors = 0;
    geo::Geometry::TPC_iterator iTPC;
    for(unsigned int c = 0; c < geom->Ncryostats(); ++c){
      for(unsigned int t = 0; t < geom->Cryostat(c).NTPC(); ++t){
//        for(unsigned int p = 0; p < geom->Cryostat(cs).TPC(t).Nplanes(); ++p){
//          for(unsigned int w = 0; w < geom->Cryostat(cs).TPC(t).Plane(p).Nwires(); ++w){
        if (!iTPC) {
          LOG_ERROR("GeometryIteratorTest")
            << "Iterator thinks it's all over at C=" << c << " T=" << t;
        }
        else if (iTPC->Cryostat != c) {
          LOG_ERROR("GeometryIteratorTest")
            << "Iterator thinks it's at C=" << iTPC->Cryostat << " instead of "
            << c;
        }
        else if (iTPC->TPC != t) {
          LOG_ERROR("GeometryIteratorTest")
            << "Iterator thinks it's at T=" << iTPC->TPC << " instead of "
            << t;
        }
        
        ++iTPC;
            } // if good lookup fails
//          } // end loop over wires
//        } // end loop over planes
      } // end loop over tpcs
    } // end loop over cryostats

    
  } // GeometryIteratorTest::testTPCiterator()


  DEFINE_ART_MODULE(GeometryIteratorTest)

} // namespace geo
