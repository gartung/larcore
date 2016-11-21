////////////////////////////////////////////////////////////////////////
/// \file  WireGeo.h
/// \brief Encapsulate the geometry of a wire
///
/// \version $Id: WireGeo.h,v 1.7 2009/11/04 21:31:41 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#ifndef GEO_WIREGEO_H
#define GEO_WIREGEO_H

// C/C++ libraries
#include <vector>
#include <cmath> // std::sin(), ...

// CLHEP
#include "CLHEP/Geometry/Transform3D.h"

// ROOT
#include "TVector3.h"


class TGeoNode;
class TGeoHMatrix;
class TGeoMatrix;


namespace geo {
  
  class WireID; // forward declaration
  
  
  /** **************************************************************************
   * @brief Geometry description of a TPC wire
   * 
   * The wire is a single straight segment on a wire plane.
   * Different wires may be connected to the same readout channel. That is of
   * no relevance for the geometry description.
   * 
   * The wire has a start and an end point. Their definition of them is related
   * to the other wires in the plane and to the TPC itself.
   * 
   * The direction of increasing wire coordinate, defined in the wire plane,
   * is orthogonal to the wire direction and of course points to the direction
   * where the wire number within the plane increases. This direction is
   * indirectly defined when sorting the wires in the plane, which is done by
   * the plane (geo::PlaneGeo). This direction lies by definition on the wire
   * plane. The direction normal to the wire plane is defined by the TPC so that
   * it points inward the TPC rather than outward.
   * Finally, the wire direction is defined so that the triplet of unit vectors
   * direction of the wire @f$ \hat{l} @f$, direction of increasing wire number
   * @f$ \hat{w} @f$, and normal to the plane @f$ \hat{n} @f$ is positively
   * defined (@f$ \hat{l} \times \hat{w} \cdot \hat{n} = +1 @f$).
   * The start @f$ \vec{a}_{w} @f$ and the end of the wire @f$ \vec{b}_{w} @f$
   * are defined so that their difference @f$ \vec{b}_{w} - \vec{a}_{w} @f$
   * points in the same direction as @f$ \hat{l} @f$.
   * 
   */
  class WireGeo {
  public:
    WireGeo(std::vector<const TGeoNode*>& path, int depth);
    
    
    /// @{
    /// @name Size and coordinates
    
    /// Returns the outer half-size of the wire [cm]
    double RMax() const;
    
    /// Returns half the length of the wire [cm]
    double HalfL() const;
    
    /// Returns the inner radius of the wire (usually 0) [cm]
    double RMin() const;
    
    /**
     * @brief Fills the world coordinate of a point on the wire
     * @param xyz _(output)_ the position to be filled, as [ x, y, z ] (in cm)
     * @param localz distance of the requested point from the middle of the wire
     * @see GetCenter(), GetStart(), GetEnd(), GetPositionFromCenter()
     */
    void GetCenter(double* xyz, double localz=0.0) const;
    
    /// Fills the world coordinate of one end of the wire
    void GetStart(double* xyz) const { GetCenter(xyz, -fHalfL); }
    
    /// Fills the world coordinate of one end of the wire
    void GetEnd(double* xyz) const { GetCenter(xyz, +fHalfL); }

    /**
     * @brief Returns the position (world coordinate) of a point on the wire
     * @param localz distance of the requested point from the middle of the wire
     * @return the position of the requested point (in cm)
     * @see GetCenter(), GetStart(), GetEnd()
     */
    TVector3 GetPositionFromCenter(double localz) const
      { double xyz[3]; GetCenter(xyz, localz); return { xyz }; }
    
    /// Returns the world coordinate of the center of the wire [cm]
    TVector3 GetCenter() const { return GetPositionFromCenter(0.0); }
    
    /// Returns the world coordinate of one end of the wire [cm]
    TVector3 GetStart() const { return GetPositionFromCenter(-HalfL()); }
    
    /// Returns the world coordinate of one end of the wire [cm]
    TVector3 GetEnd() const { return GetPositionFromCenter(+HalfL()); }
    
    /// Returns the wire length in centimeters
    double Length() const { return 2. * HalfL(); }
    
    /// @}
    
    /// @{
    /// @name Orientation and angles
    
    /// Returns angle of wire with respect to z axis in the Y-Z plane in radians
    double ThetaZ() const { return fThetaZ; }
    
    /**
     * Returns angle of wire with respect to z axis in the Y-Z plane
     * @param degrees return the angle in degrees rather than radians
     * @return wire angle
     */
    double ThetaZ(bool degrees) const;
    
    //@{
    /// Returns trigonometric operations on ThetaZ()
    double CosThetaZ() const { return std::cos(ThetaZ()); }
    double SinThetaZ() const { return std::sin(ThetaZ()); }
    double TanThetaZ() const { return std::tan(ThetaZ()); }
    //@}
    
    /// Returns if this wire is horizontal (theta_z ~ 0)
    bool isHorizontal() const { return std::abs(SinThetaZ()) < 1e-5; }
    
    /// Returns if this wire is vertical (theta_z ~ pi/2)
    bool isVertical() const { return std::abs(CosThetaZ()) < 1e-5; }
    
    /// Returns if this wire is parallel to another (projected in y/z plane)
    bool isParallelTo(geo::WireGeo const& wire) const
      { return std::abs(wire.ThetaZ() - ThetaZ()) < 1e-5; }
    
    /// Returns the wire direction as a norm-one vector
    TVector3 Direction() const;
    
    /// @}
    
    
    /// @{
    /// @name Coordinate conversion
    
    void LocalToWorld(const double* local, double* world)     const;
    void LocalToWorldVect(const double* local, double* world) const;
    void WorldToLocal(const double* world, double* local)     const;
    void WorldToLocalVect(const double* world, double* local) const;
    
    /// @}

    const TGeoNode*     Node() const { return fWireNode; }
    
    /// Returns the z coordinate, in centimetres, at the point where y = 0.
    /// Assumes the wire orthogonal to x axis and the wire not parallel to z.
    double ComputeZatY0() const
      { return fCenter[2] - fCenter[1] / TanThetaZ(); }
    
    /**
     * @brief Returns distance, projected on y/z plane, from the specified wire
     * @return the signed distance in centimetres (0 if wires are not parallel)
     * 
     * If the specified wire is "ahead" in z respect to this, the distance is
     * returned negative.
     */
    double DistanceFrom(geo::WireGeo const& wire) const
      {
        return isParallelTo(wire)
          ? std::abs(
            + (wire.fCenter[2] - fCenter[2]) * SinThetaZ()
            - (wire.fCenter[1] - fCenter[1]) * CosThetaZ()
            )
          : 0;
      } // DistanceFrom()
    
    
    /// Reset the wire ID (currently no-op since there is no ID to be reset)
    void ResetID(geo::WireID const&) {}
    
    /// Returns the pitch (distance on y/z plane) between two wires, in cm
    static double WirePitch(geo::WireGeo const& w1, geo::WireGeo const& w2)
      { return std::abs(w2.DistanceFrom(w1)); }
    
  private:
    const TGeoNode*    fWireNode;  ///< Pointer to the wire node
    double             fThetaZ;    ///< angle of the wire with respect to the z direction
    double             fHalfL;     ///< half length of the wire
    double             fCenter[3]; ///< center of the wire in world coordinates
    HepGeom::Transform3D
                       fGeoMatrix; ///< Transformation matrix to world frame
  };
}

#endif
