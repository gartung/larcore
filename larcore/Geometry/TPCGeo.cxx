////////////////////////////////////////////////////////////////////////
/// \file TPCGeo.cxx
///
/// \version $Id: TPCGeo.cxx,v 1.12 2010/03/05 19:47:51 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////


// ROOT includes
#include "TMath.h"
#include "TVector3.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"
#include <TGeoBBox.h>

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// LArSoft includes
#include "larcore/Geometry/TPCGeo.h"
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"

// C/C++ standard libraries
#include <cmath>
#include <map>
#include <algorithm> // std::max()


namespace geo{


  //......................................................................
  TPCGeo::TPCGeo(std::vector<const TGeoNode*>& path, int depth)
    : BoxBoundedGeo() // we initialize boundaries at the end of construction
    , fActiveVolume(0)
    , fTotalVolume(0)
    , fDriftDirection(geo::kUnknownDrift)
  {
    
    // all planes are going to be contained in the volume named volTPC
    // now get the total volume of the TPC
    TGeoVolume *vc = path[depth]->GetVolume();
    if(vc){
      fTotalVolume = vc;
      if(!vc){ 
	throw cet::exception("Geometry") << "cannot find detector outline volume - bail ungracefully\n";
      }
      
      // loop over the daughters of this node and look for the active volume
      int nd = vc->GetNdaughters();
      for(int i = 0; i < nd; ++i){
	if(strncmp(vc->GetNode(i)->GetName(), "volTPCActive", 12) == 0){
	  TGeoVolume *vca = vc->GetNode(i)->GetVolume();
	  if(vca) fActiveVolume = vca;
	}// end if the active part of the volume
      }// end loop over daughters of the volume
      
      if(!fActiveVolume) fActiveVolume = fTotalVolume;

    }// end if found total volume

    LOG_DEBUG("Geometry") << "detector total  volume is " << fTotalVolume->GetName()
			  << "\ndetector active volume is " << fActiveVolume->GetName();

    // build a matrix to take us from the local to the world coordinates
    // in one step
    fGeoMatrix = new TGeoHMatrix(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i){
      fGeoMatrix->Multiply(path[i]->GetMatrix());
    }
  
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
    double* rotMatrix = fGeoMatrix->GetRotationMatrix();
    if(rotMatrix[0] != 1){
      if(std::abs(rotMatrix[2]) == 1){
        fActiveHalfWidth = ((TGeoBBox*)fActiveVolume->GetShape())->GetDZ();
        fHalfWidth       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDZ();
      }
      if(std::abs(rotMatrix[1]) == 1){
        fActiveHalfWidth = ((TGeoBBox*)fActiveVolume->GetShape())->GetDY();
        fHalfWidth       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDY();
      }
    }
    if(rotMatrix[4] != 1){
      if(std::abs(rotMatrix[3]) == 1){
        fActiveHalfHeight = ((TGeoBBox*)fActiveVolume->GetShape())->GetDX();
        fHalfHeight       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDX();
      }
      if(std::abs(rotMatrix[5]) == 1){
        fActiveHalfHeight = ((TGeoBBox*)fActiveVolume->GetShape())->GetDZ();
        fHalfHeight       = ((TGeoBBox*)fTotalVolume->GetShape())->GetDZ();
      }
    }
    if(rotMatrix[8] != 1){
      if(std::abs(rotMatrix[6]) == 1){
        fActiveLength = 2.*((TGeoBBox*)fActiveVolume->GetShape())->GetDX();
        fLength       = 2.*((TGeoBBox*)fTotalVolume->GetShape())->GetDX();
      }
      if(std::abs(rotMatrix[7]) == 1){
        fActiveLength = 2.*((TGeoBBox*)fActiveVolume->GetShape())->GetDY();
        fLength       = 2.*((TGeoBBox*)fTotalVolume->GetShape())->GetDY();
      }
    }
    
    InitTPCBoundaries();
    
  } // TPCGeo::TPCGeo()

  //......................................................................
  TPCGeo::~TPCGeo()
  {
    for(unsigned int i = 0; i < fPlanes.size(); ++i)
      if(fPlanes[i]) delete fPlanes[i];
  
    fPlanes.clear();

    if(fGeoMatrix)    delete fGeoMatrix;
  }

  //......................................................................
  void TPCGeo::FindPlane(std::vector<const TGeoNode*>& path,
			 unsigned int depth) 
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
  void TPCGeo::MakePlane(std::vector<const TGeoNode*>& path, int depth) 
  {
    fPlanes.push_back(new PlaneGeo(path, depth));
  }


  //......................................................................
  // sort the PlaneGeo objects and the WireGeo objects inside 
  void TPCGeo::SortSubVolumes(geo::GeoObjectSorter const& sorter)
  {
    sorter.SortPlanes(fPlanes, fDriftDirection);

    double origin[3] = {0.};
 
    // Set view for planes in this TPC, assuming that plane sorting
    // increases in drift direction, to the convention that planes
    // 0,1,2 have views kU,kV,kZ respectively. kZ is collection.
    fPlanes[0]->SetView(geo::kU);
    fPlanes[1]->SetView(geo::kV);
    if (fPlanes.size() == 3) fPlanes[2]->SetView(geo::kZ);

    // set the plane pitch for this TPC
    double xyz[3]  = {0.};
    fPlanes[0]->LocalToWorld(origin,xyz);
    double xyz1[3] = {0.};
    fPlaneLocation.clear();
    fPlaneLocation.resize(fPlanes.size());
    for(unsigned int i = 0; i < fPlaneLocation.size(); ++i) fPlaneLocation[i].resize(3);
    fPlane0Pitch.clear();
    fPlane0Pitch.resize(this->Nplanes(), 0.);
    // the PlaneID_t cast convert InvalidID into a rvalue (non-reference);
    // leaving it a reference would cause C++ to treat it as such,
    // that can't be because InvalidID is a static member constant without an address
    // (it is not defined in any translation unit, just declared in header)
    fViewToPlaneNumber.resize
      (1U + (size_t) geo::kUnknown, (geo::PlaneID::PlaneID_t) geo::PlaneID::InvalidID);
    for(size_t p = 0; p < this->Nplanes(); ++p){
      fPlanes[p]->SetSignalType(geo::kInduction); //<set all planes to be induction for now
      fPlanes[p]->LocalToWorld(origin,xyz1);
      if(p > 0) fPlane0Pitch[p] = fPlane0Pitch[p-1] + std::abs(xyz1[0]-xyz[0]);
      else      fPlane0Pitch[p] = 0.;
      xyz[0] = xyz1[0];
      fPlaneLocation[p][0] = xyz1[0];
      fPlaneLocation[p][1] = xyz1[1];
      fPlaneLocation[p][2] = xyz1[2];

      fViewToPlaneNumber[(size_t) fPlanes[p]->View()] = p;
    }

    // now set the last plane in drift direction to be collection
    fPlanes[fPlanes.size()-1]->SetSignalType(geo::kCollection);

    for(size_t p = 0; p < fPlanes.size(); ++p) fPlanes[p]->SortWires(sorter);

    return;
  }


  //......................................................................
  void TPCGeo::ResetIDs(geo::TPCID tpcid) {
    
    fID = tpcid;
    for (unsigned int plane = 0; plane < Nplanes(); ++plane)
      fPlanes[plane]->ResetIDs(geo::PlaneID(fID, plane));
    
  } // TPCGeo::ResetIDs()
  
  
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
  void TPCGeo::LocalToWorld(const double* tpc, double* world) const
  {
    fGeoMatrix->LocalToMaster(tpc, world);
  }

  //......................................................................
  void TPCGeo::LocalToWorldVect(const double* tpc, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(tpc, world);
  }

  //......................................................................

  void TPCGeo::WorldToLocal(const double* world, double* tpc) const
  {
    fGeoMatrix->MasterToLocal(world, tpc);
  }

  //......................................................................

  TVector3 TPCGeo::WorldToLocal( const TVector3& world ) const
  {
    double worldArray[4];
    double localArray[4];
    worldArray[0] = world.X();
    worldArray[1] = world.Y();
    worldArray[2] = world.Z();
    worldArray[3] = 1.; 
    fGeoMatrix->MasterToLocal(worldArray,localArray);
    return TVector3(localArray);
  }

  //......................................................................

  TVector3 TPCGeo::LocalToWorld( const TVector3& local ) const
  {
    double worldArray[4];
    double localArray[4];
    localArray[0] = local.X();
    localArray[1] = local.Y();
    localArray[2] = local.Z();
    localArray[3] = 1.;
    fGeoMatrix->LocalToMaster(localArray,worldArray);
    return TVector3(worldArray);
  }

  //......................................................................

  // Convert a vector from world frame to the local plane frame
  // \param world : 3-D array. Vector in world coordinates; input.
  // \param plane : 3-D array. Vector in plane coordinates; plane.
  void TPCGeo::WorldToLocalVect(const double* world, double* plane) const
  {
    fGeoMatrix->MasterToLocalVect(world,plane);
  }

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

}
////////////////////////////////////////////////////////////////////////
