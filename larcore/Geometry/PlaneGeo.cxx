////////////////////////////////////////////////////////////////////////
/// \file PlaneGeo.cxx
///
/// \version $Id: PlaneGeo.cxx,v 1.12 2010/03/05 19:47:51 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <cmath>


// ROOT includes
#include "TMath.h"
#include "TVector3.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// LArSoft includes
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"

namespace geo{


  //......................................................................
  PlaneGeo::PlaneGeo(std::vector<const TGeoNode*>& path, int depth)
  {
    // build a matrix to take us from the local to the world coordinates
    // in one step
    fGeoMatrix = new TGeoHMatrix(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i){
      fGeoMatrix->Multiply(path[i]->GetMatrix());
    }
  
    // find the wires for the plane so that you can use them later
    this->FindWire(path, depth);

    // view and signal are now set at TPC level with SetView and SetSignal
    fOrientation = kVertical;

    // perform initialization that is mapping-dependent;
    // when the mapping changes, this will have to be repeated
    // assumes same pitch between all wires in this plane
    UpdateFromMapping();
  }

  //......................................................................

  PlaneGeo::~PlaneGeo()
  {
    for(unsigned int i = 0; i < fWire.size(); ++i)
      if(fWire[i]) delete fWire[i];
  
    fWire.clear();

    if(fGeoMatrix) delete fGeoMatrix;

  }

  //......................................................................
  
  geo::WireGeo const& PlaneGeo::Wire(unsigned int iwire) const {
    geo::WireGeo const* pWire = WirePtr(iwire);
    if (!pWire) {
      throw cet::exception("WireOutOfRange")
        << "Request for non-existant wire " << iwire << "\n";
    }
    return *pWire;
  } // PlaneGeo::Wire(int)
  
  //......................................................................

  void PlaneGeo::FindWire(std::vector<const TGeoNode*>& path,
			  unsigned int depth) 
  {
    // Check if the current node is a wire
    const char* wire = "volTPCWire";
    if(strncmp(path[depth]->GetName(), wire, strlen(wire)) == 0){
      this->MakeWire(path, depth);
      return;
    }
  
    // Explore the next layer down
    unsigned int deeper = depth+1;
    if (deeper>=path.size()) {
      throw cet::exception("ExceededMaxDepth") << "Exceeded maximum depth\n";
    }
    const TGeoVolume* v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for (int i=0; i<nd; ++i) {
      path[deeper] = v->GetNode(i);
      this->FindWire(path, deeper);
    }
  }

  //......................................................................

  void PlaneGeo::MakeWire(std::vector<const TGeoNode*>& path, int depth) 
  {
    fWire.push_back(new WireGeo(path, depth));
  }

  //......................................................................

  // sort the WireGeo objects
  void PlaneGeo::SortWires(geo::GeoObjectSorter const& sorter )
  {
    sorter.SortWires(fWire);
    UpdateFromMapping();
  }
  
  
  //......................................................................
  TVector3 PlaneGeo::GetNormalDirection() const {
    const unsigned int NWires = Nwires();
    if (NWires < 2) return TVector3(); // why are we even here?
    
    // 1) get the direction of the middle wire
    TVector3 WireDir = Wire(NWires / 2).Direction();
    
    // 2) get the direction between the middle wire and the next one
    double MiddleWireCenter[3], NextToMiddleWireCenter[3];
    Wire(NWires / 2).GetCenter(MiddleWireCenter);
    Wire(NWires / 2 + 1).GetCenter(NextToMiddleWireCenter);
    TVector3 ToNextWire(NextToMiddleWireCenter);
    ToNextWire -= TVector3(MiddleWireCenter);
    
    // 3) get the direction perpendicular to the plane
    TVector3 PlaneNorm = WireDir.Cross(ToNextWire);
    
    // 4) round it
    for (int i = 0; i < 3; ++i)
      if (std::abs(PlaneNorm[i]) < 1e-4) PlaneNorm[i] = 0.;
    
    // 5) return its norm
    return PlaneNorm.Unit();
  } // GeometryTest::GetNormalDirection()
  
  
  bool PlaneGeo::WireIDincreasesWithZ() const {
    // If the first wire is shorter than the next one (they are in a corner),
    // the ordering of z exactly matches the order of wire IDs:
    // if the second wire has larger z than the first one, it has larger
    // intercept with the z axis, and we want to see if it has also larger
    // wire number.
    // In the middle of a plane narrow in z, wires may have the same z,
    // spoiling this shortcut.
    double FirstWireCenter[3], SecondWireCenter[3];
    Wire(0).GetCenter(FirstWireCenter);
    Wire(1).GetCenter(SecondWireCenter);
    
    return SecondWireCenter[2] > FirstWireCenter[2];
  } // PlaneGeo::WireIDincreasesWithZ()
  
  
  //......................................................................
  TVector3 PlaneGeo::GetIncreasingWireDirection() const {
    const unsigned int NWires = Nwires();
    if (NWires < 2) {
      // this likely means construction is not complete yet
      throw cet::exception("NoWireInPlane")
        << "GetIncreasingWireDirection() has only " << NWires << " wires.\n";
    } // if
    
    // 1) get the direction of the middle wire
    TVector3 WireDir = Wire(NWires / 2).Direction();
    
    // 2) get the direction between the middle wire and the next one
    double MiddleWireCenter[3], NextToMiddleWireCenter[3];
    Wire(NWires / 2).GetCenter(MiddleWireCenter);
    Wire(NWires / 2 + 1).GetCenter(NextToMiddleWireCenter);
    TVector3 ToNextWire(NextToMiddleWireCenter);
    ToNextWire -= TVector3(MiddleWireCenter);
    
    // 3) get the direction perpendicular to the plane
    TVector3 PlaneNorm = WireDir.Cross(ToNextWire);
    PlaneNorm = PlaneNorm.Unit();
    
    // 4) finally, get the direction perpendicular to the wire,
    //    lying on the plane, toward increasing wire numbers
    TVector3 IncreasingWireDir = PlaneNorm.Cross(WireDir);
    
    // 5) round it
    for (int i = 0; i < 3; ++i)
      if (std::abs(IncreasingWireDir[i]) < 1e-4) IncreasingWireDir[i] = 0.;
    
    return IncreasingWireDir;
  } // GeometryTest::GetIncreasingWireDirection()
  
  
  //......................................................................
  double PlaneGeo::ThetaZ() const { return FirstWire().ThetaZ(); }
  
  
  //......................................................................

  void PlaneGeo::LocalToWorld(const double* plane, double* world) const
  {
    fGeoMatrix->LocalToMaster(plane, world);
  }

  //......................................................................

  void PlaneGeo::LocalToWorldVect(const double* plane, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(plane, world);
  }

  //......................................................................

  void PlaneGeo::WorldToLocal(const double* world, double* plane) const
  {
    fGeoMatrix->MasterToLocal(world, plane);
  }

  //......................................................................

  const TVector3 PlaneGeo::WorldToLocal( const TVector3& world ) const
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

  const TVector3 PlaneGeo::LocalToWorld( const TVector3& local ) const
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
  void PlaneGeo::WorldToLocalVect(const double* world, double* plane) const
  {
    fGeoMatrix->MasterToLocalVect(world,plane);
  }

  //......................................................................
  void PlaneGeo::UpdateWirePitch() {
    fWirePitch = geo::WireGeo::WirePitch(Wire(0), Wire(1));
  } // PlaneGeo::UpdateWirePitch()
  
  //......................................................................
  void PlaneGeo::UpdatePhiZ() {
    TVector3 wire_coord_dir = GetIncreasingWireDirection();
    fCosPhiZ = wire_coord_dir.Z();
    fSinPhiZ = wire_coord_dir.Y();
  } // PlaneGeo::UpdatePhiZ()

  void PlaneGeo::UpdateView() {

    auto cos_thetaz = std::cos(ThetaZ());
    
    if( std::abs(cos_thetaz - 1.0) < std::numeric_limits<double>::epsilon() ||
	std::abs(cos_thetaz + 1.0) < std::numeric_limits<double>::epsilon()) 
      SetView(geo::View_t::kY);
    else if( std::abs(cos_thetaz - 0.0) < std::numeric_limits<double>::epsilon())
      SetView(geo::View_t::kZ);
    else if( cos_thetaz < 0.0 )
      SetView(geo::View_t::kV);
    else if( cos_thetaz > 0.0 )
      SetView(geo::View_t::kU);
    else
      SetView(geo::View_t::kUnknown);      

  }
  
  //......................................................................
  void PlaneGeo::UpdateFromMapping() {
    UpdateWirePitch();
    UpdatePhiZ();
    UpdateView();
  } // PlaneGeo::UpdateWirePitch()
  
  //......................................................................
  void PlaneGeo::ResetIDs(geo::PlaneID planeid) {
    
    fID = planeid;
    for (unsigned int wire = 0; wire < Nwires(); ++wire)
      fWire[wire]->ResetID(geo::WireID(fID, wire));
    
  } // PlaneGeo::ResetIDs()
  
  //......................................................................
  
  
}
////////////////////////////////////////////////////////////////////////
