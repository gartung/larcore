////////////////////////////////////////////////////////////////////////
/// \file  CryostatGeo.h
/// \brief Encapsulate the construction of a single cyostat
///
/// \version $Id: CryostatGeo.h,v 1.7 2009/12/01 21:07:51 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_CRYOSTATGEO_H
#define GEO_CRYOSTATGEO_H
#include <vector>
#include <algorithm>

#include "TVector3.h"
#include "TGeoVolume.h"

#include "Geometry/GeoObjectSorter.h"

class TGeoNode;
class TGeoHMatrix;

namespace geo {
  class WireGeo;
  class PlaneGeo;
  class TPCGeo;
  class OpDetGeo;

  //......................................................................
  /// Geometry information for a single tpc
  class CryostatGeo {
  public:
    // Construct a representation of a single cryostat of the detector
    CryostatGeo(std::vector<const TGeoNode*>& path, int depth);
    ~CryostatGeo();

    // Method to sort TPCGeo objects
    void              SortSubVolumes(geo::GeoObjectSorter const& sorter);

    // Number of tpcs in this cryostat
    unsigned int      NTPC()                                    const { return fTPCs.size();      }

    // Number of optical detectors in this tpc
    unsigned int      NOpDet()                                  const { return fOpDets.size();    }

    // Return the itpc'th tpc in the cryostat.
    const TPCGeo&     TPC(unsigned int itpc)                    const;

    // Return the iopdet'th tpc in the cryostat.
    const OpDetGeo&   OpDet(unsigned int iopdet)                const;

    double            HalfWidth()                               const; // half width of the cryostat
    double            HalfHeight()                              const; // half height of the cryostat
    double            Length()                                  const; // length of the cryostat
    double            Mass()                                    const { return fVolume->Weight(); }
    const TGeoVolume* Volume()                                  const { return fVolume;           }

    const TPCGeo&     PositionToTPC(double const  worldLoc[3],
				    unsigned int &tpc,
				    double const &wiggle)       const; // return the TPCGeo object 
                                                                       // containing the
                                                                       // world position worldLoc

    // Transform point from local plane frame to world frame
    void LocalToWorld(const double* tpc, double* world)         const;

    // Transform direction vector from local to world
    void LocalToWorldVect(const double* tpc, double* world)     const;

    // Again, with TVectors
    const TVector3 LocalToWorld( const TVector3& local )        const;

    // Transform point from world frame to local tpc frame
    void WorldToLocal(const double* world, double* tpc)         const;

    // Transform direction vector from world to local
    void WorldToLocalVect(const double* world, double* tpc)     const;

    // Again, with TVectors
    const TVector3 WorldToLocal( const TVector3& world )        const;

    // Get name of opdet geometry element
    std::string  OpDetGeoName()                                 const { return fOpDetGeoName; }

    // Find the nearest opdet to point in this cryostat
    unsigned int GetClosestOpDet(double * xyz)                  const;

  private:

    void FindTPC(std::vector<const TGeoNode*>& path,
		 unsigned int depth);
    void MakeTPC(std::vector<const TGeoNode*>& path,
		 int depth);
    
    void FindOpDet(std::vector<const TGeoNode*>& path,
		   unsigned int depth);
    void MakeOpDet(std::vector<const TGeoNode*>& path,
		   int depth);

  private:

    TGeoHMatrix*           fGeoMatrix;      ///< TPC to world transform
    std::vector<TPCGeo*>   fTPCs;           ///< List of tpcs in this cryostat
    std::vector<OpDetGeo*> fOpDets;         ///< List of opdets in this cryostat
    TGeoVolume*            fVolume;         ///< Total volume of cryostat, called volCryostat in GDML file
    std::string            fOpDetGeoName;   ///< Name of opdet geometry elements in gdml

  };
}

#endif
////////////////////////////////////////////////////////////////////////
