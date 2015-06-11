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
    LOG_VERBATIM("GeometryIteratorLoopTest")
      << "We have " << nCryo << " cryostats";
    
    unsigned int nErrors = 0;
    unsigned int nCryostats = 0, nTPCs = 0, nPlanes = 0, nWires = 0;
    geo::GeometryCore::cryostat_id_iterator iCryostatID
      = geom->begin_cryostat_id();
    geo::GeometryCore::TPC_id_iterator iTPCID = geom->begin_TPC_id();
    geo::GeometryCore::plane_id_iterator iPlaneID = geom->begin_plane_id();
    geo::GeometryCore::wire_id_iterator iWireID = geom->begin_wire_id();
    
    for(unsigned int c = 0; c < nCryo; ++c) {
      const CryostatGeo& cryo(geom->Cryostat(c));
      const unsigned int nTPC = cryo.NTPC();
    
      LOG_TRACE("GeometryIteratorLoopTest") << "  C=" << c
        << " (" << nTPC << " TPCs)";
      
      if (!iCryostatID) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator thinks it's all over at C=" << c;
        ++nErrors;
      }
      else if (iCryostatID->Cryostat != c) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator thinks it's at C=" << (*iCryostatID)
          << " instead of " << c;
        ++nErrors;
      }
      else if (iCryostatID.get() != &cryo) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator retrieves CryostatGeo["
          << ((void*) iCryostatID.get())
          << "] instead of [" << ((void*) &cryo) << "]";
        ++nErrors;
      }
      
      
      for(unsigned int t = 0; t < nTPC; ++t){
        const TPCGeo& TPC(cryo.TPC(t));
        const unsigned int NPlanes = TPC.Nplanes();
        
        LOG_TRACE("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
          << " (" << NPlanes << " planes)";
        if (!iTPCID) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator thinks it's all over at C=" << c << " T=" << t;
          ++nErrors;
        }
        else if (iTPCID->Cryostat != c) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator thinks it's at C=" << iTPCID->Cryostat
            << " instead of " << c;
          ++nErrors;
        }
        else if (iTPCID->TPC != t) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator thinks it's at T=" << iTPCID->TPC << " instead of "
            << t;
          ++nErrors;
        }
        else if (iTPCID.get() != &TPC) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator retrieves TPCGeo[" << ((void*) iTPCID.get())
            << "] instead of [" << ((void*) &TPC) << "]";
          ++nErrors;
        }
        
        for(unsigned int p = 0; p < NPlanes; ++p) {
          const PlaneGeo& Plane(TPC.Plane(p));
          const unsigned int NWires = Plane.Nwires();
          
          LOG_TRACE("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
            << " P=" << p << " (" << NWires << " wires)";
          if (!iPlaneID) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's all over at C=" << c << " T=" << t
              << " P=" << p;
            ++nErrors;
          }
          else if (iPlaneID->Cryostat != c) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's at C=" << iPlaneID->Cryostat
              << " instead of " << c;
            ++nErrors;
          }
          else if (iPlaneID->TPC != t) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's at T=" << iPlaneID->TPC
              << " instead of " << t;
            ++nErrors;
          }
          else if (iPlaneID->Plane != p) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator thinks it's at P=" << iPlaneID->Plane
              << " instead of " << p;
            ++nErrors;
          }
          else if (iPlaneID.get() != &Plane) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator retrieves TPCGeo[" << ((void*) iPlaneID.get())
              << "] instead of [" << ((void*) &Plane) << "]";
            ++nErrors;
          }
          
          
          for(unsigned int w = 0; w < NWires; ++w) {
            const WireGeo& Wire(Plane.Wire(w));
            
            LOG_TRACE("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
              << " P=" << p << " W=" << w;
            if (!iWireID) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's all over at C=" << c
                << " T=" << t << " P=" << p << " W=" << w;
              ++nErrors;
            }
            else if (iWireID->Cryostat != c) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at C=" << iWireID->Cryostat
                << " instead of " << c;
              ++nErrors;
            }
            else if (iWireID->TPC != t) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at T=" << iWireID->TPC
                << " instead of " << t;
              ++nErrors;
            }
            else if (iWireID->Plane != p) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at P=" << iWireID->Plane
                << " instead of " << p;
              ++nErrors;
            }
            else if (iWireID->Wire != w) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator thinks it's at W=" << iWireID->Wire
                << " instead of " << w;
              ++nErrors;
            }
            else if (iWireID.get() != &Wire) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator retrieves TPCGeo[" << ((void*) iWireID.get())
                << "] instead of [" << ((void*) &Plane) << "]";
              ++nErrors;
            }
          
            ++iWireID;
            ++nWires;
          } // end loop over wires
          ++iPlaneID;
          ++nPlanes;
        } // end loop over planes
        ++iTPCID;
        ++nTPCs;
      } // end loop over tpcs
      ++iCryostatID;
      ++nCryostats;
    } // end loop over cryostats
    
    if (iCryostatID) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Cryostat iterator thinks it's still at " << *iCryostatID
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all cryostats with the iterators (via iterator box)
    unsigned int nLoopedCryostats = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nCryostats << " cryostats";
    for (geo::CryostatID const& cID: geom->IterateCryostatIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << cID;
      if (nLoopedCryostats >= nCryostats) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedCryostats
          << " cryostats, iterator has not reached the end ("
          << *(geom->end_cryostat_id()) << ") but it's still at " << cID;
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
    
    if (iTPCID) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "TPC iterator thinks it's still at " << *iTPCID
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all TPCs with the iterators (via iterator box)
    unsigned int nLoopedTPCs = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nTPCs << " TPCs";
    for (geo::TPCID const& tID: geom->IterateTPCIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << tID;
      if (nLoopedTPCs >= nTPCs) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedTPCs
          << " TPCs, iterator has not reached the end ("
          << *(geom->end_TPC_id()) << ") but it's still at " << tID;
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
    
    if (iPlaneID) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "plane iterator thinks it's still at " << *iPlaneID
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all planes with the iterators (via iterator box)
    unsigned int nLoopedPlanes = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nPlanes << " planes";
    for (geo::PlaneID const& pID: geom->IteratePlaneIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << pID;
      if (nLoopedPlanes >= nPlanes) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedPlanes
          << " planes, iterator has not reached the end ("
          << *(geom->end_plane_id()) << ") but it's still at " << pID;
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
    
    if (iWireID) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "wire iterator thinks it's still at " << *iWireID
        << ", but we are already over";
      ++nErrors;
    }
    
    // test if we can loop all wires with the iterators (via iterator box)
    unsigned int nLoopedWires = 0;
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nWires << " wires";
    for (geo::WireID const& wID: geom->IterateWireIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << wID;
      if (nLoopedWires >= nWires) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedWires
          << " wires, iterator has not reached the end ("
          << *(geom->end_wire_id()) << ") but it's still at " << wID;
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
