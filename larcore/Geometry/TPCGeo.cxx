////////////////////////////////////////////////////////////////////////
/// \file TPCGeo.cxx
///
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////


// ROOT includes
#include "TMath.h"
#include "TVector3.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"
#include "TGeoBBox.h"

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// LArSoft includes
#include "larcore/Geometry/TPCGeo.h"
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"
#include "larcore/CoreUtils/RealComparisons.h"

// C/C++ standard libraries
#include <cmath>
#include <cassert>
#include <map>
#include <algorithm> // std::max()


namespace geo{


  //......................................................................
  TVector3 const TPCGeo::DirX(1.0, 0.0, 0.0);
  TVector3 const TPCGeo::DirY(0.0, 1.0, 0.0);
  TVector3 const TPCGeo::DirZ(0.0, 0.0, 1.0);
  
  //......................................................................
  TPCGeo::TPCGeo(GeoNodePath_t& path, size_t depth)
    : BoxBoundedGeo() // we initialize boundaries at the end of construction
    , fTrans(path, depth)
    , fActiveVolume(0)
    , fTotalVolume(0)
    , fDriftDirection(geo::kUnknownDrift)
    , fWidthDir (DirX)
    , fHeightDir(DirY)
    , fLengthDir(DirZ)
    , fDriftDir() // null until known
  {
    
    // all planes are going to be contained in the volume named volTPC
    // now get the total volume of the TPC
    TGeoVolume *vc = path[depth]->GetVolume();
    if(!vc){ 
      throw cet::exception("Geometry") << "cannot find detector outline volume - bail ungracefully\n";
    }
    
    fTotalVolume = vc;
    
    // loop over the daughters of this node and look for the active volume
    int nd = vc->GetNdaughters();
    TGeoNode const* pActiveVolNode = nullptr;
    for(int i = 0; i < nd; ++i){
      if(strncmp(vc->GetNode(i)->GetName(), "volTPCActive", 12) != 0) continue;
      
      pActiveVolNode = vc->GetNode(i);
      TGeoVolume *vca = pActiveVolNode->GetVolume();
      if(vca) fActiveVolume = vca;
      break;
      
    }// end loop over daughters of the volume
    
    if(!fActiveVolume) fActiveVolume = fTotalVolume;
    
    LOG_DEBUG("Geometry") << "detector total  volume is " << fTotalVolume->GetName()
                          << "\ndetector active volume is " << fActiveVolume->GetName();

    // compute the active volume transformation too
    TGeoHMatrix ActiveHMatrix(fTrans.Matrix());
    if (pActiveVolNode) ActiveHMatrix.Multiply(pActiveVolNode->GetMatrix());
    // we don't keep the active volume information... just store its center:
    std::array<double, 3> localActiveCenter, worldActiveCenter;
    localActiveCenter.fill(0.0);
    ActiveHMatrix.LocalToMaster
      (localActiveCenter.data(), worldActiveCenter.data());
    fActiveCenter = TVector3(worldActiveCenter.data());
    
    
    // find the wires for the plane so that you can use them later
    this->FindPlane(path, depth);

    // set the width, height, and lengths
    fActiveHalfWidth  =     ((TGeoBBox*)fActiveVolume->GetShape())->GetDX();
    fActiveHalfHeight =     ((TGeoBBox*)fActiveVolume->GetShape())->GetDY();
    fActiveLength     = 2.0*((TGeoBBox*)fActiveVolume->GetShape())->GetDZ();

    fHalfWidth  =     ((TGeoBBox*)fTotalVolume->GetShape())->GetDX();
    fHalfHeight =     ((TGeoBBox*)fTotalVolume->GetShape())->GetDY();
    fLength     = 2.0*((TGeoBBox*)fTotalVolume->GetShape())->GetDZ();

    // check that the rotation matrix to the world is the identity, if not
    // we need to change the width, height and length values
    double const* rotMatrix = fTrans.Matrix().GetRotationMatrix();
    if(rotMatrix[0] != 1){
      if(std::abs(rotMatrix[2]) == 1){
        fActiveHalfWidth = ((TGeoBBox*)fActiveVolume->GetShape())->GetDZ();
        fHalfWidth       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDZ();
        fWidthDir        = DirZ;
      }
      if(std::abs(rotMatrix[1]) == 1){
        fActiveHalfWidth = ((TGeoBBox*)fActiveVolume->GetShape())->GetDY();
        fHalfWidth       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDY();
        fWidthDir        = DirY;
      }
    }
    if(rotMatrix[4] != 1){
      if(std::abs(rotMatrix[3]) == 1){
        fActiveHalfHeight = ((TGeoBBox*)fActiveVolume->GetShape())->GetDX();
        fHalfHeight       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDX();
        fHeightDir        = DirX;
      }
      if(std::abs(rotMatrix[5]) == 1){
        fActiveHalfHeight = ((TGeoBBox*)fActiveVolume->GetShape())->GetDZ();
        fHalfHeight       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDZ();
        fHeightDir        = DirZ;
      }
    }
    if(rotMatrix[8] != 1){
      if(std::abs(rotMatrix[6]) == 1){
        fActiveLength = 2.*((TGeoBBox*)fActiveVolume->GetShape())->GetDX();
        fLength       = 2.*((TGeoBBox*)fTotalVolume->GetShape())->GetDX();
        fLengthDir    = DirX;
      }
      if(std::abs(rotMatrix[7]) == 1){
        fActiveLength = 2.*((TGeoBBox*)fActiveVolume->GetShape())->GetDY();
        fLength       = 2.*((TGeoBBox*)fTotalVolume->GetShape())->GetDY();
        fLengthDir    = DirY;
      }
    }
    
    InitTPCBoundaries();
    ResetDriftDirection();
    
  } // TPCGeo::TPCGeo()

  //......................................................................
  TPCGeo::~TPCGeo()
  {
    for(unsigned int i = 0; i < fPlanes.size(); ++i)
      if(fPlanes[i]) delete fPlanes[i];
  
    fPlanes.clear();

  }

  //......................................................................
  void TPCGeo::FindPlane(GeoNodePath_t& path, size_t depth) 
  {

    const char* nm = path[depth]->GetName();
    if( (strncmp(nm, "volTPCPlane", 11) == 0) ){
      this->MakePlane(path,depth);
      return;
    }

    //explore the next layer down
    unsigned int deeper = depth+1;
    if(deeper >= path.size()){
      throw cet::exception("BadTGeoNode") << "exceeded maximum TGeoNode depth\n";
    }

    const TGeoVolume *v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for(int i = 0; i < nd; ++i){
      path[deeper] = v->GetNode(i);
      this->FindPlane(path, deeper);
    }
  
  }


  //......................................................................
  void TPCGeo::MakePlane(GeoNodePath_t& path, size_t depth) 
  {
    fPlanes.push_back(new PlaneGeo(path, depth));
  }


  //......................................................................
  short int TPCGeo::DetectDriftDirection() const {
    
    //
    // 1. determine the drift axis
    // 2. determine the drift direction on it
    // 
    // We assume that all the planes cover most of the TPC face; therefore,
    // the centre of the plane and the one of the TPC should be very close
    // to each other, when projected on the same drift plane.
    // Here we find which is the largest coordinate difference.
    
    if (Nplanes() == 0) {
      // chances are that we get this because stuff is not initialised yet,
      // and then even the ID might be wrong
      throw cet::exception("TPCGeo")
        << "DetectDriftDirection(): no planes in TPC " << std::string(ID())
        << "\n";
    }
    
    auto const TPCcenter = GetCenter();
    auto const PlaneCenter = Plane(0).GetBoxCenter(); // any will do
    
    auto const driftVector = PlaneCenter - TPCcenter; // approximation!
    
    if ((std::abs(driftVector.X()) > std::abs(driftVector.Y()))
      && (std::abs(driftVector.X()) > std::abs(driftVector.Z())))
    {
      // x is the solution
      return (driftVector.X() > 0)? +1: -1;
    }
    else if (std::abs(driftVector.Y()) > std::abs(driftVector.Z()))
    {
      // y is the man
      return (driftVector.Y() > 0)? +2: -2;
    }
    else {
      // z is the winner
      return (driftVector.Z() > 0)? +3: -3;
    }
    
  } // TPCGeo::DetectDriftDirection()
  
  //......................................................................
  // sort the PlaneGeo objects and the WireGeo objects inside 
  void TPCGeo::SortSubVolumes(geo::GeoObjectSorter const& sorter)
  {
    fPlanes = SortPlanes(fPlanes);
    
    double origin[3] = {0.};
 
    // set the plane pitch for this TPC
    double xyz[3]  = {0.};
    fPlanes[0]->LocalToWorld(origin,xyz);
    double xyz1[3] = {0.};
    fPlaneLocation.clear();
    fPlaneLocation.resize(fPlanes.size());
    for(unsigned int i = 0; i < fPlaneLocation.size(); ++i) fPlaneLocation[i].resize(3);
    fPlane0Pitch.clear();
    fPlane0Pitch.resize(this->Nplanes(), 0.);
    for(size_t p = 0; p < this->Nplanes(); ++p){
      fPlanes[p]->LocalToWorld(origin,xyz1);
      if(p > 0) fPlane0Pitch[p] = fPlane0Pitch[p-1] + std::abs(xyz1[0]-xyz[0]);
      else      fPlane0Pitch[p] = 0.;
      xyz[0] = xyz1[0];
      fPlaneLocation[p][0] = xyz1[0];
      fPlaneLocation[p][1] = xyz1[1];
      fPlaneLocation[p][2] = xyz1[2];
    }

    // the PlaneID_t cast convert InvalidID into a rvalue (non-reference);
    // leaving it a reference would cause C++ to treat it as such,
    // that can't be because InvalidID is a static member constant without an address
    // (it is not defined in any translation unit, just declared in header)
    fViewToPlaneNumber.resize
      (1U + (size_t) geo::kUnknown, (geo::PlaneID::PlaneID_t) geo::PlaneID::InvalidID);
    for(size_t p = 0; p < this->Nplanes(); ++p)
      fViewToPlaneNumber[(size_t) fPlanes[p]->View()] = p;

    for(size_t p = 0; p < fPlanes.size(); ++p) fPlanes[p]->SortWires(sorter);
    
  }


  //......................................................................
  void TPCGeo::UpdateAfterSorting(geo::TPCID tpcid) {
    
    // reset the ID
    fID = tpcid;
    
    // ask the planes to update; also check
    
    for (unsigned int plane = 0; plane < Nplanes(); ++plane) {
      fPlanes[plane]->UpdateAfterSorting(geo::PlaneID(fID, plane), *this);
      
      // check that the plane normal is opposite to the TPC drift direction
      assert(lar::util::makeVector3DComparison(1e-5)
        .equal(-(fPlanes[plane]->GetNormalDirection()), DriftDir()));
      
    } // for
    
    UpdatePlaneViewCache();
    
  } // TPCGeo::UpdateAfterSorting()
  
  
  //......................................................................
  const PlaneGeo& TPCGeo::Plane(unsigned int iplane) const
  {
    geo::PlaneGeo const* pPlane = PlanePtr(iplane);
    if (!pPlane){
      throw cet::exception("PlaneOutOfRange") << "Request for non-existant plane " << iplane << "\n";
    }

    return *pPlane;
  }

  //......................................................................
  const PlaneGeo& TPCGeo::Plane(geo::View_t view) const
  {
    geo::PlaneID::PlaneID_t const p = fViewToPlaneNumber[size_t(view)];
    if (p == geo::PlaneID::InvalidID) {
      throw cet::exception("TPCGeo")
        << "TPCGeo[" << ((void*) this) << "]::Plane(): no plane for view #"
        << (size_t) view << "\n";
    }
    return *fPlanes[p];
  } // TPCGeo::Plane(geo::View_t)

  
  //......................................................................
  geo::PlaneGeo const& TPCGeo::SmallestPlane() const {
    
    //
    // Returns the plane with the smallest width x depth. No nonsense here.
    //
    
    auto iPlane = fPlanes.begin(), pend = fPlanes.end();
    geo::PlaneGeo const* smallestPlane = *iPlane;
    double smallestSurface = smallestPlane->Width() * smallestPlane->Depth();
    while (++iPlane != pend) {
      double const surface = (*iPlane)->Width() * (*iPlane)->Depth();
      if (surface > smallestSurface) continue;
      smallestSurface = surface;
      smallestPlane = *iPlane;
    } // while
    return *smallestPlane;
    
  } // TPCGeo::SmallestPlane()
  
  
  //......................................................................
  unsigned int TPCGeo::MaxWires() const {
    unsigned int maxWires = 0;
    for (geo::PlaneGeo const* pPlane: fPlanes) {
      if (!pPlane) continue;
      unsigned int maxWiresInPlane = pPlane->Nwires();
      if (maxWiresInPlane > maxWires) maxWires = maxWiresInPlane;
    } // for
    return maxWires;
  } // TPCGeo::MaxWires()
  
  
  //......................................................................
  // returns distance between plane 0 to each of the remaining planes 
  // not the distance between two consecutive planes  
  double TPCGeo::Plane0Pitch(unsigned int p) const
  {
    return fPlane0Pitch[p];
  }

  //......................................................................
  TVector3 TPCGeo::GetCenter() const {
    
    // convert the origin (default constructed TVector)
    return LocalToWorld({});
    
  } // TPCGeo::GetCenter()
  
  
  //......................................................................
  TVector3 TPCGeo::GetCathodeCenter() const {
    
    //
    // 1. find the center of the face of the TPC opposite to the anode
    // 2. compute the distance of it from the last wire plane
    //
    
    //
    // find the cathode center
    //
    TVector3 cathodeCenter = GetActiveVolumeCenter();
    switch (DetectDriftDirection()) {
      case -1:
        cathodeCenter.SetX(cathodeCenter.X() + ActiveHalfWidth());
        break;
      case +1:
        cathodeCenter.SetX(cathodeCenter.X() - ActiveHalfWidth());
        break;
      case -2:
        cathodeCenter.SetY(cathodeCenter.Y() + ActiveHalfHeight());
        break;
      case +2:
        cathodeCenter.SetY(cathodeCenter.Y() - ActiveHalfHeight());
        break;
      case -3:
        cathodeCenter.SetZ(cathodeCenter.Z() + ActiveLength() / 2.0);
        break;
      case +3:
        cathodeCenter.SetZ(cathodeCenter.Z() - ActiveLength() / 2.0);
        break;
      case 0:
      default:
        // in this case, a better algorithm is probably needed
        throw cet::exception("TPCGeo")
          << "CathodeCenter(): Can't determine the cathode plane (code="
          << DetectDriftDirection() << ")\n";
    } // switch
    return cathodeCenter;
    
  } // TPCGeo::GetCathodeCenter()
  
  
  //......................................................................
  // returns xyz location of planes in TPC
  const double* TPCGeo::PlaneLocation(unsigned int p) const
  {
    return &fPlaneLocation[p][0];
  }

  //......................................................................
  double TPCGeo::PlanePitch(unsigned int p1, 
                            unsigned int p2) const
  {
    return std::abs(fPlane0Pitch[p2] - fPlane0Pitch[p1]);
  }

  //......................................................................
  // This method returns the distance between the specified wires.
  // w1 < w2.  The wires are assumed to be on the same plane
  double TPCGeo::WirePitch(unsigned int /*w1*/,  
			   unsigned int /*w2*/,  
			   unsigned int plane) const
  { 
    return this->Plane(plane).WirePitch();
  }

  //......................................................................
  void TPCGeo::ResetDriftDirection() {
    
    auto const driftDirCode = DetectDriftDirection();
    switch (driftDirCode) {
      case +1:
        fDriftDirection = geo::kPosX; // this is the same as kPos!
        fDriftDir = DirX;
        break;
      case -1:
        fDriftDirection = geo::kNegX; // this is the same as kNeg!
        fDriftDir = -DirX;
        break;
      case +2:
        fDriftDir = DirY;
        fDriftDirection = geo::kPos;
        break;
      case -2:
        fDriftDir = -DirY;
        fDriftDirection = geo::kNeg;
        break;
      case +3:
        fDriftDir = DirZ;
        fDriftDirection = geo::kPos;
        break;
      case -3:
        fDriftDir = -DirZ;
        fDriftDirection = geo::kNeg;
        break;
      default:
        // TPC ID is likely not yet set
        fDriftDirection = kUnknownDrift;
        
        // we estimate the drift direction roughly from the geometry
        fDriftDir = Plane(0).GetBoxCenter() - GetCenter();
        
        mf::LogError("TPCGeo")
          << "Unable to detect drift direction (result: " << driftDirCode
          << ", drift: ( " << fDriftDir.X() << " ; " << fDriftDir.Y() << " ; "
          << fDriftDir.Z() << " )";
        break;
    } // switch
    
    geo::PlaneGeo::roundVector(fDriftDir, 1e-4);
    
  } // TPCGeo::ResetDriftDirection()
  
  
  //......................................................................
  double TPCGeo::ComputeDriftDistance() const {
    
    //
    // 1. find the center of the face of the TPC opposite to the anode
    // 2. compute the distance of it from the last wire plane
    //
    
    geo::PlaneGeo const& plane = *(fPlanes.back());
    return std::abs(plane.DistanceFromPlane(GetCathodeCenter()));
    
  } // TPCGeo::ComputeDriftDistance()
  
  
  //......................................................................
  void TPCGeo::InitTPCBoundaries() {
    // note that this assumes no rotations of the TPC
    // (except for rotations of a flat angle around one of the three main axes);
    // to avoid this, we should transform the six vertices
    // rather than just the centre
    
    std::array<double, 3> origin, world;
    origin.fill(0.);
    LocalToWorld(origin.data(), world.data());
    
    // y and z values are easy and can be figured out using the TPC origin
    // the x values are a bit trickier, at least the -x value seems to be
    
    SetBoundaries(
      world[0] - HalfWidth(),  world[0] + HalfWidth(),
      world[1] - HalfHeight(), world[1] + HalfHeight(),
      world[2] - 0.5*Length(), world[2] + 0.5*Length()
      );
    
  } // CryostatGeo::InitTPCBoundaries()

  //......................................................................
  
  void TPCGeo::UpdatePlaneViewCache() {
    
    // the PlaneID_t cast convert InvalidID into a rvalue (non-reference);
    // leaving it a reference would cause C++ to treat it as such,
    // that can't be because InvalidID is a static member constant without an address
    // (it is not defined in any translation unit, just declared in header)
    fViewToPlaneNumber.clear();
    fViewToPlaneNumber.resize
      (1U + (size_t) geo::kUnknown, (geo::PlaneID::PlaneID_t) geo::PlaneID::InvalidID);
    for(size_t p = 0; p < Nplanes(); ++p)
      fViewToPlaneNumber[(size_t) fPlanes[p]->View()] = p;
    
  } // TPCGeo::UpdatePlaneViewCache()
  

  //......................................................................
  std::vector<geo::PlaneGeo*> TPCGeo::SortPlanes
    (std::vector<geo::PlaneGeo*> const& planes) const
  {
    //
    // Sort planes by increasing drift distance.
    // 
    // This function should work in bootstrap mode, relying on least things as
    // possible. Therefore we compute here a proxy of the drift axis.
    //
    
    //
    // determine the drift axis (or close to): from TPC center to plane center
    // 
    
    // Instead of using the plane center, which might be not available yet,
    // we use the plane box center, which only needs the geometry description
    // to be available.
    // We use the first plane -- it does not make any difference.
    decltype(auto) TPCcenter = GetCenter();
    auto driftAxis
      = geo::vect::Normalize(planes[0]->GetBoxCenter() - TPCcenter);
    
    //
    // associate each plane with its distance from the center of TPC
    //
    decltype(auto) center = GetCenter();
    // pair: <plane pointer,drift distance>
    std::vector<std::pair<geo::PlaneGeo*, double>> planesWithDistance;
    planesWithDistance.reserve(planes.size());
    for (geo::PlaneGeo* plane: planes) {
      double const driftDistance
        = geo::vect::Dot(plane->GetBoxCenter() - TPCcenter, driftAxis);
      planesWithDistance.emplace_back(plane, driftDistance);
    } // for
    
    //
    // sort by distance
    //
    std::sort(planesWithDistance.begin(), planesWithDistance.end(), 
      [](auto const& a, auto const& b){ return a.second < b.second; }
      );
    
    //
    // extract the result
    //
    std::vector<geo::PlaneGeo*> sortedPlanes;
    sortedPlanes.reserve(planesWithDistance.size());
    for (auto const& pair: planesWithDistance)
      sortedPlanes.push_back(pair.first);
    
    return sortedPlanes;
  } // TPCGeo::SortPlanes()
  
  //......................................................................
  
}
////////////////////////////////////////////////////////////////////////
