////////////////////////////////////////////////////////////////////////
/// \file  WireGeo.h
/// \brief Encapsulate the geometry of a wire
///
/// \version $Id: WireGeo.h,v 1.7 2009/11/04 21:31:41 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#ifndef GEO_WIREGEO_H
#define GEO_WIREGEO_H
#include <vector>

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
    WireGeo(std::vector<const TGeoNode*>& path, 
	    int depth);
    ~WireGeo();

    void   GetCenter(double* xyz, double localz=0.0) const;
    double RMax() const;
    double HalfL() const;
    double RMin() const;
    double ThetaZ(bool degrees = false) const;  ///< returns angle of wire
                                                ///< with respect to z axis 
                                                ///< in the Y-Z plane, in 
                                                ///< radians by default
    TVector3 const& Direction() const;

    void LocalToWorld(const double* local, double* world)     const;
    void LocalToWorldVect(const double* local, double* world) const;
    void WorldToLocal(const double* local, double* world)     const;
    void WorldToLocalVect(const double* local, double* world) const;

    const TGeoNode*     Node() const { return fWireNode; }

  private:
    const TGeoNode*    fWireNode;  ///< Pointer to the wire node
    const TGeoHMatrix* fGeoMatrix; ///< Transformation matrix to world frame
    double             fThetaZ;    ///< angle of the wire with respect to the z direction
    double             fHalfL;     ///< half length of the wire
    double             fRMax;      ///< max radius of the wire
    double             fRMin;      ///< min radius of the wire
    TVector3           fCenter;    ///< center of the wire in world coordinates
    TVector3           fDirection; ///< direction of the wire in world coordinates
  };
}

inline TVector3 const& geo::WireGeo::Direction() const { return fDirection; }

#endif
