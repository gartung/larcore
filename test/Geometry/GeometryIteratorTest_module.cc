/**
 * @file   GeometryIteratorTest_module.cc
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
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"

// Framework includes
#include "cetlib/exception.h"
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
    if (nErrors) {
      mf::LogError("GeometryIteratorTest") << nErrors << " errors collected!";
      throw cet::exception("GeometryIteratorTest")
        << "geometry iterator test failed with " << nErrors << " errors\n";
    }
    else {
      mf::LogError("GeometryIteratorTest")
        << "Test was completed with no errors";
    }
  } // GeometryIteratorTest::beginJob()


  //......................................................................
  unsigned int GeometryIteratorTest::testTPCiterator() {
    art::ServiceHandle<geo::Geometry> geom;
    const unsigned int nCryo = geom->Ncryostats(); 
    LOG_VERBATIM("GeometryIteratorTest") << "We have " << nCryo << " cryostats";
    
    unsigned int nErrors = 0;
    geo::Geometry::cryostat_iterator iCryostat;
    geo::Geometry::TPC_iterator iTPC;
    geo::Geometry::plane_iterator iPlane;
    geo::Geometry::wire_iterator iWire;
    
    for(unsigned int c = 0; c < nCryo; ++c) {
      const CryostatGeo& cryo(geom->Cryostat(c));
      const unsigned int nTPC = cryo.NTPC();
    
      LOG_DEBUG("GeometryIteratorTest") << "  C=" << c
        << " (" << nTPC << " TPCs)";
      
      if (!iCryostat) {
        LOG_ERROR("GeometryIteratorTest")
          << "Cryostat iterator thinks it's all over at C=" << c;
        ++nErrors;
      }
      else if (*iCryostat != c) {
        LOG_ERROR("GeometryIteratorTest")
          << "Cryostat iterator thinks it's at C=" << (*iCryostat)
          << " instead of " << c;
        ++nErrors;
      }
      else if (iCryostat.get() != &cryo) {
        LOG_ERROR("GeometryIteratorTest")
          << "Cryostat iterator retrieves CryostatGeo["
          << ((void*) iCryostat.get())
          << "] instead of [" << ((void*) &cryo) << "]";
        ++nErrors;
      }
      
      
      for(unsigned int t = 0; t < nTPC; ++t){
        const TPCGeo& TPC(cryo.TPC(t));
        const unsigned int NPlanes = TPC.Nplanes();
        
        LOG_DEBUG("GeometryIteratorTest") << "    C=" << c << " T=" << t
          << " (" << NPlanes << " planes)";
        if (!iTPC) {
          LOG_ERROR("GeometryIteratorTest")
            << "TPC iterator thinks it's all over at C=" << c << " T=" << t;
          ++nErrors;
        }
        else if (iTPC->Cryostat != c) {
          LOG_ERROR("GeometryIteratorTest")
            << "TPC iterator thinks it's at C=" << iTPC->Cryostat
            << " instead of " << c;
          ++nErrors;
        }
        else if (iTPC->TPC != t) {
          LOG_ERROR("GeometryIteratorTest")
            << "TPC iterator thinks it's at T=" << iTPC->TPC << " instead of "
            << t;
          ++nErrors;
        }
        else if (iTPC.get() != &TPC) {
          LOG_ERROR("GeometryIteratorTest")
            << "TPC iterator retrieves TPCGeo[" << ((void*) iTPC.get())
            << "] instead of [" << ((void*) &TPC) << "]";
          ++nErrors;
        }
        
        for(unsigned int p = 0; p < NPlanes; ++p) {
          const PlaneGeo& Plane(TPC.Plane(p));
          const unsigned int NWires = Plane.Nwires();
          
          LOG_DEBUG("GeometryIteratorTest") << "    C=" << c << " T=" << t
            << " P=" << p << " (" << NWires << " wires)";
          if (!iPlane) {
            LOG_ERROR("GeometryIteratorTest")
              << "plane iterator thinks it's all over at C=" << c << " T=" << t
              << " P=" << p;
            ++nErrors;
          }
          else if (iPlane->Cryostat != c) {
            LOG_ERROR("GeometryIteratorTest")
              << "plane iterator thinks it's at C=" << iPlane->Cryostat
              << " instead of " << c;
            ++nErrors;
          }
          else if (iPlane->TPC != t) {
            LOG_ERROR("GeometryIteratorTest")
              << "plane iterator thinks it's at T=" << iPlane->TPC
              << " instead of " << t;
            ++nErrors;
          }
          else if (iPlane->Plane != p) {
            LOG_ERROR("GeometryIteratorTest")
              << "plane iterator thinks it's at P=" << iPlane->Plane
              << " instead of " << p;
            ++nErrors;
          }
          else if (iPlane.get() != &Plane) {
            LOG_ERROR("GeometryIteratorTest")
              << "plane iterator retrieves TPCGeo[" << ((void*) iPlane.get())
              << "] instead of [" << ((void*) &Plane) << "]";
            ++nErrors;
          }
          
          
          for(unsigned int w = 0; w < NWires; ++w) {
            const WireGeo& Wire(Plane.Wire(w));
            
            LOG_DEBUG("GeometryIteratorTest") << "    C=" << c << " T=" << t
              << " P=" << p << " W=" << w;
            if (!iWire) {
              LOG_ERROR("GeometryIteratorTest")
                << "wire iterator thinks it's all over at C=" << c
                << " T=" << t << " P=" << p << " W=" << w;
              ++nErrors;
            }
            else if (iWire->Cryostat != c) {
              LOG_ERROR("GeometryIteratorTest")
                << "wire iterator thinks it's at C=" << iWire->Cryostat
                << " instead of " << c;
              ++nErrors;
            }
            else if (iWire->TPC != t) {
              LOG_ERROR("GeometryIteratorTest")
                << "wire iterator thinks it's at T=" << iWire->TPC
                << " instead of " << t;
              ++nErrors;
            }
            else if (iWire->Plane != p) {
              LOG_ERROR("GeometryIteratorTest")
                << "wire iterator thinks it's at P=" << iWire->Plane
                << " instead of " << p;
              ++nErrors;
            }
            else if (iWire->Wire != w) {
              LOG_ERROR("GeometryIteratorTest")
                << "wire iterator thinks it's at W=" << iWire->Wire
                << " instead of " << w;
              ++nErrors;
            }
            else if (iWire.get() != &Wire) {
              LOG_ERROR("GeometryIteratorTest")
                << "wire iterator retrieves TPCGeo[" << ((void*) iWire.get())
                << "] instead of [" << ((void*) &Plane) << "]";
              ++nErrors;
            }
          
            ++iWire;
          } // end loop over wires
          ++iPlane;
        } // end loop over planes
        ++iTPC;
      } // end loop over tpcs
      ++iCryostat;
    } // end loop over cryostats
    
    if (iCryostat) {
      LOG_ERROR("GeometryIteratorTest")
        << "Cryostat iterator thinks it's still at C=" << *iCryostat
        << ", but we are already over";
      ++nErrors;
    }
    
    if (iTPC) {
      LOG_ERROR("GeometryIteratorTest")
        << "TPC iterator thinks it's still at C=" << iTPC->Cryostat
        << " T=" << iTPC->TPC << ", but we are already over";
      ++nErrors;
    }
    
    if (iPlane) {
      LOG_ERROR("GeometryIteratorTest")
        << "plane iterator thinks it's still at C=" << iPlane->Cryostat
        << " T=" << iPlane->TPC << " P=" << iPlane->Plane
        << ", but we are already over";
      ++nErrors;
    }
    
    if (iWire) {
      LOG_ERROR("GeometryIteratorTest")
        << "wire iterator thinks it's still at C=" << iWire->Cryostat
        << " T=" << iWire->TPC << " P=" << iWire->Plane << " W=" << iWire->Wire
        << ", but we are already over";
      ++nErrors;
    }
    
    return nErrors;
  } // GeometryIteratorTest::testTPCiterator()


  DEFINE_ART_MODULE(GeometryIteratorTest)

} // namespace geo
