////////////////////////////////////////////////////////////////////////
/// \file  WireGeo.cxx
/// \brief Encapsulate the geometry of a wire
///
/// \version $Id: WireGeo.cxx,v 1.8 2010/03/05 05:30:56 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cmath>

#include "Geometry/WireGeo.h"

#include "TGeoManager.h"
#include "TGeoTube.h"
#include "TGeoMatrix.h"
#include "TGeoNode.h"
#include "TMath.h"

#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace geo{

  /// Construct a wire geometry
  /// \param path  : List of TGeoNodes that take us to this wire
  /// \param depth : Size of "path" list

  //-----------------------------------------
  WireGeo::WireGeo(std::vector<const TGeoNode*>& path, int depth) 
  {
    fWireNode = path[depth];
    fHalfL    = ((TGeoTube*)fWireNode->GetVolume()->GetShape())->GetDZ();
    fRMax     = ((TGeoTube*)fWireNode->GetVolume()->GetShape())->GetRmax(); 
    fRMin     = ((TGeoTube*)fWireNode->GetVolume()->GetShape())->GetRmin(); 

    /// uncomment the following to check the paths to the wires
    ///   std::string p(base);
    ///   for(int i = 0; i <= depth; ++i){
    ///     p += "/";
    ///     p += path[i]->GetName();
    ///   }
    ///   std::cout << p.c_str() << std::endl;
  
    // build a matrix to take us from the local to the world coordinates
    // in one step
    TGeoHMatrix mat(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i) mat.Multiply(path[i]->GetMatrix());

    fGeoMatrix = new TGeoHMatrix(mat);

    // determine the orientation of the wire
    double local[3]    = {0.};
    double xyzCenter[3] = {0.};
    double xyzEnd[3] = {0.};
    double xyzStart[3] = {0.};

    this->LocalToWorld(local, xyzCenter);
    fCenter = TVector3(xyzCenter);
    local[2] = fHalfL;
    this->LocalToWorld(local, xyzEnd);
    local[2] *= -1.;
    this->LocalToWorld(local, xyzStart);

    // get the cosines in each direction, ie dx/dS, etc
    fDirection = TVector3((xyzEnd[0]-xyzStart[0])/(2.*fHalfL),
			  (xyzEnd[1]-xyzStart[1])/(2.*fHalfL),
			  (xyzEnd[2]-xyzStart[2])/(2.*fHalfL));

    fThetaZ = std::acos((xyzEnd[2] - xyzCenter[2])/fHalfL);
    
    // check to see if it runs "forward" or "backwards" in z
    // check is made looking at the y position of the end point
    // relative to the center point because you want to know if
    // the end point is above or below the center of the wire in 
    // the yz plane
    if(xyzEnd[1] < xyzCenter[1]) fThetaZ *= -1.;

    //This ensures we are looking at the angle between 0 and Pi
    //as if the wire runs at one angle it also runs at that angle +-Pi
    if(fThetaZ < 0) fThetaZ +=TMath::Pi(); 
    
    return;
  }

  //......................................................................
  WireGeo::~WireGeo()
  {
    //if(fGeoMatrix) delete fGeoMatrix;
  
    return;
  }  

  //......................................................................

  /// Transform a position from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void WireGeo::LocalToWorld(const double* local, double* world) const
  {
    fGeoMatrix->LocalToMaster(local,world);
  }

  //......................................................................    

  /// Transform a 3-vector from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void WireGeo::LocalToWorldVect(const double* local, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(local,world);
  }
    
  //......................................................................

  /// Transform a position from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void WireGeo::WorldToLocal(const double* local, double* world) const
  {
    fGeoMatrix->MasterToLocal(local,world);
  }

  //......................................................................

  /// Transform a 3-vector from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void WireGeo::WorldToLocalVect(const double* local, double* world) const
  {
    fGeoMatrix->MasterToLocalVect(local,world);
  }

  //......................................................................

  /// Return the center position of a wire.
  /// \param xyz : 3-D array. The returned location.
  /// \param localz : Distance along the length of the wire
  /// (cm). Default is center of wire
  void WireGeo::GetCenter(double* xyz, double localz) const
  {
    double locz = localz;
    if(std::abs(locz) > fHalfL){
      mf::LogWarning("WireGeo") << "asked for z position along wire that "
				<< "extends beyond the wire, returning position "
				<< "at end point";
      if(locz < 0) locz = -fHalfL;
      else         locz =  fHalfL;
    }

    TVector3 point(fCenter);
    point += fDirection*locz;

    xyz[0] = point.X();
    xyz[1] = point.Y();
    xyz[2] = point.Z();
  }

  //......................................................................

  double WireGeo::RMax() const 
  {
    return fRMax;
  }
    
  //......................................................................

  double WireGeo::HalfL() const 
  {
    return fHalfL;
  }

  //......................................................................

  double WireGeo::RMin() const 
  {
    return fRMin;
  }

  //......................................................................
  double WireGeo::ThetaZ(bool degrees) const
  {
    if(degrees) return fThetaZ*180.0/TMath::Pi();
    return fThetaZ;
  }

}
////////////////////////////////////////////////////////////////////////
