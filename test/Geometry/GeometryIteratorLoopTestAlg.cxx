/**
 * @file   GeometryIteratorLoopTestAlg.cxx
 * @brief  Tests the correct iteration of the geo::Geometry iterators
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   August 25, 2014
 */

// our header
#include "GeometryIteratorLoopTestAlg.h"

// LArSoft includes
#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/GeometryCore.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"

// Framework includes
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"


namespace geo {

  //......................................................................
  unsigned int GeometryIteratorLoopTestAlg::Run() {
    const unsigned int nCryo = geom->Ncryostats(); 
    LOG_VERBATIM("GeometryIteratorLoopTest") << "We have " << nCryo << " cryostats";
    
    unsigned int nErrors = 0;
    unsigned int nCryostats = 0, nTPCs = 0, nPlanes = 0, nWires = 0;
    geo::GeometryCore::cryostat_iterator iCryostat = geom->begin_cryostat();
    geo::GeometryCore::TPC_iterator iTPC(geom);
    geo::GeometryCore::plane_iterator iPlane(geom);
    geo::GeometryCore::wire_iterator iWire(geom);
    
    for(unsigned int c = 0; c < nCryo; ++c) {
      const CryostatGeo& cryo(geom->Cryostat(c));
      const unsigned int nTPC = cryo.NTPC();
    
      LOG_DEBUG("GeometryIteratorLoopTest") << "  C=" << c
        << " (" << nTPC << " TPCs)";
      
      if (!iCryostat) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator thinks it's all over at C=" << c;
        ++nErrors;
      }
      else if (iCryostat->Cryostat != c) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator thinks it's at C=" << (*iCryostat)
          << " instead of " << c;
        ++nErrors;
      }
      else if (iCryostat.get() != &cryo) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator retrieves CryostatGeo["
          << ((void*) iCryostat.get())
          << "] instead of [" << ((void*) &cryo) << "]";
        ++nErrors;
      }
      
      
      for(unsigned int t = 0; t < nTPC; ++t){
        const TPCGeo& TPC(cryo.TPC(t));
        const unsigned int NPlanes = TPC.Nplanes();
        
        LOG_DEBUG("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
          << " (" << NPlanes << " planes)";
        if (!iTPC) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator thinks it's all over at C=" << c << " T=" << t;
          ++nErrors;
        }
        else if (iTPC->Cryostat != c) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator thinks it's at C=" << iTPC->Cryostat
            << " instead of " << c;
          ++nErrors;
        }
        else if (iTPC->TPC != t) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator thinks it's at T=" << iTPC->TPC << " instead of "
            << t;
          ++nErrors;
        }
        else if (iTPC.get() != &TPC) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator retrieves TPCGeo[" << ((void*) iTPC.get())
            << "] instead of [" << ((void*) &TPC) << "]";
          ++nErrors;
        }
        
        for(unsigned int p = 0; p < NPlanes; ++p) {
          const PlaneGeo& Plane(TPC.Plane(p));
          const unsigned int NWires = Plane.Nwires();
          
          LOG_DEBUG("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
            << " P=" << p << " (" << NWires << " wires)";
          if (!iPlane) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's all over at C=" << c << " T=" << t
              << " P=" << p;
            ++nErrors;
          }
          else if (iPlane->Cryostat != c) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's at C=" << iPlane->Cryostat
              << " instead of " << c;
            ++nErrors;
          }
          else if (iPlane->TPC != t) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's at T=" << iPlane->TPC
              << " instead of " << t;
            ++nErrors;
          }
          else if (iPlane->Plane != p) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's at P=" << iPlane->Plane
              << " instead of " << p;
            ++nErrors;
          }
          else if (iPlane.get() != &Plane) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator retrieves TPCGeo[" << ((void*) iPlane.get())
              << "] instead of [" << ((void*) &Plane) << "]";
            ++nErrors;
          }
          
          
          for(unsigned int w = 0; w < NWires; ++w) {
            const WireGeo& Wire(Plane.Wire(w));
            
            LOG_DEBUG("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
              << " P=" << p << " W=" << w;
            if (!iWire) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's all over at C=" << c
                << " T=" << t << " P=" << p << " W=" << w;
              ++nErrors;
            }
            else if (iWire->Cryostat != c) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at C=" << iWire->Cryostat
                << " instead of " << c;
              ++nErrors;
            }
            else if (iWire->TPC != t) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at T=" << iWire->TPC
                << " instead of " << t;
              ++nErrors;
            }
            else if (iWire->Plane != p) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at P=" << iWire->Plane
                << " instead of " << p;
              ++nErrors;
            }
            else if (iWire->Wire != w) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at W=" << iWire->Wire
                << " instead of " << w;
              ++nErrors;
            }
            else if (iWire.get() != &Wire) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator retrieves TPCGeo[" << ((void*) iWire.get())
                << "] instead of [" << ((void*) &Plane) << "]";
              ++nErrors;
            }
          
            ++iWire;
            ++nWires;
          } // end loop over wires
          ++iPlane;
          ++nPlanes;
        } // end loop over planes
        ++iTPC;
        ++nTPCs;
      } // end loop over tpcs
      ++iCryostat;
      ++nCryostats;
    } // end loop over cryostats
    
    if (iCryostat) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Cryostat iterator thinks it's still at " << *iCryostat
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all cryostats with the iterators (via iterator box)
    unsigned int nLoopedCryostats = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nCryostats << " cryostats";
    for (geo::CryostatID const& cID: geom->IterateCryostats()) {
      LOG_TRACE("GeometryIteratorsDump") << cID;
      if (nLoopedCryostats >= nCryostats) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedCryostats
          << " cryostats, iterator has not reached the end ("
          << *(geom->end_cryostat()) << ") but it's still at " << cID;
        ++nErrors;
        break;
      }
      ++nLoopedCryostats;
    }
    if (nLoopedCryostats < nCryostats) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedCryostats
        << " cryostats, while we expected " << nCryostats << " iterations!";
      ++nErrors;
    } // if
    
    if (iTPC) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "TPC iterator thinks it's still at C=" << iTPC->Cryostat
        << " T=" << iTPC->TPC << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all TPCs with the iterators (via iterator box)
    unsigned int nLoopedTPCs = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nTPCs << " TPCs";
    for (geo::TPCID const& tID: geom->IterateTPCs()) {
      LOG_TRACE("GeometryIteratorsDump") << tID;
      if (nLoopedTPCs >= nTPCs) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedTPCs
          << " TPCs, iterator has not reached the end ("
          << *(geom->end_TPC()) << ") but it's still at " << tID;
        ++nErrors;
        break;
      }
      ++nLoopedTPCs;
    }
    if (nLoopedTPCs < nTPCs) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedTPCs
        << " TPCs, while we expected " << nTPCs << " iterations!";
      ++nErrors;
    } // if
    
    if (iPlane) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "plane iterator thinks it's still at C=" << iPlane->Cryostat
        << " T=" << iPlane->TPC << " P=" << iPlane->Plane
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all planes with the iterators (via iterator box)
    unsigned int nLoopedPlanes = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nPlanes << " planes";
    for (geo::PlaneID const& pID: geom->IteratePlanes()) {
      LOG_TRACE("GeometryIteratorsDump") << pID;
      if (nLoopedPlanes >= nPlanes) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedPlanes
          << " planes, iterator has not reached the end ("
          << *(geom->end_plane()) << ") but it's still at " << pID;
        ++nErrors;
        break;
      }
      ++nLoopedPlanes;
    }
    if (nLoopedPlanes < nPlanes) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedPlanes
        << " planes, while we expected " << nPlanes << " iterations!";
      ++nErrors;
    } // if
    
    if (iWire) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "wire iterator thinks it's still at C=" << iWire->Cryostat
        << " T=" << iWire->TPC << " P=" << iWire->Plane << " W=" << iWire->Wire
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all wires with the iterators (via iterator box)
    unsigned int nLoopedWires = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nWires << " wires";
    for (geo::WireID const& wID: geom->IterateWires()) {
      LOG_TRACE("GeometryIteratorsDump") << wID;
      if (nLoopedWires >= nWires) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedWires
          << " wires, iterator has not reached the end ("
          << *(geom->end_wire()) << ") but it's still at " << wID;
        ++nErrors;
        break;
      }
      ++nLoopedWires;
    }
    if (nLoopedWires < nWires) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedWires
        << " wires, while we expected " << nWires << " iterations!";
      ++nErrors;
    } // if
    
    return nErrors;
  } // GeometryIteratorLoopTestAlg::Run()
  
  //----------------------------------------------------------------------------

} // namespace geo
