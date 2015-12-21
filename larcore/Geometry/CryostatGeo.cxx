////////////////////////////////////////////////////////////////////////
/// \file CryostatGeo.cxx
///
/// \version $Id: CryostatGeo.cxx,v 1.12 2010/03/05 19:47:51 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cmath>
#include <limits> // std::numeric_limits<>
#include <algorithm> // std::for_each()
#include <memory> // std::default_delete<>

// ROOT includes
#include "TMath.h"
#include "TGeoManager.h"
#include "TGeoNode.h"
#include "TGeoMatrix.h"
#include <TGeoBBox.h>

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib/exception.h"

// LArSoft includes
#include "larcore/Geometry/CryostatGeo.h"
#include "larcore/Geometry/TPCGeo.h"
#include "larcore/Geometry/PlaneGeo.h"
#include "larcore/Geometry/WireGeo.h"
#include "larcore/Geometry/OpDetGeo.h"

namespace geo{


  //......................................................................
  // Define sort order for detector tpcs.
  static bool opdet_sort(const OpDetGeo* t1, const OpDetGeo* t2) 
  {
    double xyz1[3] = {0.}, xyz2[3] = {0.};
    double local[3] = {0.};
    t1->LocalToWorld(local, xyz1);
    t2->LocalToWorld(local, xyz2);

    if(xyz1[2]!=xyz2[2])
      return xyz1[2]>xyz2[2];
    else if(xyz1[1]!=xyz2[1])
      return xyz1[1]>xyz2[1];
    else
      return xyz1[0]>xyz2[0];
  }

  // DUNE specific sorting originally intended for 10kt
  // executed when there are 600+ opdets. -talion
  ///\todo: move dune opdet sorting to appropriate place in dunetpc 
  static bool DUNE_opdet_sort(const OpDetGeo* t1, const OpDetGeo* t2)
  {
    double xyz1[3] = {0.}, xyz2[3] = {0.};
    double local[3] = {0.};
    t1->LocalToWorld(local, xyz1);
    t2->LocalToWorld(local, xyz2);

    if(xyz1[0]!=xyz2[0])
      return xyz1[0]>xyz2[0];
    else if(xyz1[2]!=xyz2[2])
      return xyz1[2]>xyz2[2];
    else
    return xyz1[1]>xyz2[1];
  }

  //......................................................................
  CryostatGeo::CryostatGeo(std::vector<const TGeoNode*>& path, int depth)
    : fVolume(0)
  {
    
    // all planes are going to be contained in the volume named volCryostat
    // now get the total volume of the Cryostat
    TGeoVolume *vc = path[depth]->GetVolume();
    if(vc){
      fVolume = vc;
      if(!vc)
	throw cet::exception("CryostatGeo") << "cannot find cryostat outline volume\n";
      
    }// end if found volume

    LOG_DEBUG("Geometry") << "cryostat  volume is " << fVolume->GetName();

    // build a matrix to take us from the local to the world coordinates
    // in one step
    fGeoMatrix = new TGeoHMatrix(*path[0]->GetMatrix());
    for(int i = 1; i <= depth; ++i){
      fGeoMatrix->Multiply(path[i]->GetMatrix());
    }
  
    // find the tpcs for the cryostat so that you can use them later
    this->FindTPC(path, depth);


    // Set OpDetName;
    fOpDetGeoName = "volOpDetSensitive";
    
    // find the opdets for the cryostat so that you can use them later
    this->FindOpDet(path, depth);
    
    // sort the OpDets according to xyz position
    // 600 intended to separate dune10kt geometry from others when sorting
    ///\todo: remove the hard-coded 600 in favor of selecting sorting the same way as in ChannelMapAlgs
    if(fOpDets.size() != 600 ) std::sort(fOpDets.begin(), fOpDets.end(), opdet_sort);
    else std::sort(fOpDets.begin(), fOpDets.end(), DUNE_opdet_sort);
    return;
  }

  //......................................................................
  CryostatGeo::~CryostatGeo()
  {
    for(size_t i = 0; i < fTPCs.size(); ++i)
      if(fTPCs[i]) delete fTPCs[i];
    fTPCs.clear();
    
    std::for_each
      (fOpDets.begin(), fOpDets.end(), std::default_delete<OpDetGeo>());
    fOpDets.clear();

    if(fGeoMatrix)    delete fGeoMatrix;
  }


  //......................................................................
  void CryostatGeo::FindTPC(std::vector<const TGeoNode*>& path,
			    unsigned int depth) 
  {

    const char* nm = path[depth]->GetName();
    if( (strncmp(nm, "volTPC", 6) == 0) ){
      this->MakeTPC(path,depth);
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
      this->FindTPC(path, deeper);
    }
  
  }

  //......................................................................
  void CryostatGeo::MakeTPC(std::vector<const TGeoNode*>& path, int depth) 
  {
    fTPCs.push_back(new TPCGeo(path, depth));
  }

  //......................................................................
  // sort the TPCGeo objects, and the PlaneGeo objects inside
  void CryostatGeo::SortSubVolumes(geo::GeoObjectSorter const& sorter)
  {
    sorter.SortTPCs(fTPCs);
    for(size_t t = 0; t < fTPCs.size(); ++t) { 
      TPCGeo* TPC = fTPCs[t];

      // determine the drift direction of the electrons in the TPC
      // and the drift distance.  The electrons always drift in the x direction
      // first get the location of the planes in the world coordinates
      double origin[3]     = { 0., 0., 0. };
      double planeworld[3] = { 0., 0., 0. };
      double tpcworld[3]   = { 0., 0., 0. };

      TPC->Plane(0).LocalToWorld(origin, planeworld);

      // now get the origin of the TPC in world coordinates
      TPC->LocalToWorld(origin, tpcworld);
  
      // check to see if the x coordinates change between the tpc
      // origin and the plane origin, and if so in which direction
      if     ( tpcworld[0] > 1.01*planeworld[0] ) TPC->SetDriftDirection(geo::kNegX);
      else if( tpcworld[0] < 0.99*planeworld[0] ) TPC->SetDriftDirection(geo::kPosX);
      else {
        throw cet::exception("CryostatGeo")
          << "Can't determine drift direction of TPC #" << t << " at ("
          << tpcworld[0] << "; " << tpcworld[1] << "; " << tpcworld[2]
          << " to planes (plane 0 at "
          << planeworld[0] << "; " << planeworld[1] << "; " << planeworld[2]
          << ")\n";
      }

      TPC->SortSubVolumes(sorter);
    }

  }


  //......................................................................
  const TPCGeo& CryostatGeo::TPC(unsigned int itpc) const
  {
    TPCGeo const* pTPC = TPCPtr(itpc);
    if(!pTPC){
      throw cet::exception("TPCOutOfRange") << "Request for non-existant TPC "
                                            << itpc << "\n";
    }

    return *pTPC;
  }



  //......................................................................
  void CryostatGeo::FindOpDet(std::vector<const TGeoNode*>& path,
			      unsigned int depth) 
  {

    const char* nm = path[depth]->GetName();
    if( (strncmp(nm, OpDetGeoName().c_str(), 6) == 0) ){
      this->MakeOpDet(path,depth);
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
      this->FindOpDet(path, deeper);
    }
  
  }

  //......................................................................
  void CryostatGeo::MakeOpDet(std::vector<const TGeoNode*>& path, int depth) 
  {
    fOpDets.push_back(new OpDetGeo(path, depth));
  }

  //......................................................................
  const OpDetGeo& CryostatGeo::OpDet(unsigned int iopdet) const
  {
    if(iopdet >= fOpDets.size()){
      throw cet::exception("OpDetOutOfRange") << "Request for non-existant OpDet " 
					      << iopdet;
    }

    return *fOpDets[iopdet];
  }
  
  
  
  //......................................................................
  // wiggle is 1+a small number to allow for rounding errors on the 
  // passed in world loc relative to the boundaries.
  unsigned int CryostatGeo::FindTPCAtPosition(double const worldLoc[3], 
                                              double const wiggle) const
  {
    const unsigned int nTPC = NTPC();
    for(unsigned int t = 0; t < nTPC; ++t){
      geo::TPCGeo const& tpc = TPC(t);
      if (tpc.ContainsPosition(worldLoc, wiggle)) return t;
    }
    return std::numeric_limits<unsigned int>::max();
  } // CryostatGeo::FindTPCAtPosition()

  //......................................................................
  // wiggle is 1+a small number to allow for rounding errors on the 
  // passed in world loc relative to the boundaries.
  const TPCGeo& CryostatGeo::PositionToTPC(double const  worldLoc[3],
					   unsigned int &tpc, 
					   double const &wiggle) const
  {
    tpc = FindTPCAtPosition(worldLoc, wiggle);
    if(tpc == std::numeric_limits<unsigned int>::max())
      throw cet::exception("Geometry") << "Can't find TPC for position (" 
				       << worldLoc[0] << ","
				       << worldLoc[1] << "," 
				       << worldLoc[2] << ")\n";
			
    return TPC(tpc);
  }

  //......................................................................
  unsigned int CryostatGeo::MaxPlanes() const {
    unsigned int maxPlanes = 0;
    for (geo::TPCGeo const* pTPC: fTPCs) {
      if (!pTPC) continue;
      unsigned int maxPlanesInTPC = pTPC->Nplanes();
      if (maxPlanesInTPC > maxPlanes) maxPlanes = maxPlanesInTPC;
    } // for
    return maxPlanes;
  } // CryostatGeo::MaxPlanes()
  
  //......................................................................
  unsigned int CryostatGeo::MaxWires() const {
    unsigned int maxWires = 0;
    for (geo::TPCGeo const* pTPC: fTPCs) {
      if (!pTPC) continue;
      unsigned int maxWiresInTPC = pTPC->MaxWires();
      if (maxWiresInTPC > maxWires) maxWires = maxWiresInTPC;
    } // for
    return maxWires;
  } // CryostatGeo::MaxWires()
  
  //......................................................................
  double CryostatGeo::HalfWidth()  const 
  {
    return ((TGeoBBox*)fVolume->GetShape())->GetDX();
  }

  //......................................................................
  double CryostatGeo::HalfHeight() const 
  {
    return ((TGeoBBox*)fVolume->GetShape())->GetDY();
  }

  //......................................................................
  double CryostatGeo::Length() const
  { 
    return 2.0*((TGeoBBox*)fVolume->GetShape())->GetDZ();
  }

  //......................................................................
  void CryostatGeo::LocalToWorld(const double* tpc, double* world) const
  {
    fGeoMatrix->LocalToMaster(tpc, world);
  }

  //......................................................................
  void CryostatGeo::LocalToWorldVect(const double* tpc, double* world) const
  {
    fGeoMatrix->LocalToMasterVect(tpc, world);
  }

  //......................................................................

  void CryostatGeo::WorldToLocal(const double* world, double* tpc) const
  {
    fGeoMatrix->MasterToLocal(world, tpc);
  }

  //......................................................................

  TVector3 CryostatGeo::WorldToLocal( const TVector3& world ) const
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

  TVector3 CryostatGeo::LocalToWorld( const TVector3& local ) const
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
  void CryostatGeo::WorldToLocalVect(const double* world, double* plane) const
  {
    fGeoMatrix->MasterToLocalVect(world,plane);
  }



  //......................................................................
  // Find the nearest opdet to point in this cryostat

  unsigned int CryostatGeo::GetClosestOpDet(double const* xyz) const
  {
    int    ClosestDet=-1;
    float  ClosestDist=UINT_MAX;

    for(size_t o=0; o!=NOpDet(); o++)
      {
	float ThisDist = OpDet(0).DistanceToPoint(xyz); 
	if(ThisDist < ClosestDist)
	  {
	    ClosestDist = ThisDist;
	    ClosestDet  = o;
	  }
      }
    return ClosestDet;
    
  }

}
////////////////////////////////////////////////////////////////////////
