////////////////////////////////////////////////////////////////////////
/// \file  WireGeo.cxx
/// \brief Encapsulate the geometry of a wire
///
/// \version $Id: WireGeo.cxx,v 1.8 2010/03/05 05:30:56 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "larcore/Geometry/WireGeo.h"

// C/C++ libraries
#include <cmath>
#include <algorithm> // std::copy()

// ROOT
#include "TGeoManager.h"
#include "TGeoTube.h"
#include "TGeoMatrix.h"
#include "TGeoNode.h"
#include "TMath.h"

// CLHEP
#include "CLHEP/Vector/RotationInterfaces.h" // HepGeom::HepRep3x3
#include "CLHEP/Vector/Rotation.h" // CLHEP::HepRotation
#include "CLHEP/Vector/ThreeVector.h" // CLHEP::Hep3Vector
#include "CLHEP/Geometry/Vector3D.h"
#include "CLHEP/Geometry/Transform3D.h"

// framework
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"


namespace geo{

  /// Construct a wire geometry
  /// \param path  : List of TGeoNodes that take us to this wire
  /// \param depth : Size of "path" list

  //-----------------------------------------
  WireGeo::WireGeo(std::vector<const TGeoNode*>& path, int depth) 
    : flipped(false)
  {
    fWireNode = path[depth];
    fHalfL    = ((TGeoTube*)fWireNode->GetVolume()->GetShape())->GetDZ();

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
    
    const Double_t* translation = mat.GetTranslation();
    // there are not many ways to set a HepGeom::Transform3D...
    fGeoMatrix = HepGeom::Transform3D(
      CLHEP::HepRotation(CLHEP::HepRep3x3(mat.GetRotationMatrix())),
      CLHEP::Hep3Vector(translation[0], translation[1], translation[2])
      );
    
    // determine the orientation of the wire
    double local[3] = { 0., 0., 0. };
    LocalToWorld(local, fCenter);

    double xyzEnd[3];
    local[2] = fHalfL;
    LocalToWorld(local, xyzEnd);
    
    fThetaZ = std::acos((xyzEnd[2] - fCenter[2])/fHalfL);
    
    // check to see if it runs "forward" or "backwards" in z
    // check is made looking at the y position of the end point
    // relative to the center point because you want to know if
    // the end point is above or below the center of the wire in 
    // the yz plane
    if(xyzEnd[1] < fCenter[1]) fThetaZ *= -1.;

    //This ensures we are looking at the angle between 0 and Pi
    //as if the wire runs at one angle it also runs at that angle +-Pi
    if(fThetaZ < 0) fThetaZ +=TMath::Pi(); 
    
    return;
  }

  //......................................................................

  /// Transform a position from local frame to world frame
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void WireGeo::LocalToWorld(const double* local, double* world) const
  {
    HepGeom::Point3D<double> worldPoint
      = fGeoMatrix * HepGeom::Point3D<double>(local);
    std::copy((const double*) worldPoint, (const double*) worldPoint + 3, world);
  }

  //......................................................................    

  /// Transform a 3-vector from local frame to world frame (rotation only)
  /// \param local : 3D array. Position in the local frame  Input.
  /// \param world : 3D array. Position in the world frame. Returned.
  void WireGeo::LocalToWorldVect(const double* local, double* world) const
  {
    HepGeom::Vector3D<double> worldVect
      = fGeoMatrix * HepGeom::Vector3D<double>(local);
    std::copy((const double*) worldVect, (const double*) worldVect + 3, world);
  }
  
  //......................................................................

  /// Transform a position from world frame to local frame
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void WireGeo::WorldToLocal(const double* world, double* local) const
  {
    HepGeom::Point3D<double> localPoint
      = fGeoMatrix.inverse() * HepGeom::Point3D<double>(world);
    std::copy((const double*) localPoint, (const double*) localPoint + 3, local);
  }

  //......................................................................

  /// Transform a 3-vector from world frame to local frame (rotation only)
  /// \param world : 3D array. Position in the world frame. Input.
  /// \param local : 3D array. Position in the local frame  Returned.
  void WireGeo::WorldToLocalVect(const double* world, double* local) const
  {
    HepGeom::Vector3D<double> localVect
      = fGeoMatrix.inverse() * HepGeom::Vector3D<double>(world);
    std::copy((const double*) localVect, (const double*) localVect + 3, local);
  } // WireGeo::WorldToLocalVect()

  //......................................................................

  /// Return the center position of a wire.
  /// \param xyz : 3-D array. The returned location.
  /// \param localz : Distance along the length of the wire
  /// (cm). Default is center of wire
  void WireGeo::GetCenter(double* xyz, double localz) const
  {
    if (localz == 0.) { // if no dislocation is requested, we already have it
      std::copy(fCenter, fCenter + 3, xyz);
      return;
    }
    
    double locz = relLength(localz);
    if (std::abs(locz) > fHalfL) {
      mf::LogWarning("WireGeo") << "asked for position along wire that"
        " extends beyond the wire, returning position at end point";
      locz = relLength((locz < 0)? -fHalfL: fHalfL);
    }
    const double local[3] = { 0., 0., locz };
    LocalToWorld(local, xyz);
  }

  //......................................................................
  TVector3 geo::WireGeo::Direction() const {
    // to make it faster, we could compare GetStart() with GetCenter()
    
    // determine the orientation of the wire
    double xyzEnd[3], xyzStart[3];
    GetStart(xyzStart);
    GetEnd(xyzEnd);

    // get the cosines in each direction, ie dx/dS, etc
    return TVector3(
      (xyzEnd[0] - xyzStart[0]) / Length(),
      (xyzEnd[1] - xyzStart[1]) / Length(),
      (xyzEnd[2] - xyzStart[2]) / Length()
      );                     
  } // geo::WireGeo::Direction()
  
  //......................................................................
  double geo::WireGeo::RMax() const
    { return ((TGeoTube*)fWireNode->GetVolume()->GetShape())->GetRmax(); }
  
  //......................................................................
  double geo::WireGeo::RMin() const
    { return ((TGeoTube*)fWireNode->GetVolume()->GetShape())->GetRmin(); }
  
  //......................................................................
  
  double WireGeo::HalfL() const 
  {
    return fHalfL;
  }

  //......................................................................
  double WireGeo::ThetaZ(bool degrees) const
  {
    if(degrees) return fThetaZ*180.0/TMath::Pi();
    return fThetaZ;
  }

  //......................................................................
  double WireGeo::DistanceFrom(geo::WireGeo const& wire) const {
    //
    // The algorithm assumes that picking any point on the wire will do,
    // that is, that the wires are parallel.
    //
    
    if (!isParallelTo(wire)) return 0;
    
    // a vector connecting to the other wire
    auto toWire = wire.GetCenter() - GetCenter();
    
    // the distance is that vector, times the sine of the angle with the wire
    // direction; we get that in a generic way with a cross product.
    // We don't even care about the sign here
    // (if we did, we would do a dot-product with the normal to the plane,
    // and we should get a positive distance if the other wire has larger wire
    // coordinate than this one).
    return toWire.Cross(Direction()).Mag();
    
  } // WireGeo::DistanceFrom()
    
    
  //......................................................................
  void WireGeo::UpdateAfterSorting(geo::WireID const&, bool flip) {
    
    // flip, if asked
    if (flip) Flip();
    
  } // WireGeo::UpdateAfterSorting()
  
  //......................................................................
  void WireGeo::Flip() {
    // we don't need to do much to flip so far:
    // - ThetaZ() is defined in [0, pi], invariant to flipping
    // - we don't change the transformation matrices, that we want to be
    //   untouched and coherent with the original geometry source
    // - center is invariant for flipping
    // - start and end are computed on the fly (taking flipping into account)
    // - ... and we chose to leave half length unsigned and independent
    
    // change the flipping bit
    flipped = !flipped;
    
  } // WireGeo::Flip()

  //......................................................................
  
}
////////////////////////////////////////////////////////////////////////
