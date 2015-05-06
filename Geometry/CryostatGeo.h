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
  /// Geometry information for a single cryostat
  class CryostatGeo {
  public:
    
    /// Construct a representation of a single cryostat of the detector
    CryostatGeo(std::vector<const TGeoNode*>& path, int depth);
    
    /// Destructor: destructs
    ~CryostatGeo();

    
    /// @{
    /// @name Cryostat geometry information
    
    /// Half width of the cryostat
    double            HalfWidth()                               const;
    /// Half height of the cryostat
    double            HalfHeight()                              const;
    /// Length of the cryostat
    double            Length()                                  const;
    /// Mass of the cryostat
    double            Mass()                                    const { return fVolume->Weight(); }
    /// Pointer to ROOT's volume descriptor
    const TGeoVolume* Volume()                                  const { return fVolume;           }
    
    /// @}
    
    
    /// @{
    /// @name TPC access
    
    /// Number of TPCs in this cryostat
    unsigned int      NTPC()                                    const { return fTPCs.size();      }

    /// Return the itpc'th TPC in the cryostat.
    const TPCGeo&     TPC(unsigned int itpc)                    const;

    /**
     * @brief Returns the index of the TPC at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @param wiggle a small factor (like 1+epsilon) to avoid rounding errors
     * @return the TPC index, or UINT_MAX if no TPC is there
     */
    unsigned int FindTPCAtPosition(double const worldLoc[3],
                                   double const wiggle) const;
    
    /// Return the TPCGeo object containing the world position worldLoc
    /// @todo What if there is none?
    const TPCGeo&     PositionToTPC(double const  worldLoc[3],
				    unsigned int &tpc,
				    double const &wiggle)       const;
    /// @}
    
    
    /// @{
    /// @name Optical detector access
    
    /// Number of optical detectors in this TPC
    unsigned int      NOpDet()                                  const { return fOpDets.size();    }
    
    /// Return the iopdet'th optical detector in the cryostat
    const OpDetGeo&   OpDet(unsigned int iopdet)                const;

    /// Find the nearest opdet to point in this cryostat
    unsigned int GetClosestOpDet(double * xyz)                  const;
    
    /// Get name of opdet geometry element
    std::string  OpDetGeoName()                                 const { return fOpDetGeoName; }

    /// @}

    /// @{
    /// @name Coordinate transformation
    
    //@{
    /// Transform point from local plane frame to world frame
    void LocalToWorld(const double* tpc, double* world)         const;
    const TVector3 LocalToWorld( const TVector3& local )        const;
    //@}
    
    /// Transform direction vector from local to world
    void LocalToWorldVect(const double* tpc, double* world)     const;
    
    //@{
    /// Transform point from world frame to local tpc frame
    void WorldToLocal(const double* world, double* tpc)         const;
    const TVector3 WorldToLocal( const TVector3& world )        const;
    //@}
    
    // Transform direction vector from world to local
    void WorldToLocalVect(const double* world, double* tpc)     const;
    
    ///@}
    
    /// Method to sort TPCGeo objects
    void              SortSubVolumes(geo::GeoObjectSorter const& sorter);

    
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
