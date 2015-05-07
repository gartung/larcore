////////////////////////////////////////////////////////////////////////
/// \file  TPCGeo.h
/// \brief Encapsulate the construction of a single detector plane
///
/// \version $Id: TPCGeo.h,v 1.7 2009/12/01 21:07:51 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_TPCGEO_H
#define GEO_TPCGEO_H
#include <vector>
#include <array>
#include <algorithm>

#include "TGeoVolume.h"

#include "WireGeo.h"

class TGeoNode;
class TGeoHMatrix;

#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/BoxBoundedGeo.h"
#include "Geometry/GeoObjectSorter.h"

namespace geo {
  class PlaneGeo;

  //......................................................................
  /// Geometry information for a single tpc
  class TPCGeo: public BoxBoundedGeo {
  public:
    // Construct a representation of a single plane of the detector
    TPCGeo(std::vector<const TGeoNode*>& path, int depth);
    ~TPCGeo();
    
    
    /// @{
    /// @name TPC properties
    
    double            ActiveHalfWidth()                         const { return fActiveHalfWidth;        }
    double            ActiveHalfHeight()                        const { return fActiveHalfHeight;       }
    double            ActiveLength()                            const { return fActiveLength;           }
    double            HalfWidth()                               const { return fHalfWidth;              }
    double            HalfHeight()                              const { return fHalfHeight;             }
    double            Length()                                  const { return fLength;                 }
    double            ActiveMass()                              const { return fActiveVolume->Weight(); }
    const TGeoVolume* ActiveVolume()                            const { return fActiveVolume;           }
    const TGeoVolume* TotalVolume()                             const { return fTotalVolume;            }
    DriftDirection_t  DriftDirection()                          const { return fDriftDirection;         }
    double            DriftDistance()                           const { return 2.*ActiveHalfWidth();    }
    
    /// @}
    
    
    /// @{
    /// @name Plane access
    
    //@{
    /// Number of planes in this tpc
    unsigned int    Nplanes()                                   const { return fPlanes.size();   }
    unsigned int    NElements()                                 const { return fPlanes.size();   }
    //@}
    
    //@{
    /**
     * @brief Returns whether a plane with index iplane is present in this TPC
     * @param iplane index of plane in this TPC
     * @return whether the plane with index iplane is present in this TPC
     */
    bool HasPlane(unsigned int iplane) const { return iplane < Nplanes(); }
    bool HasElement(unsigned int iplane) const { return HasPlane(iplane); }
    //@}
    
    //@{
    /**
     * @brief Returns whether the plane in planeid is present in this TPC
     * @param tpcid full TPC ID
     * @return whether the plane in planeid is present in this TPC
     *
     * The cryostat and TPC numbers in planeid are ignored, as it is ignored
     * whether planeid is invalid.
     */
    bool HasPlane(geo::PlaneID const& planeid) const
      { return HasPlane(planeid.Plane); }
    bool HasElement(geo::PlaneID const& planeid) const
      { return HasPlane(planeid); }
    //@}
    
    /// Return the plane in the tpc with View_t view.
    PlaneGeo const&     Plane(geo::View_t  view)                const;
    
    
    /// Return the iplane'th plane in the TPC.
    /// @throws cet::exception (category "PlaneOutOfRange")  if no such plane
    PlaneGeo const&     Plane(unsigned int iplane)              const;
    
    //@{
    /**
     * @brief Returns the plane in planeid from this TPC
     * @param planeid full plane ID
     * @return a constant reference to the plane in planeid
     * @throws cet::exception (category "PlaneOutOfRange") if no such plane
     *
     * The cryostat and TPC numbers in planeid are ignored, as it is ignored
     * whether planeid is invalid.
     */
    const PlaneGeo&     Plane(PlaneID const& planeid)           const
      { return Plane(planeid.Plane); }
    const PlaneGeo&     GetElement(PlaneID const& planeid)            const
      { return Plane(planeid); }
    //@}
    
    /**
     * @brief Returns the plane number iplane from this TPC
     * @param iplane the number of local plane
     * @return a constant pointer to the plane, or nullptr if it does not exist
     */
    PlaneGeo const*     PlanePtr(unsigned int iplane)                const
      { return HasPlane(iplane)? fPlanes[iplane]: nullptr; }
    
    //@{
    /**
     * @brief Returns the plane in planeid from this TPC
     * @param planeid full plane ID
     * @return a constant pointer to the plane, or nullptr if it does not exist
     *
     * The cryostat and TPC numbers in planeid are ignored, as it is ignored
     * whether planeid is invalid.
     */
    PlaneGeo const*     PlanePtr(PlaneID const& planeid)                   const
      { return PlanePtr(planeid.Plane); }
    PlaneGeo const*     GetElementPtr(PlaneID const& planeid)            const
      { return PlanePtr(planeid); }
    //@}
    
    
    /// @}
    
    
    /// @{
    /// @name TPC geometry properties
    
    const double*     PlaneLocation(unsigned int p)             const; 
    double            Plane0Pitch(unsigned int p)               const;
    double            PlanePitch(unsigned int p1=0,
                                 unsigned int p2=1)             const;
    double            WirePitch(unsigned int w1=0,
                                unsigned int w2=1,
                                unsigned int p=0)               const;
    
    /**
     * @brief Returns whether this TPC contains the specified world coordinate
     * @param worldLoc the absolute ("world") coordinate (x, y, z)
     * @param wiggle expansion factor for the range
     * @return whether the specified coordinate is in this TPC
     *
     * If the wiggle is larger than 1, each size of the TPC is expanded by the
     * wiggle factor.
     * If the wiggle is less than 1, each size is shrinked.
     */
    bool ContainsPosition(double const worldLoc[3], double const wiggle) const;
    
    /// @}
    
    
    /// @{
    /// @name Setters
    
    /**
     * @brief Method to set the drift direction of a TPCGeo from CryostatGeo
     *
     * Must be done before TPCGeo level since this information is
     * needed at the beginning of the plane sorting in TPCGeo.
     */
    void              SetDriftDirection(geo::DriftDirection_t drift)  { fDriftDirection = drift; }
    
    
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
    
    /// Transform direction vector from world to local
    void WorldToLocalVect(const double* world, double* tpc)     const;
    
    /// @}
    
    
    /// Apply sorting to the PlaneGeo objects
    void              SortSubVolumes(geo::GeoObjectSorter const& sorter);
    
    
    
    /**
     * @brief Returns whether the specified coordinate is in a range
     * @param c the coordinate
     * @param min lower boundary of the range
     * @param max upper boundary of the range
     * @param wiggle expansion factor for the range
     * @return whether the specified coordinate is in a range
     *
     * If the wiggle is larger than 1, the range is expanded by the wiggle factor.
     * If the wiggle is less than 1, the range is shrinked.
     */
    static bool CoordinateContained
      (double c, double min, double max, double wiggle = 1.)
      {
        return (c >= (min > 0? min / wiggle: min * wiggle))
          && (c <= (max < 0? max / wiggle: max * wiggle));
      } // CoordinateContained()
    
    static bool CoordinateContained
      (double c, double const* range, double wiggle = 1.)
      { return CoordinateContained(c, range[0], range[1], wiggle); }
    
    
  private:
    
    void FindPlane(std::vector<const TGeoNode*>& path,
                   unsigned int depth);
    void MakePlane(std::vector<const TGeoNode*>& path, 
                   int depth);
    
  private:

    TGeoHMatrix*                       fGeoMatrix;      ///< TPC to world transform
    std::vector<PlaneGeo*>             fPlanes;         ///< List of planes in this plane
    TGeoVolume*                        fActiveVolume;   ///< Active volume of LAr, called volTPCActive in GDML file 
    TGeoVolume*                        fTotalVolume;    ///< Total volume of TPC, called volTPC in GDML file
    DriftDirection_t                   fDriftDirection; ///< Direction of the electron drift in the TPC
    std::vector<double>                fPlane0Pitch;    ///< Pitch between planes
    std::vector< std::vector<double> > fPlaneLocation;  ///< xyz locations of planes in the TPC

    double                             fActiveHalfWidth;  ///< half width of active volume
    double                             fActiveHalfHeight; ///< half height of active volume
    double                             fActiveLength;     ///< length of active volume
    double                             fHalfWidth;        ///< half width of total volume
    double                             fHalfHeight;       ///< half height of total volume
    double                             fLength;           ///< length of total volume
  
    /// Recomputes the TPC boundary
    void InitTPCBoundaries();
  
  };
}

#endif
////////////////////////////////////////////////////////////////////////
