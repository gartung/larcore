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
  
  
  /// \brief Encapsulate the cell geometry
  //
  /// A note on the cell geometry: Wires are constructed such that, in
  /// their local frame, their profile occupies the x-y plane with
  /// their long dimension running along the z-axis. 

  class WireGeo {
  public:
    WireGeo(std::vector<const TGeoNode*>& path, int depth);
    WireGeo() = delete;   // disallow default constructor

    void   GetCenter(double* xyz, double localz=0.0) const;
    void   GetStart(double* xyz) const { GetCenter(xyz, -fHalfL); }
    void   GetEnd(double* xyz) const { GetCenter(xyz, +fHalfL); }
    double RMax() const;
    double HalfL() const;
    double RMin() const;

    /// Returns the wire length in centimeters
    double Length() const { return 2. * HalfL(); }
    
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

    void LocalToWorld(const double* local, double* world)     const;
    void LocalToWorldVect(const double* local, double* world) const;
    void WorldToLocal(const double* world, double* local)     const;
    void WorldToLocalVect(const double* world, double* local) const;

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
