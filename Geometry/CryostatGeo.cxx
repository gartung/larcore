////////////////////////////////////////////////////////////////////////
/// \file CryostatGeo.cxx
///
/// \version $Id: CryostatGeo.cxx,v 1.12 2010/03/05 19:47:51 bpage Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <cmath>


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
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "Geometry/OpDetGeo.h"

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


  static bool LBNE_opdet_sort(const OpDetGeo* t1, const OpDetGeo* t2)
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
    if(fOpDets.size() != 600 ) std::sort(fOpDets.begin(), fOpDets.end(), opdet_sort);
    else std::sort(fOpDets.begin(), fOpDets.end(), LBNE_opdet_sort);
    return;
  }

  //......................................................................
  CryostatGeo::~CryostatGeo()
  {
    for(size_t i = 0; i < fTPCs.size(); ++i)
      if(fTPCs[i]) delete fTPCs[i];
  
    fTPCs.clear();
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
    for(size_t t = 0; t < fTPCs.size(); ++t){ 

      // determine the drift direction of the electrons in the TPC
      // and the drift distance.  The electrons always drift in the x direction
      // first get the location of the planes in the world coordinates
      double origin[3]     = {0.};
      double planeworld[3] = {0.};
      double tpcworld[3]   = {0.};

      fTPCs[t]->Plane(0).LocalToWorld(origin, planeworld);

      // now get the origin of the TPC in world coordinates
      fTPCs[t]->LocalToWorld(origin, tpcworld);
  
      // check to see if the x coordinates change between the tpc
      // origin and the plane origin, and if so in which direction
      if     ( tpcworld[0] > 1.01*planeworld[0] ) fTPCs[t]->SetDriftDirection(geo::kNegX);
      else if( tpcworld[0] < 0.99*planeworld[0] ) fTPCs[t]->SetDriftDirection(geo::kPosX);


      fTPCs[t]->SortSubVolumes(sorter);
    }

  }


  //......................................................................
  const TPCGeo& CryostatGeo::TPC(unsigned int itpc) const
  {
    if(itpc >= fTPCs.size()){
      throw cet::exception("TPCOutOfRange") << "Request for non-existant TPC " 
					    << itpc << "\n";
    }

    return *fTPCs[itpc];
  }



  //......................................................................
  void CryostatGeo::FindOpDet(std::vector<const TGeoNode*>& path,
			      unsigned int depth) 
  {

    const char* nm = path[depth]->GetName();
    if( (strncmp(nm, OpDetGeoName().c_str(), 6) == 0) ){
      this->MakeOpDet(path,depth);
std::cout<<" making opdet CryostatGeo 202 "<<path[depth]->GetName()<<std::endl;
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
std::cout<<" adding Optical detectors in CryostatGeo 225 container size "<<fOpDets.size()<<std::endl;
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
  const TPCGeo& CryostatGeo::PositionToTPC(double const  worldLoc[3],
					   unsigned int &tpc, 
					   double const &wiggle) const
  {
    // boundaries of the TPC in the world volume are organized as
    // [0] = -x
    // [1] = +x
    // [2] = -y
    // [3] = +y
    // [4] = -z
    // [5] = +z
    static std::vector<double> tpcBoundaries(this->NTPC()*6);

    bool firstCalculation = true;

    if ( firstCalculation ){
      firstCalculation = false;
      double origin[3] = {0.};
      double world[3] = {0.};
      for(unsigned int t = 0; t < this->NTPC(); ++t){
	this->TPC(t).LocalToWorld(origin, world);
	// y and z values are easy and can be figured out using the TPC origin
	// the x values are a bit trickier, at least the -x value seems to be
	tpcBoundaries[0+t*6] =  world[0] - this->TPC(t).HalfWidth();
	tpcBoundaries[1+t*6] =  world[0] + this->TPC(t).HalfWidth();
	tpcBoundaries[2+t*6] =  world[1] - this->TPC(t).HalfHeight();
	tpcBoundaries[3+t*6] =  world[1] + this->TPC(t).HalfHeight();
	tpcBoundaries[4+t*6] =  world[2] - 0.5*this->TPC(t).Length();
	tpcBoundaries[5+t*6] =  world[2] + 0.5*this->TPC(t).Length();
      }
    }// end if this is the first calculation

    // locate the desired TPC
    // allow the position to be a little off of the boundary
    // to account for rounding errors
    tpc = UINT_MAX;
    for(unsigned int t = 0; t < this->NTPC(); ++t){
      if(worldLoc[0] >= tpcBoundaries[0+t*6] * wiggle &&
	 worldLoc[0] <= tpcBoundaries[1+t*6] * wiggle && 
	 worldLoc[1] >= tpcBoundaries[2+t*6] * wiggle && 
	 worldLoc[1] <= tpcBoundaries[3+t*6] * wiggle && 
	 worldLoc[2] >= tpcBoundaries[4+t*6] * wiggle && 
	 worldLoc[2] <= tpcBoundaries[5+t*6] * wiggle ){
	tpc = t;
	break;
      }
    }

    if(tpc == UINT_MAX)
      throw cet::exception("Geometry") << "Can't find TPC for position (" 
				       << worldLoc[0] << ","
				       << worldLoc[1] << "," 
				       << worldLoc[2] << ")\n";
			
    return this->TPC(tpc);
  }

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

  const TVector3 CryostatGeo::WorldToLocal( const TVector3& world ) const
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

  const TVector3 CryostatGeo::LocalToWorld( const TVector3& local ) const
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

  unsigned int CryostatGeo::GetClosestOpDet(double * xyz) const
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
