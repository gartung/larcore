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
#include "Geometry/GeoObjectSorter.h"

namespace geo {
  class PlaneGeo;

  //......................................................................
  /// Geometry information for a single tpc
  class TPCGeo {
  public:
    // Construct a representation of a single plane of the detector
    TPCGeo(std::vector<const TGeoNode*>& path, int depth);
    ~TPCGeo();


    // Method to sort PlaneGeo objects
    void              SortSubVolumes(geo::GeoObjectSorter const& sorter);

    // Method to set the drift direction of a TPCGeo from CryostatGeo
    //  (must be done before TPCGeo level since this information is
    //  needed at the beginning of the plane sorting in TPCGeo)
    void              SetDriftDirection(geo::DriftDirection_t drift)  { fDriftDirection = drift; } 

    // Number of planes in this tpc
    unsigned int      Nplanes()                                 const { return fPlanes.size();   }       
								                                         
    // Return the iplane'th plane in the tpc. 			                                         
    const PlaneGeo& Plane(unsigned int iplane)                	const;				       

    // Return the plane in the tpc with View_t view. 			                                         
    const PlaneGeo& Plane(geo::View_t  view)                	const;				       
    								                                         
    double            ActiveHalfWidth()                         const { return fActiveHalfWidth;        }       
    double            ActiveHalfHeight()          		const { return fActiveHalfHeight;       }
    double            ActiveLength()              		const { return fActiveLength;           }
    double            HalfWidth()                             	const { return fHalfWidth;              } 
    double            HalfHeight()          		      	const { return fHalfHeight;            	} 
    double            Length()              		      	const { return fLength;           	} 
    double            ActiveMass()                            	const { return fActiveVolume->Weight(); }
    const TGeoVolume* ActiveVolume()                          	const { return fActiveVolume;           } 
    const TGeoVolume* TotalVolume()                          	const { return fTotalVolume;            } 
    DriftDirection_t  DriftDirection()                          const { return fDriftDirection;         }
    double            DriftDistance()                           const { return 2.*ActiveHalfWidth();  	}

    const double*     PlaneLocation(unsigned int p)             const; 
    double            Plane0Pitch(unsigned int p)               const;
    double            PlanePitch(unsigned int p1=0,
				 unsigned int p2=1)             const;
    double            WirePitch(unsigned int w1=0,
				unsigned int w2=1,
				unsigned int p=0)               const;

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
  
    /// TPC boundaries in world coordinates (-x, +x, -y, +y, -z, +z)
    std::array<double, 6>              tpcBoundaries;
    
    /// Recomputes the TPC boundary
    void BuildTPCBoundaries();
  
  };
}

#endif
////////////////////////////////////////////////////////////////////////
