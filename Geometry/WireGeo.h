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

// CLHEP
#include "CLHEP/Geometry/Transform3D.h"

// ROOT
#include "TVector3.h"


class TGeoNode;
class TGeoHMatrix;
class TGeoMatrix;


namespace geo {
  /// \brief Encapsulate the cell geometry
  //
  /// A note on the cell geometry: Wires are constructed such that, in
  /// their local frame, their profile occupies the x-y plane with
  /// their long dimension running along the z-axis. 

  class WireGeo {
  public:
    WireGeo(std::vector<const TGeoNode*>& path, int depth);

    void   GetCenter(double* xyz, double localz=0.0) const;
    void   GetStart(double* xyz) const { GetCenter(xyz, -fHalfL); }
    void   GetEnd(double* xyz) const { GetCenter(xyz, +fHalfL); }
    double RMax() const;
    double HalfL() const;
    double RMin() const;
    /**
     * Returns angle of wire with respect to z axis in the Y-Z plane
     * @param degrees return the angle in degrees rather than radians,
     * @return wire angle, in radians by default
     */
    double ThetaZ(bool degrees = false) const;
    
    /// Returns the wire direction as a norm-one vector
    TVector3 Direction() const;

    void LocalToWorld(const double* local, double* world)     const;
    void LocalToWorldVect(const double* local, double* world) const;
    void WorldToLocal(const double* world, double* local)     const;
    void WorldToLocalVect(const double* world, double* local) const;

    const TGeoNode*     Node() const { return fWireNode; }

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
