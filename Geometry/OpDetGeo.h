////////////////////////////////////////////////////////////////////////
/// \file  OpDetGeo.h
/// \brief Encapsulate the geometry of an optical detector
///
/// \author  bjpjones@mit.gov
////////////////////////////////////////////////////////////////////////

#ifndef GEO_OPDETGEO_H
#define GEO_OPDETGEO_H
#include <vector>
class TGeoNode;
class TGeoHMatrix;
class TGeoMatrix;

namespace geo {

  class OpDetGeo {
  public:
    OpDetGeo(std::vector<const TGeoNode*>& path, 
	    int depth);
    ~OpDetGeo();

    void   GetCenter(double* xyz, double localz=0.0) const;
    double RMax() const;
    double HalfL() const;
    double RMin() const;
    double ThetaZ(bool degrees = false) const;  ///< returns angle of detector
                                                ///< with respect to z axis 
                                                ///< in the Y-Z plane, in 
                                                ///< radians by default
    double CosThetaFromNormal(double * xyz) const;
    double DistanceToPoint(double * xyz) const;


    void LocalToWorld(const double* local, double* world)     const;
    void LocalToWorldVect(const double* local, double* world) const;
    void WorldToLocal(const double* local, double* world)     const;
    void WorldToLocalVect(const double* local, double* world) const;

    const TGeoNode*     Node() const { return fOpDetNode; }

  private:
    const TGeoNode* fOpDetNode;  ///< Pointer to theopdet node
    TGeoHMatrix*    fGeoMatrix; ///< Transformation matrix to world frame
  };
}


#endif
