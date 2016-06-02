/**
 * @file   GeometryIteratorLoopTestAlg.cxx
 * @brief  Tests the correct iteration of the geo::Geometry iterators
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   August 25, 2014
 */

// our header
#include "GeometryIteratorLoopTestAlg.h"

// LArSoft includes
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcore/Geometry/GeometryCore.h"
#include "larcore/Geometry/CryostatGeo.h"
#include "larcore/Geometry/TPCGeo.h"
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"

// Framework includes
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace geo {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

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
    
    geo::GeometryCore::cryostat_iterator iCryostat = geom->begin_cryostat();
    geo::GeometryCore::TPC_iterator iTPC = geom->begin_TPC();
    geo::GeometryCore::plane_iterator iPlane = geom->begin_plane();
    geo::GeometryCore::wire_iterator iWire = geom->begin_wire();
    
    for(unsigned int c = 0; c < nCryo; ++c) {
      const CryostatGeo& cryo(geom->Cryostat(c));
      const unsigned int nTPC = cryo.NTPC();
    
      LOG_TRACE("GeometryIteratorLoopTest") << "  C=" << c
        << " (" << nTPC << " TPCs)";
      
      if (!iCryostatID) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat ID iterator thinks it's all over at C=" << c;
        ++nErrors;
      }
      else if (iCryostatID->Cryostat != c) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat ID iterator thinks it's at C=" << (*iCryostatID)
          << " instead of " << c;
        ++nErrors;
      }
      else if (iCryostatID.get() != &cryo) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat ID iterator retrieves CryostatGeo["
          << ((void*) iCryostatID.get())
          << "] instead of [" << ((void*) &cryo) << "]";
        ++nErrors;
      }
      
      if (&*iCryostat != &cryo) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "Cryostat iterator retrieves CryostatGeo["
          << ((void*) iCryostat.get())
          << "] (" << iCryostat.ID() << ") instead of ["
          << ((void*) &cryo) << "] (C=" << c << ")";
        ++nErrors;
      }
      
      
      for(unsigned int t = 0; t < nTPC; ++t){
        const TPCGeo& TPC(cryo.TPC(t));
        const unsigned int NPlanes = TPC.Nplanes();
        
        LOG_TRACE("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
          << " (" << NPlanes << " planes)";
        if (!iTPCID) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC ID iterator thinks it's all over at C=" << c << " T=" << t;
          ++nErrors;
        }
        else if (iTPCID->Cryostat != c) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC ID iterator thinks it's at C=" << iTPCID->Cryostat
            << " instead of " << c;
          ++nErrors;
        }
        else if (iTPCID->TPC != t) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC ID iterator thinks it's at T=" << iTPCID->TPC
            << " instead of " << t;
          ++nErrors;
        }
        else if (iTPCID.get() != &TPC) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC ID iterator retrieves TPCGeo[" << ((void*) iTPCID.get())
            << "] instead of [" << ((void*) &TPC) << "]";
          ++nErrors;
        }
        
        if (&*iTPC != &TPC) {
          LOG_ERROR("GeometryIteratorLoopTest")
            << "TPC iterator retrieves TPCGeo[" << ((void*) iTPC.get())
            << "] (" << iTPC.ID() << ") instead of [" << ((void*) &TPC)
            << "] (C=" << c << " T=" << t << ")";
          ++nErrors;
        }
        
        for(unsigned int p = 0; p < NPlanes; ++p) {
          const PlaneGeo& Plane(TPC.Plane(p));
          const unsigned int NWires = Plane.Nwires();
          
          LOG_TRACE("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
            << " P=" << p << " (" << NWires << " wires)";
          if (!iPlaneID) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane ID iterator thinks it's all over at C="
              << c << " T=" << t << " P=" << p;
            ++nErrors;
          }
          else if (iPlaneID->Cryostat != c) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane ID iterator thinks it's at C=" << iPlaneID->Cryostat
              << " instead of " << c;
            ++nErrors;
          }
          else if (iPlaneID->TPC != t) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane ID iterator thinks it's at T=" << iPlaneID->TPC
              << " instead of " << t;
            ++nErrors;
          }
          else if (iPlaneID->Plane != p) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane ID iterator thinks it's at P=" << iPlaneID->Plane
              << " instead of " << p;
            ++nErrors;
          }
          else if (iPlaneID.get() != &Plane) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane ID iterator retrieves PlaneGeo["
              << ((void*) iPlaneID.get()) << "] instead of ["
               << ((void*) &Plane) << "]";
            ++nErrors;
          }
          
          if (&*iPlane != &Plane) {
            LOG_ERROR("GeometryIteratorLoopTest")
              << "plane iterator retrieves PlaneGeo[" << ((void*) iPlane.get())
              << "] instead of [" << ((void*) &Plane) << "] (C="
              << c << " T=" << t << " P=" << p << ")";
            ++nErrors;
          }
          
          
          for(unsigned int w = 0; w < NWires; ++w) {
            const WireGeo& Wire(Plane.Wire(w));
            
            LOG_TRACE("GeometryIteratorLoopTest") << "    C=" << c << " T=" << t
              << " P=" << p << " W=" << w;
            if (!iWireID) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire ID iterator thinks it's all over at C=" << c
                << " T=" << t << " P=" << p << " W=" << w;
              ++nErrors;
            }
            else if (iWireID->Cryostat != c) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire ID iterator thinks it's at C=" << iWireID->Cryostat
                << " instead of " << c;
              ++nErrors;
            }
            else if (iWireID->TPC != t) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire ID iterator thinks it's at T=" << iWireID->TPC
                << " instead of " << t;
              ++nErrors;
            }
            else if (iWireID->Plane != p) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire ID iterator thinks it's at P=" << iWireID->Plane
                << " instead of " << p;
              ++nErrors;
            }
            else if (iWireID->Wire != w) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire ID iterator thinks it's at W=" << iWireID->Wire
                << " instead of " << w;
              ++nErrors;
            }
            else if (iWireID.get() != &Wire) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire ID iterator retrieves WireGeo["
                << ((void*) iWireID.get())
                << "] instead of [" << ((void*) &Wire) << "]";
              ++nErrors;
            }
            
            if (&*iWire != &Wire) {
              LOG_ERROR("GeometryIteratorLoopTest")
                << "wire iterator retrieves WireGeo[" << ((void*) iWire.get())
                << "] instead of [" << ((void*) &Wire) << "] (C="
                << c << " T=" << t << " P=" << p << " W=" << w << ")";
              ++nErrors;
            }
          
            ++iWireID;
            ++iWire;
            ++nWires;
          } // end loop over wires
          ++iPlaneID;
          ++iPlane;
          ++nPlanes;
        } // end loop over planes
        ++iTPCID;
        ++iTPC;
        ++nTPCs;
      } // end loop over tpcs
      ++iCryostatID;
      ++iCryostat;
      ++nCryostats;
    } // end loop over cryostats
    
    if (iCryostatID) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Cryostat ID iterator thinks it's still at " << *iCryostatID
        << ", but we are already finished";
      ++nErrors;
    }
    try {
      geo::CryostatGeo const& Cryo = *iCryostat;
      LOG_ERROR("GeometryIteratorLoopTest")
        << "cryostat iterator thinks it's still at " << iCryostat.ID()
        << ", but we are already finished";
      ++nErrors;
    }
    catch (cet::exception const&) {
      LOG_DEBUG("GeometryIteratorLoopTest") << "exception caught"
        " while dereferencing an iterator to a past-the-end cryostat.\n";
    }
    
    // test if we can loop all cryostats with the iterators (via iterator box)
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nCryostats << " cryostats";
    unsigned int nLoopedCryostatIDs = 0;
    for (geo::CryostatID const& cID: geom->IterateCryostatIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << cID;
      if (nLoopedCryostatIDs >= nCryostats) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedCryostatIDs
          << " cryostat IDs, iterator has not reached the end ("
          << *(geom->end_cryostat_id()) << ") but it's still at " << cID;
        ++nErrors;
        break;
      }
      ++nLoopedCryostatIDs;
    }
    if (nLoopedCryostatIDs < nCryostats) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedCryostatIDs
        << " cryostat IDs, while we expected " << nCryostats << " iterations!";
      ++nErrors;
    } // if
    unsigned int nLoopedCryostats = 0;
    for (geo::CryostatGeo const& Cryo: geom->IterateCryostats()) {
      if (nLoopedCryostats >= nCryostats) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedCryostats
          << " cryostats, iterator has not reached the end";
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
        << ", but we are already finished";
      ++nErrors;
    }
    try {
      geo::TPCGeo const& TPC = *iTPC;
      LOG_ERROR("GeometryIteratorLoopTest")
        << "TPC iterator thinks it's still at " << iTPC.ID()
        << ", but we are already finished";
      ++nErrors;
    }
    catch (cet::exception const&) {
      LOG_DEBUG("GeometryIteratorLoopTest") << "exception caught"
        " while dereferencing an iterator to a past-the-end TPC.\n";
    }
    
    // test if we can loop all TPCs with the iterators (via iterator box)
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nTPCs << " TPCs";
    unsigned int nLoopedTPCIDs = 0;
    for (geo::TPCID const& tID: geom->IterateTPCIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << tID;
      if (nLoopedTPCIDs >= nTPCs) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedTPCIDs
          << " TPC IDs, iterator has not reached the end ("
          << *(geom->end_TPC_id()) << ") but it's still at " << tID;
        ++nErrors;
        break;
      }
      ++nLoopedTPCIDs;
    }
    if (nLoopedTPCIDs < nTPCs) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedTPCIDs
        << " TPC IDs, while we expected " << nTPCs << " iterations!";
      ++nErrors;
    } // if
    unsigned int nLoopedTPCs = 0;
    for (geo::TPCGeo const& TPC: geom->IterateTPCs()) {
      if (nLoopedTPCs >= nTPCs) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedTPCs
          << " TPCs, iterator has not reached the end";
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
        << ", but we are already finished";
      ++nErrors;
    }
    try {
      geo::PlaneGeo const& Plane = *iPlane;
      LOG_ERROR("GeometryIteratorLoopTest")
        << "plane iterator thinks it's still at " << iPlane.ID()
        << ", but we are already finished";
      ++nErrors;
    }
    catch (cet::exception const&) {
      LOG_DEBUG("GeometryIteratorLoopTest") << "exception caught"
        " while dereferencing an iterator to a past-the-end plane.\n";
    }
    
    // test if we can loop all planes with the iterators (via iterator box)
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nPlanes << " planes";
    unsigned int nLoopedPlaneIDs = 0;
    for (geo::PlaneID const& pID: geom->IteratePlaneIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << pID;
      if (nLoopedPlaneIDs >= nPlanes) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedPlaneIDs
          << " planes, ID iterator has not reached the end ("
          << *(geom->end_plane_id()) << ") but it's still at " << pID;
        ++nErrors;
        break;
      }
      ++nLoopedPlaneIDs;
    }
    if (nLoopedPlaneIDs < nPlanes) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedPlaneIDs
        << " plane IDs, while we expected " << nPlanes << " iterations!";
      ++nErrors;
    } // if
    unsigned int nLoopedPlanes = 0;
    for (geo::PlaneGeo const& Plane: geom->IteratePlanes()) {
      if (nLoopedPlanes >= nPlanes) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedPlanes
          << " planes, iterator has not reached the end";
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
        << ", but we are already finished";
      ++nErrors;
    }
    try {
      geo::WireGeo const& Wire = *iWire;
      LOG_ERROR("GeometryIteratorLoopTest")
        << "wire iterator thinks it's still at " << iWire.ID()
        << ", but we are already finished";
      ++nErrors;
    }
    catch (cet::exception const&) {
      LOG_DEBUG("GeometryIteratorLoopTest") << "exception caught"
        " while dereferencing an iterator to a past-the-end wire.\n";
    }
    
    // test if we can loop all wires with the iterators (via iterator box)
    LOG_DEBUG("GeometryIteratorsDump")
      << "Looping though " << nWires << " wires";
    unsigned int nLoopedWireIDs = 0;
    for (geo::WireID const& wID: geom->IterateWireIDs()) {
      LOG_TRACE("GeometryIteratorsDump") << wID;
      if (nLoopedWireIDs >= nWires) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedWireIDs
          << " wire IDs, iterator has not reached the end ("
          << *(geom->end_wire_id()) << ") but it's still at " << wID;
        ++nErrors;
        break;
      }
      ++nLoopedWireIDs;
    }
    if (nLoopedWireIDs < nWires) {
      LOG_ERROR("GeometryIteratorLoopTest")
        << "Looped only " << nLoopedWireIDs
        << " wire IDs, while we expected " << nWires << " iterations!";
      ++nErrors;
    } // if
    unsigned int nLoopedWires = 0;
    for (geo::WireGeo const& Wire: geom->IterateWires()) {
      if (nLoopedWires >= nWires) {
        LOG_ERROR("GeometryIteratorLoopTest")
          << "After all " << nLoopedWires
          << " wires, iterator has not reached the end";
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

#pragma GCC diagnostic pop

} // namespace geo
