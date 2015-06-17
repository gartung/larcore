////////////////////////////////////////////////////////////////////////
/// \file  PlaneGeo.h
/// \brief Encapsulate the construction of a single detector plane
///
/// \version $Id: PlaneGeo.h,v 1.7 2009/12/01 21:07:51 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_PLANEGEO_H
#define GEO_PLANEGEO_H

// LArSoft libraries
#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/GeoObjectSorter.h"

// C/C++ standard libraries
#include <cmath> // std::atan2()
#include <vector>

class TGeoNode;
class TGeoHMatrix;
class TVector3;

namespace geo {
  class WireGeo;

  //......................................................................
  
  /// Geometry information for a single readout plane
  class PlaneGeo {
  public:
    /// Construct a representation of a single plane of the detector
    PlaneGeo(std::vector<const TGeoNode*>& path, int depth);
    ~PlaneGeo();

    
    /// @{
    /// @name Plane properties
    
    /// Which coordinate does this plane measure
    View_t View()                                             const { return fView;          }
    
    /// What is the orienation of the plane
    Orient_t Orientation()                                    const { return fOrientation;   }

    /// What is the signal type for the plane
    SigType_t SignalType()                                    const { return fSignalType;    }

    /// Angle of the wires from positive z axis; @f$ \theta_{z} \in [ 0, \pi ]@f$.
    double ThetaZ()                                           const;
    
    /// Angle from positive z axis of the wire coordinate axis, in radians
    double PhiZ()                                             const
      { return std::atan2(fSinPhiZ, fCosPhiZ); }
    
    /// Sine of PhiZ()
    double SinPhiZ()                                          const { return fSinPhiZ; }
    
    /// Cosine of PhiZ()
    double CosPhiZ()                                          const { return fCosPhiZ; }
    
    /// @}
    
    
    /// @{
    /// @name Wire access
    
    //@{
    /// Number of wires in this plane
    unsigned int Nwires()                                     const { return fWire.size();   }
    unsigned int NElements()                                  const { return Nwires();       }
    //@}
    
    //@{
    /**
     * @brief Returns whether a wire with index iwire is present in this plane
     * @param iwire index of wire in this plane
     * @return whether the wire with index iwire is present in this plane
     */
    bool HasWire(unsigned int iwire) const { return iwire < Nwires(); }
    bool HasElement(unsigned int iwire) const { return HasWire(iwire); }
    //@}
    
    //@{
    /**
     * @brief Returns whether the wire in wireid is present in this plane
     * @param wireid full wire ID
     * @return whether the wire in wireid is present in this plane
     *
     * The cryostat, TPC and plane numbers in wireid are ignored, as it is
     * ignored whether wireid is invalid.
     */
    bool HasWire(geo::WireID const& wireid) const
      { return HasWire(wireid.Wire); }
    bool HasElement(geo::WireID const& wireid) const
      { return HasWire(wireid); }
    //@}
    
    /// Return the iwire'th wire in the plane.
    /// @throws cet::exception (category "WireOutOfRange") if no such wire
    /// @note In the past, no check was performed.
    WireGeo const& Wire(unsigned int iwire) const;
    
    //@{
    /**
     * @brief Returns the wire in wireid from this plane
     * @param wireid full wire ID
     * @return a constant reference to the wire in wireid
     * @throws cet::exception (category "WireOutOfRange") if no such wire
     *
     * The cryostat, TPC and plane numbers in wireid are ignored, as it is
     * ignored whether wireid is invalid.
     */
    WireGeo const& Wire(WireID const& wireid) const
      { return Wire(wireid.Wire); }
    WireGeo const& GetElement(WireID const& wireid) const
      { return Wire(wireid); }
    //@}
    
    /**
     * @brief Returns the wire number iwire from this plane
     * @param iwire the number of local wire
     * @return a constant pointer to the wire, or nullptr if it does not exist
     */
    WireGeo const* WirePtr(unsigned int iwire) const
      { return HasWire(iwire)? fWire[iwire]: nullptr; }
    
    //@{
    /**
     * @brief Returns the wire in wireid from this plane
     * @param wireid full wire ID
     * @return a constant pointer to the wire, or nullptr if it does not exist
     *
     * The cryostat, TPC and plane numbers in wireid are ignored, as it is
     * ignored whether wireid is invalid.
     */
    WireGeo const* WirePtr(WireID const& wireid) const
      { return WirePtr(wireid.Wire); }
    WireGeo const* GetElementPtr(WireID const& wireid) const
      { return WirePtr(wireid); }
    //@}
    
    
    /// Return the first wire in the plane.
    const WireGeo& FirstWire()                                const { return Wire(0);        }
    
    /// Return the middle wire in the plane.
    const WireGeo& MiddleWire()                               const { return Wire(Nwires()/2); }
    
    /// Return the last wire in the plane.
    const WireGeo& LastWire()                                 const { return Wire(Nwires()-1); }
    
    /// @}
    
    
    /// @{
    /// @name Plane geometry properties
    
    double WirePitch()                                        const { return fWirePitch; }
    
    /**
     * @brief Returns whether the higher z wires have higher wire ID
     * @return whether the higher z wires have higher wire ID
     * @see GetIncreasingWireDirection()
     * 
     * This method is related to GetIncreasingWireDirection()
     * (it might be expressed as "GetIncreasingWireDirection()[2] > 0"),
     * but it is implemented in a faster and independent way.
     */
    bool WireIDincreasesWithZ() const;
    
    /**
     * @brief Returns the direction normal to the plane
     * @return a TVector3 versor with a direction normal to the plane
     *
     * The versor is orthogonal to the plane.
     * Its direction is defined so that the cross product of it and the wires
     * direction points to wires of larger ID.
     * The wire direction is as returned by geo::WireGeo::Direction(),
     * and defined by the geometry.
     */
    TVector3 GetNormalDirection() const;

    /**
     * @brief Returns the direction of increasing wires
     * @return a TVector3 versor with the direction of increasing wires
     *
     * The versor is orthogonal to the wires (assumed parallel),
     * lies on the plane and its direction goes toward increasing wire IDs.
     */
    TVector3 GetIncreasingWireDirection() const;
    
    /// @}
    
    
    /// @{
    /// @name Coordinate transformation
    
    /// Transform point from local plane frame to world frame
    void LocalToWorld(const double* plane, double* world)     const;
    
    /// Transform direction vector from local to world
    void LocalToWorldVect(const double* plane, double* world) const;

    // Again, with TVectors
    const TVector3 LocalToWorld( const TVector3& local )      const;

    /// Transform point from world frame to local plane frame
    void WorldToLocal(const double* world, double* plane)     const;

    /// Transform direction vector from world to local
    void WorldToLocalVect(const double* world, double* plane) const;
    
    // Again, with TVectors
    const TVector3 WorldToLocal( const TVector3& world )      const;
    
    /// @}
    
    
    /// @{
    /// @name Setters
    
    /// Set the signal type and view from TPCGeo
    void SetSignalType(geo::SigType_t sigtype)                      { fSignalType = sigtype; }
    void SetView(geo::View_t view)                                  { fView = view; }
    
    /// @}
    
    /// Apply sorting to WireGeo objects
    void SortWires(geo::GeoObjectSorter const& sorter);
    
  private:
    
    void FindWire(std::vector<const TGeoNode*>& path,
		  unsigned int depth);
    void MakeWire(std::vector<const TGeoNode*>& path, 
		  int depth);
    
    /// Updates the stored wire pitch
    void UpdateWirePitch();
    
    /// Updates the stored phi_z
    void UpdatePhiZ();
    
    /// Performs all the updates needed after mapping changes
    void UpdateFromMapping();
    
  private:
    TGeoHMatrix*          fGeoMatrix;   ///< Plane to world transform
    View_t                fView;        ///< Does this plane measure U,V, or W?
    Orient_t              fOrientation; ///< Is the plane vertical or horizontal?
    SigType_t             fSignalType;  ///< Is the plane induction or collection?
    std::vector<WireGeo*> fWire;        ///< List of wires in this plane
    double                fWirePitch;   ///< pitch of wires in this plane
    double                fSinPhiZ;     ///< sine of phiZ
    double                fCosPhiZ;     ///< cosine of phiZ
  };
}

#endif
////////////////////////////////////////////////////////////////////////
