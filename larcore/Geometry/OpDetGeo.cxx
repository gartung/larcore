////////////////////////////////////////////////////////////////////////
/// \file  OpDetGeo.cxx
/// \brief Encapsulate the geometry of an OpDet
///
/// \author  bjpjones@mit.edu
////////////////////////////////////////////////////////////////////////

#include <cmath>

#include "larcore/Geometry/OpDetGeo.h"

#include "TGeoManager.h"
#include "TGeoTube.h"
#include "TGeoMatrix.h"
#include "TGeoNode.h"
#include "TMath.h"

namespace {
  template <typename T>
  inline T sqr(T v) { return v*v; }
} // local namespace

namespace geo{

  //-----------------------------------------
  OpDetGeo::OpDetGeo(std::vector<const TGeoNode*>& path, int depth) 
  {
    fOpDetNode = path[depth];


    // Build the matrix that takes us to the top world frame
    // build a matrix to take us from the local to the world coordinates
    // in one step
    fGeoMatrix = new TGeoHMatrix(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i){
      fGeoMatrix->Multiply(path[i]->GetMatrix());
    }
  }

  //......................................................................
  OpDetGeo::~OpDetGeo()
  {
    //if(fGeoMatrix) delete fGeoMatrix;
  
    return;
  }  

  //......................................................................

  /// Transform a position from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void OpDetGeo::LocalToWorld(const double* local, double* world) const
  {
    fGeoMatrix->LocalToMaster(local,world);
  }

  //......................................................................    

  /// Transform a 3-vector from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void OpDetGeo::LocalToWorldVect(const double* local, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(local,world);
  }
    
  //......................................................................

  /// Transform a position from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void OpDetGeo::WorldToLocal(const double* world, double* local) const
  {
    fGeoMatrix->MasterToLocal(world,local);
  }

  //......................................................................

  /// Transform a 3-vector from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void OpDetGeo::WorldToLocalVect(const double* world, double* local) const
  {
    fGeoMatrix->MasterToLocalVect(world,local);
  }

  //......................................................................

  /// Return the center position of an opdet
  /// \param xyz : 3-D array. The returned location.
  /// \param localz : Distance along the length of the volume
  /// (cm). Default is center of wire
  void OpDetGeo::GetCenter(double* xyz, double localz) const
  {
    double xyzLocal[3] = {0.,0.,localz};
    this->LocalToWorld(xyzLocal, xyz);
  }

  //......................................................................

  double OpDetGeo::RMax() const 
  {
    return ((TGeoTube*)fOpDetNode->GetVolume()->GetShape())->GetRmax();
  }
    
  //......................................................................

  double OpDetGeo::HalfL() const 
  {
    return ((TGeoTube*)fOpDetNode->GetVolume()->GetShape())->GetDZ();
  }

  //......................................................................

  double OpDetGeo::RMin() const 
  {
    return ((TGeoTube*)fOpDetNode->GetVolume()->GetShape())->GetRmin();
  }

  //......................................................................
  double OpDetGeo::ThetaZ(bool degrees) const
  {
    static double xyzCenter[3] = {0,0,0};
    static double xyzEnd[3] = {0,0,0};
    static double halfL =this->HalfL();
    this->GetCenter(xyzCenter,0.0);
    this->GetCenter(xyzEnd,halfL);
    //either y or x will be 0, so ading both will always catch the right
    //one
    double angle = (xyzEnd[1]-xyzCenter[1]+xyzEnd[0]-xyzCenter[0]) / 
      TMath::Abs(xyzEnd[1]-xyzCenter[1]+xyzCenter[0]-xyzEnd[0]) * 
      TMath::ACos((xyzEnd[2] - xyzCenter[2])/halfL);
    if(angle < 0) angle +=TMath::Pi(); 
    if(degrees) angle*=180.0/TMath::Pi();
    return angle;
  }

  

  //......................................................................
  // Get the distance from some point to this detector
  double OpDetGeo::DistanceToPoint(double const* xyz) const
  {
    double Center[3];
    GetCenter(Center);
    return std::sqrt(
      sqr(Center[0]-xyz[0]) +
      sqr(Center[1]-xyz[1]) +
      sqr(Center[2]-xyz[2]));
  }


  //......................................................................
  // Get cos(angle) to normal of this detector - used for solid angle calcs
  double OpDetGeo::CosThetaFromNormal(double const* xyz) const
  {
    double local[3];
    WorldToLocal(xyz, local);
    return local[2] / 
      std::sqrt(sqr(local[0]) +
	   sqr(local[1]) +
	   sqr(local[2]));

  }


}
////////////////////////////////////////////////////////////////////////
