////////////////////////////////////////////////////////////////////////
/// \file  AuxDetGeo.cxx
/// \brief Encapsulate the geometry of an auxilary detector
///
/// \author  miceli@fnal.gov
////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "Geometry/AuxDetGeo.h"

#include "TGeoManager.h"
#include "TGeoTube.h"
#include "TGeoMatrix.h"
#include "TGeoNode.h"
#include "TMath.h"
#include "TVector3.h"

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

namespace geo{
  
  //-----------------------------------------
  AuxDetGeo::AuxDetGeo(std::vector<const TGeoNode*>& path, int depth)
  : fTotalVolume(0)
  {
    
    TGeoVolume *vc = path[depth]->GetVolume();
    if(vc){
      fTotalVolume = vc;
      if(!fTotalVolume)
        throw cet::exception("AuxDetGeo") << "cannot find AuxDet volume";
      
    }// end if found volume
    
    LOG_DEBUG("Geometry") << "detector total  volume is " << fTotalVolume->GetName();
    
    // Build the matrix that takes us to the top world frame
    // build a matrix to take us from the local to the world coordinates
    // in one step
    fGeoMatrix = new TGeoHMatrix(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i){
      fGeoMatrix->Multiply(path[i]->GetMatrix());
    }
    
    // set the width, height, and length
    fHalfWidth  =     ((TGeoBBox*)fTotalVolume->GetShape())->GetDX();
    fHalfHeight =     ((TGeoBBox*)fTotalVolume->GetShape())->GetDY();
    fLength     = 2.0*((TGeoBBox*)fTotalVolume->GetShape())->GetDZ();
  }
  
  //......................................................................
  AuxDetGeo::~AuxDetGeo()
  {
    if(fGeoMatrix) delete fGeoMatrix;
    
    return;
  }
  
  //......................................................................
  
  /// Transform a position from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void AuxDetGeo::LocalToWorld(const double* local, double* world) const
  {
    fGeoMatrix->LocalToMaster(local,world);
  }
  
  //......................................................................
  
  /// Transform a 3-vector from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void AuxDetGeo::LocalToWorldVect(const double* local, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(local,world);
  }
  
  //......................................................................
  
  /// Transform a position from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void AuxDetGeo::WorldToLocal(const double* world, double* local) const
  {
    fGeoMatrix->MasterToLocal(world,local);
  }
  
  //......................................................................
  
  /// Transform a 3-vector from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void AuxDetGeo::WorldToLocalVect(const double* world, double* local) const
  {
    fGeoMatrix->MasterToLocalVect(world,local);
  }
  
  //......................................................................
  
  /// Return the center position of an AuxDet
  /// \param xyz : 3-D array. The returned location.
  /// \param localz : Distance along the length of the volume (z)
  /// (cm). Default is 0.
  void AuxDetGeo::GetCenter(double* xyz, double localz) const
  {
    double xyzLocal[3] = {0.,0.,localz};
    this->LocalToWorld(xyzLocal, xyz);
  }
  
  //......................................................................
  
  // Return the unit normal vector (0,0,1) in local coordinates to global coordinates
  void AuxDetGeo::GetNormalVector(double* xyzDir) const
  {
    double normal[3]={0.,0.,1.};
    this->LocalToWorldVect(normal,xyzDir);
  }
  
  
  //......................................................................
  
  // Get the distance from some point to this detector, xyz in global coordinates
  double AuxDetGeo::DistanceToPoint(double * xyz) const
  {
    double Center[3];
    GetCenter(Center);
    return std::sqrt((Center[0]-xyz[0])*(Center[0]-xyz[0]) +
                     (Center[1]-xyz[1])*(Center[1]-xyz[1]) +
                     (Center[2]-xyz[2])*(Center[2]-xyz[2]));
  }
}
////////////////////////////////////////////////////////////////////////
