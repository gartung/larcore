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

#include "larcore/Geometry/BoxBoundedGeo.h"
#include "larcore/Geometry/GeoObjectSorter.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"

class TGeoNode;
class TGeoHMatrix;

namespace geo {
  class WireGeo;
  class PlaneGeo;
  class TPCGeo;
  class OpDetGeo;

  //......................................................................
  /// Geometry information for a single cryostat
  class CryostatGeo: public geo::BoxBoundedGeo {
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
    /// Full width of the cryostat
    double            Width()                                   const { return 2. * HalfWidth(); }
    /// Full height of the cryostat
    double            Height()                                  const { return 2. * HalfHeight(); }
    /// Length of the cryostat
    double            Length()                                  const;
    /// Mass of the cryostat
    double            Mass()                                    const { return fVolume->Weight(); }
    /// Pointer to ROOT's volume descriptor
    const TGeoVolume* Volume()                                  const { return fVolume;           }
    
    /// @brief Returns boundaries of the cryostat (in centimetres)
    /// @return boundaries in a geo::BoxBoundedGeo
    geo::BoxBoundedGeo const& Boundaries() const { return *this; }
    
    /// @brief Fills boundaries of the cryostat (in centimetres)
    /// @param boundaries filled as: [0] -x [1] +x [2] -y [3] +y [4] -z [5] +z
    void Boundaries(double* boundaries) const;
    
    
    /// Returns the identifier of this cryostat
    geo::CryostatID const& ID() const { return fID; }
    
    /// @}
    
    
    /// @{
    /// @name TPC access
    
    //@{
    /// Number of TPCs in this cryostat
    unsigned int      NTPC()                                    const { return fTPCs.size();      }
    unsigned int      NElements()                               const { return fTPCs.size();      }
    //@}
    
    //@{
    /**
     * @brief Returns whether a TPC with index itpc is present in this cryostat
     * @param itpc index of TPC in this cryostat
     * @return whether the TPC with index itpc is present in this cryostat
     */
    bool HasTPC(unsigned int itpc) const { return itpc < NTPC(); }
    bool HasElement(unsigned int itpc) const { return HasTPC(itpc); }
    //@}
    
    //@{
    /**
     * @brief Returns whether the TPC in tpcid is present in this cryostat
     * @param tpcid full TPC ID
     * @return whether the TPC in tpcid is present in this cryostat
     *
     * The cryostat number in tpcid is ignored, as it is ignored whether tpcid
     * is invalid.
     */
    bool HasTPC(geo::TPCID const& tpcid) const { return HasTPC(tpcid.TPC); }
    bool HasElement(geo::TPCID const& tpcid) const { return HasTPC(tpcid); }
    //@}
    
    /// Return the itpc'th TPC in the cryostat.
    /// @throws cet::exception (category "TPCOutOfRange") if no such TPC
    const TPCGeo&     TPC(unsigned int itpc)                    const;
    
    //@{
    /**
     * @brief Returns the TPC in tpcid from this cryostat
     * @param tpcid full TPC ID
     * @return a constant reference to the TPC in tpcid
     * @throws cet::exception (category "TPCOutOfRange") if no such TPC
     *
     * The cryostat number in tpcid is ignored, as it is ignored whether tpcid
     * is invalid.
     */
    const TPCGeo&     TPC(TPCID const& tpcid)                   const
      { return TPC(tpcid.TPC); }
    const TPCGeo&     GetElement(TPCID const& tpcid)            const
      { return TPC(tpcid); }
    //@}
    
    /**
     * @brief Returns the TPC number itpc from this cryostat
     * @param itpc the number of local TPC
     * @return a constant pointer to the TPC, or nullptr if it does not exist
     */
    TPCGeo const*     TPCPtr(unsigned int itpc)                    const
      { return HasTPC(itpc)? fTPCs[itpc]: nullptr; }
    
    //@{
    /**
     * @brief Returns the TPC in tpcid from this cryostat
     * @param tpcid full TPC ID
     * @return a constant pointer to the TPC, or nullptr if it does not exist
     *
     * The cryostat number in tpcid is ignored, as it is ignored whether tpcid
     * is invalid.
     */
    TPCGeo const*     TPCPtr(TPCID const& tpcid)                   const
      { return TPCPtr(tpcid.TPC); }
    TPCGeo const*     GetElementPtr(TPCID const& tpcid)            const
      { return TPCPtr(tpcid); }
    //@}
    
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
    
    /// Returns the largest number of planes among the TPCs in this cryostat
    unsigned int MaxPlanes() const;
    
    /// Returns the largest number of wires among the TPCs in this cryostat
    unsigned int MaxWires() const;
    
    /// @}
    
    
    /// @{
    /// @name Optical detector access
    
    /// Number of optical detectors in this TPC
    unsigned int      NOpDet()                                  const { return fOpDets.size();    }
    
    /// Return the iopdet'th optical detector in the cryostat
    const OpDetGeo&   OpDet(unsigned int iopdet)                const;

    /// Find the nearest opdet to point in this cryostat
    unsigned int GetClosestOpDet(double const* xyz)             const;
    
    /// Get name of opdet geometry element
    std::string  OpDetGeoName()                                 const { return fOpDetGeoName; }

    /// @}

    /// @{
    /// @name Coordinate transformation
    
    //@{
    /// Transform point from local plane frame to world frame
    void LocalToWorld(const double* tpc, double* world)         const;
    TVector3 LocalToWorld( const TVector3& local )              const;
    //@}
    
    /// Transform direction vector from local to world
    void LocalToWorldVect(const double* tpc, double* world)     const;
    
    //@{
    /// Transform point from world frame to local tpc frame
    void WorldToLocal(const double* world, double* tpc)         const;
    TVector3 WorldToLocal( const TVector3& world )              const;
    //@}
    
    // Transform direction vector from world to local
    void WorldToLocalVect(const double* world, double* tpc)     const;
    
    ///@}
    
    /// Method to sort TPCGeo objects
    void              SortSubVolumes(geo::GeoObjectSorter const& sorter);

    
    /// Performs all needed updates after geometry has sorted the cryostats
    void UpdateAfterSorting(geo::CryostatID cryoid);
    
  private:

    void FindTPC(std::vector<const TGeoNode*>& path,
		 unsigned int depth);
    void MakeTPC(std::vector<const TGeoNode*>& path,
		 int depth);
    
    void FindOpDet(std::vector<const TGeoNode*>& path,
		   unsigned int depth);
    void MakeOpDet(std::vector<const TGeoNode*>& path,
		   int depth);

    /// Fill the boundary information of the cryostat
    void InitCryoBoundaries();

  private:

    TGeoHMatrix*           fGeoMatrix;      ///< TPC to world transform
    std::vector<TPCGeo*>   fTPCs;           ///< List of tpcs in this cryostat
    std::vector<OpDetGeo*> fOpDets;         ///< List of opdets in this cryostat
    TGeoVolume*            fVolume;         ///< Total volume of cryostat, called volCryostat in GDML file
    std::string            fOpDetGeoName;   ///< Name of opdet geometry elements in gdml
    geo::CryostatID        fID;             ///< ID of this cryostat
    
  };
}

#endif
////////////////////////////////////////////////////////////////////////
