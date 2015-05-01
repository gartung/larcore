/**
 * @file   GeometryCore.cxx
 * @brief  Access the description of detector geometry - implementation file
 * @author brebel@fnal.gov
 * @see    GeometryCore.h
 *
 */

// class header
#include "Geometry/GeometryCore.h"

// lar includes
#include "SimpleTypesAndConstants/PhysicalConstants.h" // util::pi<>
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "Geometry/OpDetGeo.h"
#include "Geometry/AuxDetGeo.h"
#include "Geometry/AuxDetSensitiveGeo.h"

// Framework includes
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// ROOT includes
#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TGeoMatrix.h>
#include <TGeoBBox.h>
// #include <Rtypes.h>

// C/C++ includes
#include <cstddef> // size_t
#include <cctype> // ::tolower()
#include <cmath> // std::abs() ...
#include <vector>
#include <algorithm> // std::for_each(), std::transform()
#include <utility> // std::swap()
#include <limits> // std::numeric_limits<>
#include <memory> // std::default_deleter<>


namespace geo {
  
  template <typename T>
  inline T sqr(T v) { return v * v; }
  
  
  //......................................................................
  // Constructor.
  GeometryCore::GeometryCore(
    fhicl::ParameterSet const& pset
    )
    : fSurfaceY         (pset.get< double            >("SurfaceY"               ))
    , fDetectorName     (pset.get< std::string       >("Name"                   ))
    , fMinWireZDist     (pset.get< double            >("MinWireZDist",     3.0  ))
    , fPositionWiggle   (pset.get< double            >("PositionEpsilon",  1.e-4))
  {
    std::transform(fDetectorName.begin(), fDetectorName.end(),
      fDetectorName.begin(), ::tolower);
  } // GeometryCore::GeometryCore()
  
  
  //......................................................................
  GeometryCore::~GeometryCore() {
    ClearGeometry();
  } // GeometryCore::~GeometryCore()


  //......................................................................
  void GeometryCore::ApplyChannelMap
    (std::shared_ptr<geo::ChannelMapAlg> pChannelMap)
  {
    pChannelMap->Initialize(fGeoData);
    fChannelMapAlg = pChannelMap;
  } // Geometry::ApplyChannelMap()

  //......................................................................
  void GeometryCore::LoadGeometryFile
    (std::string gdmlfile, std::string rootfile)
  {
    
    if (gdmlfile.empty()) {
      throw cet::exception("GeometryCore")
        << "No GDML Geometry file specified!\n";
    }
    
    if (rootfile.empty()) {
      throw cet::exception("GeometryCore")
        << "No ROOT Geometry file specified!\n";
    }
    
    ClearGeometry();

    // Open the GDML file, and convert it into ROOT TGeoManager format.
    TGeoManager::Import(rootfile.c_str());

    std::vector<const TGeoNode*> path(8);
    path[0] = gGeoManager->GetTopNode();
    FindCryostat(path, 0);
    FindAuxDet(path, 0);
    
    fGDMLfile = gdmlfile;
    fROOTfile = rootfile;
    
    mf::LogInfo("GeometryCore") << "New detector geometry loaded from "
                                << "\n\t" << fROOTfile 
                                << "\n\t" << fGDMLfile << "\n";
    
  } // GeometryCore::LoadGeometryFile()

  //......................................................................
  void GeometryCore::ClearGeometry() {
    
    // cryostats
    std::for_each(Cryostats().begin(), Cryostats().end(),
      std::default_delete<CryostatGeo>());
    Cryostats().clear();
    
    // auxiliary detectors
    std::for_each(AuxDets().begin(), AuxDets().end(),
      std::default_delete<AuxDetGeo>());
    AuxDets().clear();
    
  } // GeometryCore::ClearGeometry()


  //......................................................................
  TGeoManager* GeometryCore::ROOTGeoManager() const
  {
    return gGeoManager;
  }
  
  //......................................................................
  unsigned int GeometryCore::Nchannels() const
  {
    return fChannelMapAlg->Nchannels();
  }

  //......................................................................
  unsigned int GeometryCore::NTPC(unsigned int cstat) const
  {
    return this->Cryostat(cstat).NTPC();
  }

  //......................................................................
  unsigned int GeometryCore::NOpDets() const
  {
    int N=0;
    for(size_t cstat=0; cstat!=Ncryostats(); cstat++)
      N += this->Cryostat(cstat).NOpDet();
    return N;
  }

  //......................................................................
  unsigned int GeometryCore::NOpChannels() const
  {
    return fChannelMapAlg->NOpChannels(this->NOpDets());
  }

  //......................................................................
  unsigned int GeometryCore::NOpHardwareChannels(int opDet) const
  {
    return fChannelMapAlg->NOpHardwareChannels(opDet);
  }

  //......................................................................
  unsigned int GeometryCore::OpChannel(int detNum, int hardwareChannel) const
  {
    return fChannelMapAlg->OpChannel(detNum, hardwareChannel);
  }

  //......................................................................
  unsigned int GeometryCore::OpDetFromOpChannel(int opChannel) const
  {
    return fChannelMapAlg->OpDetFromOpChannel(opChannel);
  }

  //......................................................................
  unsigned int GeometryCore::HardwareChannelFromOpChannel(int opChannel) const
  {
    return fChannelMapAlg->HardwareChannelFromOpChannel(opChannel);
  }

  //......................................................................
  // Is this a valid OpChannel number?
  bool GeometryCore::IsValidOpChannel(int opChannel) const
  {
    return fChannelMapAlg->IsValidOpChannel(opChannel, this->NOpDets());
  }

  
  //......................................................................
  unsigned int GeometryCore::Nplanes(unsigned int tpc,
                                 unsigned int cstat) const
  {
    return this->Cryostat(cstat).TPC(tpc).Nplanes();
  }

  //......................................................................
  unsigned int GeometryCore::Nwires(unsigned int p, 
                                unsigned int tpc,
                                unsigned int cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc).Plane(p).Nwires();
  }

  //......................................................................
  unsigned int Geometry::NAuxDetSensitive(size_t const& aid) const
  {
    if( aid > fAuxDets.size() - 1)
      throw cet::exception("Geometry") << "Requested AuxDet index " << aid 
				       << " is out of range: " << fAuxDets.size();

    return fAuxDets[aid]->NSensitiveVolume();
  }

  //......................................................................
  // Number of different views, or wire orientations
  // The function assumes that all TPCs in all cryostats of
  // a detector have the same number of planes, which should be 
  // a safe assumption
  unsigned int GeometryCore::Nviews() const
  {
    return this->Cryostat(0).TPC(0).Nplanes();
  }

  //......................................................................
  //
  // Return the geometry description of the ith plane in the detector.
  //
  // \param cstat : input cryostat number, starting from 0
  // \returns cryostat geometry for ith cryostat
  //
  // \throws geo::Exception if "cstat" is outside allowed range
  //
  const CryostatGeo& GeometryCore::Cryostat(unsigned int const cstat) const 
  {
    if(cstat >= Ncryostats()) 
      throw cet::exception("GeometryCore") << "Cryostat " 
                                           << cstat 
                                           << " does not exist\n";

    return *(Cryostats()[cstat]);
  }

  //......................................................................
  //
  // Return the geometry description of the ith AuxDet.
  //
  // \param ad : input AuxDet number, starting from 0
  // \returns AuxDet geometry for ith AuxDet
  //
  // \throws geo::Exception if "ad" is outside allowed range
  //
  const AuxDetGeo& GeometryCore::AuxDet(unsigned int const ad) const
  {
    if(ad >= NAuxDets())
    throw cet::exception("GeometryCore") << "AuxDet "
    << ad
    << " does not exist\n";
    
    return *(AuxDets()[ad]);
  }
  

  
  //......................................................................
  //
  // Return the geometry description of the ith plane in the detector.
  //
  // \param tpc : input plane number, starting from 0
  // \returns plane geometry for ith plane
  //
  // \throws geo::Exception if "tpc" is outside allowed range
  //
  const TPCGeo& GeometryCore::TPC(unsigned int const tpc,
                              unsigned int const cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc);
  }

  
  //......................................................................
  geo::TPCID GeometryCore::FindTPCAtPosition(double const worldLoc[3]) const {
    geo::TPCID tpcid; // invalid by default
    
    // first find the cryostat
    tpcid.Cryostat = FindCryostatAtPosition(worldLoc);
    if (tpcid.Cryostat == std::numeric_limits<unsigned int>::max()) return tpcid;
    
    // then ask it about the TPC
    tpcid.TPC = Cryostat(tpcid.Cryostat).FindTPCAtPosition(worldLoc, 1. + fPositionWiggle);
    if (tpcid.TPC == UINT_MAX) return tpcid;
    
    // finally declare the result valid and return it
    tpcid.isValid = true;
    return tpcid;
  } // GeometryCore::FindTPCAtPosition()
  
  
  //......................................................................
  const TPCGeo& GeometryCore::PositionToTPC(double const  worldLoc[3],
                                        unsigned int &tpc,
                                        unsigned int &cstat) const
  {
    return this->PositionToCryostat(worldLoc,cstat).PositionToTPC(worldLoc,tpc, 1.+fPositionWiggle);
  }

  //......................................................................
  unsigned int GeometryCore::FindCryostatAtPosition(double const worldLoc[3]) const
  {
    // boundaries of the TPC in the world volume are organized as
    // [0] = -x
    // [1] = +x
    // [2] = -y
    // [3] = +y
    // [4] = -z
    // [5] = +z
    static std::vector<double> cstatBoundaries(this->Ncryostats()*6);

    static bool firstCalculation = true;

    if ( firstCalculation ){
      firstCalculation = false;
      double origin[3] = {0.};
      double world[3] = {0.};
      for(unsigned int c = 0; c < this->Ncryostats(); ++c){
        this->Cryostat(c).LocalToWorld(origin, world);
        // y and z values are easy and can be figured out using the TPC origin
        // the x values are a bit trickier, at least the -x value seems to be
        cstatBoundaries[0+c*6] =  world[0] - this->Cryostat(c).HalfWidth();
        cstatBoundaries[1+c*6] =  world[0] + this->Cryostat(c).HalfWidth();
        cstatBoundaries[2+c*6] =  world[1] - this->Cryostat(c).HalfHeight();
        cstatBoundaries[3+c*6] =  world[1] + this->Cryostat(c).HalfHeight();
        cstatBoundaries[4+c*6] =  world[2] - 0.5*this->Cryostat(c).Length();
        cstatBoundaries[5+c*6] =  world[2] + 0.5*this->Cryostat(c).Length();
      }
    }// end if this is the first calculation

    // locate the desired Cryostat
    for(unsigned int c = 0; c < this->Ncryostats(); ++c){
      if(worldLoc[0] >= cstatBoundaries[0+c*6] * (1. + fPositionWiggle) &&
         worldLoc[0] <= cstatBoundaries[1+c*6] * (1. + fPositionWiggle) && 
         worldLoc[1] >= cstatBoundaries[2+c*6] * (1. + fPositionWiggle) && 
         worldLoc[1] <= cstatBoundaries[3+c*6] * (1. + fPositionWiggle) && 
         worldLoc[2] >= cstatBoundaries[4+c*6] * (1. + fPositionWiggle) && 
         worldLoc[2] <= cstatBoundaries[5+c*6] * (1. + fPositionWiggle) ){
        return c;
      }
    }
    return UINT_MAX;
  } // GeometryCore::FindCryostatAtPosition()

  //......................................................................
  const CryostatGeo& GeometryCore::PositionToCryostat(double const  worldLoc[3],
                                                  unsigned int &cstat) const
  {
    cstat = FindCryostatAtPosition(worldLoc);
    
    if(cstat == UINT_MAX)
      throw cet::exception("GeometryCore") << "Can't find Cryostat for position (" 
                                       << worldLoc[0] << ","
                                       << worldLoc[1] << "," 
                                       << worldLoc[2] << ")\n";
                        
    return this->Cryostat(cstat);
  }
    
  
  //......................................................................
  unsigned int GeometryCore::FindAuxDetAtPosition(double const  worldPos[3]) const
  {
    return fChannelMapAlg->NearestAuxDet(worldPos, fAuxDets);
  } // GeometryCore::FindAuxDetAtPosition()
  

  
  //......................................................................
  const AuxDetGeo& GeometryCore::PositionToAuxDet(double const  worldLoc[3],
                                              unsigned int &ad) const
  {    
    // locate the desired Auxiliary Detector
    ad = this->FindAuxDetAtPosition(worldLoc);
    
    return this->AuxDet(ad);
  }

  //......................................................................
  void Geometry::FindAuxDetSensitiveAtPosition(double const worldPos[3],
					       size_t     & adg,
					       size_t     & sv) const
  {
    adg = this->FindAuxDetAtPosition(worldPos);
    sv  = fChannelMapAlg->NearestSensitiveAuxDet(worldPos, fAuxDets);

    return;
  } // Geometry::FindAuxDetAtPosition()
  

  
  //......................................................................
  const AuxDetSensitiveGeo& Geometry::PositionToAuxDetSensitive(double const worldLoc[3],
								size_t      &ad,
								size_t      &sv) const
  {    
    // locate the desired Auxiliary Detector
    this->FindAuxDetSensitiveAtPosition(worldLoc, ad, sv);    
    return this->AuxDet(ad).SensitiveVolume(sv);
  }
  
  //......................................................................
  //
  // Return the geometry description of the ith plane in the detector.
  //
  // \param p : input plane number, starting from 0
  // \returns plane geometry for ith plane
  //
  // \throws geo::Exception if "i" is outside allowed range
  //
  const PlaneGeo& GeometryCore::Plane(unsigned int const p, 
                                  unsigned int const tpc,
                                  unsigned int const cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc).Plane(p);
  }

  //......................................................................
  SigType_t GeometryCore::SignalType(raw::ChannelID_t const channel) const
  {
    return fChannelMapAlg->SignalType(channel);
  }

  //......................................................................
  SigType_t GeometryCore::SignalType(geo::PlaneID const pid) const
  {
    return this->Cryostat(pid.Cryostat).TPC(pid.TPC).Plane(pid.Plane).SignalType();
  }


  //......................................................................
  View_t GeometryCore::View(raw::ChannelID_t const channel) const
  {
    return fChannelMapAlg->View(channel);
  }

  //......................................................................
  View_t GeometryCore::View(geo::PlaneID const pid) const
  {
    return this->Cryostat(pid.Cryostat).TPC(pid.TPC).Plane(pid.Plane).View();
  }

  //......................................................................
  std::set<View_t> const& GeometryCore::Views() const
  {
    return fChannelMapAlg->Views();
  }

  //......................................................................
  std::set<PlaneID> const& GeometryCore::PlaneIDs() const
  {
    return fChannelMapAlg->PlaneIDs();
  }

  //......................................................................
  const std::string GeometryCore::GetWorldVolumeName() const
  {
    // For now, and possibly forever, this is a constant (given the
    // definition of "nodeNames" above).
    return std::string("volWorld");
  }

  //......................................................................
  const std::string GeometryCore::GetLArTPCVolumeName(unsigned int const tpc,
                                                  unsigned int const cstat) const
  {

    return std::string(this->Cryostat(cstat).TPC(tpc).ActiveVolume()->GetName()); 
  }

  //......................................................................
  const std::string GeometryCore::GetCryostatVolumeName(unsigned int const cstat) const
  {
    return this->Cryostat(cstat).Volume()->GetName();
  }

  //......................................................................
  double GeometryCore::DetHalfWidth(unsigned int tpc,
                                unsigned int cstat)  const 
  {
    return this->Cryostat(cstat).TPC(tpc).ActiveHalfWidth();
  }

  //......................................................................
  double GeometryCore::DetHalfHeight(unsigned int tpc,
                                 unsigned int cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc).ActiveHalfHeight();
  }

  //......................................................................
  double GeometryCore::DetLength(unsigned int tpc,
                             unsigned int cstat) const
  { 
    return this->Cryostat(cstat).TPC(tpc).ActiveLength();
  }

  //......................................................................
  double GeometryCore::CryostatHalfWidth(unsigned int cstat) const
  {
    return this->Cryostat(cstat).HalfWidth();
  }

  //......................................................................
  double GeometryCore::CryostatHalfHeight(unsigned int cstat) const
  {
    return this->Cryostat(cstat).HalfHeight();
  }

  //......................................................................
  double GeometryCore::CryostatLength(unsigned int cstat) const
  {
    return this->Cryostat(cstat).Length();
  }

  //......................................................................
  // Boundaries of the cryostat in 3 pairs
  // [0]: -x
  // [1]: +x
  // [2]: -y
  // [3]: +y
  // [4]: -z
  // [5]: +z
  void GeometryCore::CryostatBoundaries(double* boundaries,
                                    unsigned int cstat) const
  {
    // get the half width, height, etc of the cryostat
    double halflength = ((TGeoBBox*)this->Cryostat(cstat).Volume()->GetShape())->GetDZ();
    double halfwidth  = ((TGeoBBox*)this->Cryostat(cstat).Volume()->GetShape())->GetDX();
    double halfheight = ((TGeoBBox*)this->Cryostat(cstat).Volume()->GetShape())->GetDY();
    
    double posW[3] = {0.};
    double negW[3] = {0.};
    double pos[3]  = { halfwidth,  halfheight,  halflength};
    double neg[3]  = {-halfwidth, -halfheight, -halflength};
    
    this->Cryostat(cstat).LocalToWorld(pos, posW);
    this->Cryostat(cstat).LocalToWorld(neg, negW);

    boundaries[0] = negW[0];
    boundaries[1] = posW[0];
    boundaries[2] = negW[1];
    boundaries[3] = posW[1];
    boundaries[4] = negW[2];
    boundaries[5] = posW[2];
    
    return;
  }

  //......................................................................
  // This method returns the distance between the specified planes.
  // p1 < p2
  double GeometryCore::PlanePitch(unsigned int p1, 
                              unsigned int p2, 
                              unsigned int tpc,
                              unsigned int cstat) const
  { 
    return this->Cryostat(cstat).TPC(tpc).PlanePitch(p1, p2);
  }
      
  //......................................................................
  // This method returns the distance between the specified wires.
  // w1 < w2.  The wires are assumed to be on the same plane
  double GeometryCore::WirePitch(unsigned int w1,  
                             unsigned int w2,  
                             unsigned int plane,
                             unsigned int tpc,
                             unsigned int cstat) const
  { 
    return this->Cryostat(cstat).TPC(tpc).WirePitch(w1,w2,plane);    
  }

  //......................................................................
  // This method returns the distance between wires in the specified view
  // it assumes all planes of a given view have the same pitch
  double GeometryCore::WirePitch(geo::View_t view) const
  { 
    // loop over the planes in cryostat 0, tpc 0 to find the plane with the 
    // specified view
    unsigned int p = 0;
    for(p = 0; p < this->Cryostat(0).TPC(0).Nplanes(); ++p)
      if( this->Cryostat(0).TPC(0).Plane(p).View() == view ) break;

    return this->Cryostat(0).TPC(0).WirePitch(0, 1, p);
  }

  //......................................................................
  // This method returns the distance between wires in the specified view
  // it assumes all planes of a given view have the same pitch
  double GeometryCore::WireAngleToVertical(geo::View_t view, int TPC, int Cryo) const
  { 
    // loop over the planes in cryostat 0, tpc 0 to find the plane with the 
    // specified view
    unsigned int p = 0;
    for(p = 0; p < this->Cryostat(Cryo).TPC(TPC).Nplanes(); ++p)
      if( this->Cryostat(Cryo).TPC(TPC).Plane(p).View() == view ) break;

    return this->Cryostat(Cryo).TPC(TPC).Plane(p).Wire(0).ThetaZ(false);
  }

  //......................................................................
  //
  // Return the ranges of x,y and z for the "world volume" that the
  // entire geometry lives in. If any pointers are 0, then those
  // coordinates are ignored.
  //
  // \param xlo : On return, lower bound on x positions
  // \param xhi : On return, upper bound on x positions
  // \param ylo : On return, lower bound on y positions
  // \param yhi : On return, upper bound on y positions
  // \param zlo : On return, lower bound on z positions
  // \param zhi : On return, upper bound on z positions
  //
  void GeometryCore::WorldBox(double* xlo, double* xhi,
                          double* ylo, double* yhi,
                          double* zlo, double* zhi) const
  {
    const TGeoShape* s = gGeoManager->GetVolume("volWorld")->GetShape();
    if(!s)
      throw cet::exception("GeometryCore") << "no pointer to world volume TGeoShape\n";

    if (xlo || xhi) {
      double x1, x2;
      s->GetAxisRange(1,x1,x2); if (xlo) *xlo = x1; if (xhi) *xhi = x2;
    }
    if (ylo || yhi) {
      double y1, y2;
      s->GetAxisRange(2,y1,y2); if (ylo) *ylo = y1; if (yhi) *yhi = y2;
    }
    if (zlo || zhi) {
      double z1, z2;
      s->GetAxisRange(3,z1,z2); if (zlo) *zlo = z1; if (zhi) *zhi = z2;
    }
  }

  //......................................................................
  const TVector3 GeometryCore::GetTPCFrontFaceCenter(unsigned int tpc,
                                                   unsigned int cstat) const
  {
    return TVector3( 0.5 * this->DetHalfWidth(tpc, cstat), 0 , 0 );
  }

  //......................................................................
  const std::string GeometryCore::VolumeName(TVector3 point)
  {
    // check that the given point is in the World volume at least
    TGeoVolume *volWorld = gGeoManager->FindVolumeFast(this->GetWorldVolumeName().c_str());
    double halflength = ((TGeoBBox*)volWorld->GetShape())->GetDZ();
    double halfheight = ((TGeoBBox*)volWorld->GetShape())->GetDY();
    double halfwidth  = ((TGeoBBox*)volWorld->GetShape())->GetDX();
    if(std::abs(point.x()) > halfwidth  ||
       std::abs(point.y()) > halfheight ||
       std::abs(point.z()) > halflength
       ){
      mf::LogWarning("GeometryCoreBadInputPoint") << "point (" << point.x() << ","
                                              << point.y() << "," << point.z() << ") "
                                              << "is not inside the world volume "
                                              << " half width = " << halfwidth
                                              << " half height = " << halfheight
                                              << " half length = " << halflength
                                              << " returning unknown volume name";
      const std::string unknown("unknownVolume");
      return unknown;
    }
    
    const std::string name(gGeoManager->FindNode(point.x(), point.y(), point.z())->GetName());
    return name;
  }

  //......................................................................
  const std::string GeometryCore::MaterialName(TVector3 point)
  {
    // check that the given point is in the World volume at least
    TGeoVolume *volWorld = gGeoManager->FindVolumeFast(this->GetWorldVolumeName().c_str());
    double halflength = ((TGeoBBox*)volWorld->GetShape())->GetDZ();
    double halfheight = ((TGeoBBox*)volWorld->GetShape())->GetDY();
    double halfwidth  = ((TGeoBBox*)volWorld->GetShape())->GetDX();
    if(std::abs(point.x()) > halfwidth  ||
       std::abs(point.y()) > halfheight ||
       std::abs(point.z()) > halflength
       ){ 
      mf::LogWarning("GeometryCoreBadInputPoint") << "point (" << point.x() << ","
                                              << point.y() << "," << point.z() << ") "
                                              << "is not inside the world volume "
                                              << " half width = " << halfwidth
                                              << " half height = " << halfheight
                                              << " half length = " << halflength
                                              << " returning unknown material name";
      const std::string unknown("unknownMaterial");
      return unknown;
    }
    
    const std::string name(gGeoManager->FindNode(point.x(), 
                                                 point.y(), 
                                                 point.z())->GetMedium()->GetMaterial()->GetName());
    return name;
  }

  //......................................................................
  void GeometryCore::FindCryostat(std::vector<const TGeoNode*>& path,
                              unsigned int depth)
  {
    const char* nm = path[depth]->GetName();
    if( (strncmp(nm, "volCryostat", 11) == 0) ){
      this->MakeCryostat(path, depth);
      return;
    }
      
    //explore the next layer down
    unsigned int deeper = depth+1;
    if(deeper >= path.size()){
      throw cet::exception("GeometryCore") << "exceeded maximum TGeoNode depth\n";
    }

    const TGeoVolume *v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for(int i = 0; i < nd; ++i){
      path[deeper] = v->GetNode(i);
      this->FindCryostat(path, deeper);
    }

  }

  //......................................................................
  void GeometryCore::MakeCryostat(std::vector<const TGeoNode*>& path, int depth) 
  {
    Cryostats().push_back(new CryostatGeo(path, depth));
  }

  //......................................................................
  void GeometryCore::FindAuxDet(std::vector<const TGeoNode*>& path,
                            unsigned int depth)
  {
    const char* nm = path[depth]->GetName();
    if( (strncmp(nm, "volAuxDet", 9) == 0) ){
      this->MakeAuxDet(path, depth);
      return;
    }
    
    //explore the next layer down
    unsigned int deeper = depth+1;
    if(deeper >= path.size()){
      throw cet::exception("GeometryCore") << "exceeded maximum TGeoNode depth\n";
    }
    
    const TGeoVolume *v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for(int i = 0; i < nd; ++i){
      path[deeper] = v->GetNode(i);
      this->FindAuxDet(path, deeper);
    }
    
  }
  
  //......................................................................
  void GeometryCore::MakeAuxDet(std::vector<const TGeoNode*>& path, int depth)
  {
    AuxDets().push_back(new AuxDetGeo(path, depth));
  }

  //......................................................................
  //
  // Return the total mass of the detector
  //
  //
  double GeometryCore::TotalMass(const char *vol) const
  {
    //the TGeoNode::GetVolume() returns the TGeoVolume of the detector outline
    //and ROOT calculates the mass in kg for you
    TGeoVolume *gvol = gGeoManager->FindVolumeFast(vol);
    if(gvol) return gvol->Weight();

    throw cet::exception("GeometryCore") << "could not find specified volume " 
                                     << vol
                                     << " to determine total mass\n"; 
  }

  //......................................................................
  //
  // Return the column density between 2 points
  //
  // \param p1  : pointer to array holding xyz of first point in world coordinates
  // \param p2  : pointer to array holding xyz of second point in world coorinates
  //
  double GeometryCore::MassBetweenPoints(double *p1, double *p2) const
  {

    //The purpose of this method is to determine the column density
    //between the two points given.  Do that by starting at p1 and 
    //stepping until you get to the node of p2.  calculate the distance
    //between the point just inside that node and p2 to get the last
    //bit of column density
    double columnD = 0.;

    //first initialize a track - get the direction cosines
    double length = std::sqrt( sqr(p2[0]-p1[0])
                             + sqr(p2[1]-p1[1])
                             + sqr(p2[2]-p1[2]));
    double dxyz[3] = {(p2[0]-p1[0])/length, (p2[1]-p1[1])/length, (p2[2]-p1[2])/length}; 

    gGeoManager->InitTrack(p1,dxyz);

    //might be helpful to have a point to a TGeoNode
    TGeoNode *node = gGeoManager->GetCurrentNode();

    //check that the points are not in the same volume already.  
    //if they are in different volumes, keep stepping until you 
    //are in the same volume as the second point
    while(!gGeoManager->IsSameLocation(p2[0], p2[1], p2[2])){
      gGeoManager->FindNextBoundary();
      columnD += gGeoManager->GetStep()*node->GetMedium()->GetMaterial()->GetDensity();
    
      //the act of stepping puts you in the next node and returns that node
      node = gGeoManager->Step();
    }//end loop to get to volume of second point

    //now you are in the same volume as the last point, but not at that point.
    //get the distance between the current point and the last one
    const double *current = gGeoManager->GetCurrentPoint();
    length = std::sqrt( sqr(p2[0]-current[0])
                      + sqr(p2[1]-current[1])
                      + sqr(p2[2]-current[2]));
    columnD += length*node->GetMedium()->GetMaterial()->GetDensity();

    return columnD;
  }

  //......................................................................
  std::vector< geo::WireID > GeometryCore::ChannelToWire( raw::ChannelID_t channel ) const
  {
    return fChannelMapAlg->ChannelToWire(channel);
  }

  //----------------------------------------------------------------------------
  double GeometryCore::WireCoordinate(double YPos, double ZPos,
                                 unsigned int PlaneNo,
                                 unsigned int TPCNo,
                                 unsigned int cstat) const
  {
    return fChannelMapAlg->WireCoordinate(YPos, ZPos, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  // The NearestWire and PlaneWireToChannel are attempts to speed
  // up the simulation by memoizing the computationally intensive
  // setup steps for some geometry calculations.  The results are
  // valid assuming the wireplanes are comprised of straight,
  // parallel wires with constant pitch across the entire plane, with
  // a hierarchical numbering scheme - Ben J Oct 2011
  unsigned int GeometryCore::NearestWire(const TVector3& worldPos, 
                                     unsigned int const PlaneNo, 
                                     unsigned int const TPCNo,
                                     unsigned int const cstat) const
  {
    return fChannelMapAlg->NearestWire(worldPos, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  unsigned int GeometryCore::NearestWire(const double worldPos[3], 
                                     unsigned int const PlaneNo, 
                                     unsigned int const TPCNo,
                                     unsigned int const cstat) const
  {
    TVector3 wp(worldPos);
    return this->NearestWire(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  unsigned int GeometryCore::NearestWire(std::vector<double> const worldPos, 
                                     unsigned int const PlaneNo, 
                                     unsigned int const TPCNo,
                                     unsigned int const cstat) const
  {
    if(worldPos.size() > 3) throw cet::exception("GeometryCore") << "bad size vector for "
                                                             << "worldPos: " 
                                                             << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return this->NearestWire(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  const geo::WireID GeometryCore::NearestWireID(const TVector3& worldPos, 
                                            unsigned int const PlaneNo, 
                                            unsigned int const TPCNo,
                                            unsigned int const cstat) const
  {
    return fChannelMapAlg->NearestWireID(worldPos,PlaneNo,TPCNo,cstat);
  }

  //----------------------------------------------------------------------------
  const geo::WireID GeometryCore::NearestWireID(std::vector<double> worldPos, 
                                            unsigned int const  PlaneNo, 
                                            unsigned int const  TPCNo,
                                            unsigned int const  cstat) const
  {
    if(worldPos.size() > 3) throw cet::exception("GeometryCore") << "bad size vector for "
                                                             << "worldPos: " 
                                                             << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return this->NearestWireID(wp,PlaneNo,TPCNo,cstat);
  }

  //----------------------------------------------------------------------------
  const geo::WireID GeometryCore::NearestWireID(const double        worldPos[3], 
                                            unsigned int const  PlaneNo, 
                                            unsigned int const  TPCNo,
                                            unsigned int const  cstat) const
  {
    TVector3 wp(worldPos);
    return this->NearestWireID(wp,PlaneNo,TPCNo,cstat);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t GeometryCore::NearestChannel(const double worldPos[3], 
                                    unsigned int const PlaneNo, 
                                    unsigned int const TPCNo,
                                    unsigned int const cstat) const
  {
    TVector3 wp(worldPos);
    return this->NearestChannel(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t GeometryCore::NearestChannel(std::vector<double> const worldPos, 
                                    unsigned int const PlaneNo, 
                                    unsigned int const TPCNo,
                                    unsigned int const cstat) const
  {
    if(worldPos.size() > 3) throw cet::exception("GeometryCore") << "bad size vector for "
                                                             << "worldPos: " 
                                                             << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return this->NearestChannel(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t GeometryCore::NearestChannel(const TVector3& worldPos, 
                                    unsigned int const PlaneNo, 
                                    unsigned int const TPCNo,
                                    unsigned int const cstat) const
  {
    
    // This method is supposed to return a channel number rather than
    //  a wire number.  Perform the conversion here (although, maybe
    //  faster if we deal in wire numbers rather than channel numbers?)
    unsigned int nearestWire = this->NearestWire(worldPos, PlaneNo, TPCNo, cstat);
    return this->PlaneWireToChannel(PlaneNo, nearestWire, TPCNo, cstat);
  }

  //--------------------------------------
  raw::ChannelID_t GeometryCore::PlaneWireToChannel(unsigned int const plane,
                                        unsigned int const wire,
                                        unsigned int const tpc,
                                        unsigned int const cstat) const
  {
    return fChannelMapAlg->PlaneWireToChannel(plane, wire, tpc, cstat);
  }

  //......................................................................
  raw::ChannelID_t GeometryCore::PlaneWireToChannel(WireID const& wireid) const
  {
    return this->PlaneWireToChannel(wireid.Plane, wireid.Wire, wireid.TPC, wireid.Cryostat);   
  }

  // Functions to allow determination if two wires intersect, and if so where.
  // This is useful information during 3D reconstruction.
  //......................................................................
  bool GeometryCore::ValueInRange(double value, double min, double max) const
  {
    if(min>max) std::swap(min,max);//protect against funny business due to wire angles
    if (std::abs(value-min)<1e-6||std::abs(value-max)<1e-6) return true;
    return (value>=min) && (value<=max);
  }

  //......................................................................
  void GeometryCore::WireEndPoints(unsigned int cstat,
                               unsigned int tpc,
                               unsigned int plane, 
                               unsigned int wire, 
                               double *xyzStart, 
                               double *xyzEnd) const
  {  
    double halfL = this->Cryostat(cstat).TPC(tpc).Plane(plane).Wire(wire).HalfL();//half-length of wire
    this->Cryostat(cstat).TPC(tpc).Plane(plane).Wire(wire).GetCenter(xyzStart,halfL);
    this->Cryostat(cstat).TPC(tpc).Plane(plane).Wire(wire).GetCenter(xyzEnd,-1.0*halfL);
    
    if(xyzEnd[2]<xyzStart[2]){
      //ensure that "End" has higher z-value than "Start"
      std::swap(xyzStart[0],xyzEnd[0]);
      std::swap(xyzStart[1],xyzEnd[1]);
      std::swap(xyzStart[2],xyzEnd[2]);
    }
    if(xyzEnd[1]<xyzStart[1] && std::abs(xyzEnd[2]-xyzStart[2])<0.01){
      // if wire is vertical ensure that "End" has higher y-value than "Start"
      std::swap(xyzStart[0],xyzEnd[0]);
      std::swap(xyzStart[1],xyzEnd[1]);
      std::swap(xyzStart[2],xyzEnd[2]);
    }
    
    return;
  }
   
  //Changed to use WireIDsIntersect(). Apr, 2015 T.Yang
  //......................................................................
  bool GeometryCore::ChannelsIntersect(raw::ChannelID_t c1, 
                                   raw::ChannelID_t c2, 
                                   double &y, 
                                   double &z)
  {

    std::vector< geo::WireID > chan1wires, chan2wires; 

    chan1wires = ChannelToWire(c1);
    chan2wires = ChannelToWire(c2);

    if ( chan1wires.size() == 0 || chan2wires.size() == 0 ) {
      mf::LogWarning("ChannelsIntersect") << "one of the channels you gave was out of range " << std::endl
                                          << "channel 1 " << c1 << std::endl
                                          << "channel 2 " << c2 << std::endl;
      return false;
    }
    geo::WireIDIntersection widIntersect;
    if (this->WireIDsIntersect(chan1wires[0],chan2wires[0],widIntersect)){
      y = widIntersect.y;
      z = widIntersect.z;
      return true;
    }
    else{
      y = widIntersect.y;
      z = widIntersect.z;
      return false;
    }
  }

  // This function always calculates the intersection of two wires as long as they are in the same TPC and cryostat and not parallel. If the intersection is on both wires, it returns ture, otherwise it returns false. T.Yang
  //......................................................................
  bool GeometryCore::WireIDsIntersect(const geo::WireID& wid1, const geo::WireID& wid2, 
                                   geo::WireIDIntersection & widIntersect   ) const
  {
    widIntersect.y = -9999;
    widIntersect.z = -9999;
    widIntersect.TPC = 9999;

    double w1_Start[3] = {0.};
    double w1_End[3]   = {0.};
    double w2_Start[3] = {0.};
    double w2_End[3]   = {0.};

    if( wid1.Plane == wid2.Plane ){
      mf::LogWarning("WireIDsIntersect") << "Comparing two wires in the same plane, return false";
      return false;     }
    if( wid1.TPC != wid2.TPC ){
      mf::LogWarning("WireIDsIntersect") << "Comparing two wires in different TPCs, return false";
      return false;     }
    if( wid1.Cryostat != wid2.Cryostat ){
      mf::LogWarning("WireIDsIntersect") << "Comparing two wires in different Cryostats, return false";
      return false;     }
    if( wid1.Wire<0 || wid1.Wire>=this->Nwires(wid1.Plane, wid1.TPC, wid1.Cryostat)){
      mf::LogWarning("WireIDsIntersect") <<"wire number = "<<wid1.Wire<< "max wire number = "<< this->Nwires(wid1.Plane, wid1.TPC, wid1.Cryostat);
      return false;
    }
    if( wid2.Wire<0 || wid2.Wire>=this->Nwires(wid2.Plane, wid2.TPC, wid2.Cryostat)){
      mf::LogWarning("WireIDsIntersect") <<"wire number = "<<wid2.Wire<< "max wire number = "<< this->Nwires(wid2.Plane, wid2.TPC, wid2.Cryostat);
      return false;
    }
  

    // get the endpoints to see if i1 and i2 even intersect
    this->WireEndPoints(wid1.Cryostat, wid1.TPC, wid1.Plane, wid1.Wire, w1_Start, w1_End);
    this->WireEndPoints(wid2.Cryostat, wid2.TPC, wid2.Plane, wid2.Wire, w2_Start, w2_End);

    //Equation from http://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
    //T.Yang Nov, 2014
    double x1 = w1_Start[1];
    double y1 = w1_Start[2];
    double x2 = w1_End[1];
    double y2 = w1_End[2];
    double x3 = w2_Start[1];
    double y3 = w2_Start[2];
    double x4 = w2_End[1];
    double y4 = w2_End[2];

    double denom = (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4);
    if (!denom) {
      mf::LogWarning("WireIDsIntersect") << "Two wires are parallel, return false";
      return false;
    }
    double x = ((x1*y2-y1*x2)*(x3-x4)-(x1-x2)*(x3*y4-y3*x4))/denom;
    double y = ((x1*y2-y1*x2)*(y3-y4)-(y1-y2)*(x3*y4-y3*x4))/denom;

    widIntersect.y = x;
    widIntersect.z = y;
    widIntersect.TPC = wid1.TPC;
    if (this->ValueInRange(x,x1,x2) &&
        this->ValueInRange(x,x3,x4) &&
        this->ValueInRange(y,y1,y2) &&
        this->ValueInRange(y,y3,y4)){
      return true;
    }
    else{
      return false;
    }

    return false;
  }

  // Given slopes dTime/dWire in two planes, return with the slope in the 3rd plane.
  // B. Baller August 2014
  // Rewritten by T. Yang Apr 2015 using the equation in H. Greenlee's talk:
  // https://cdcvs.fnal.gov/redmine/attachments/download/1821/larsoft_apr20_2011.pdf
  // slide 2
  double Geometry::ThirdPlaneSlope(unsigned int plane1, double slope1, 
                                   unsigned int plane2, double slope2, 
                                   unsigned int tpc, unsigned int cstat)
  
  {

    if(Nplanes(tpc,cstat) != 3) return 999;
    if(plane1 > 2 || plane2 > 2) return 999;
    if(plane1==plane2) return 999;
    // Can't resolve very small slopes
    if(fabs(slope1) < 0.001 && fabs(slope2) < 0.001) return 0.001;

    // Calculate static variables on the first call
    static bool first = true;
    // We need the "wire coordinate direction" for each plane. This is perpendicular
    // to the wire orientation. 
    static double angle[3];
    if(first) {
      first = false;
      for (size_t i = 0; i<3; ++i){
	angle[i] = this->Cryostat(cstat).TPC(tpc).Plane(i).Wire(0).ThetaZ();
	//We need to subtract pi/2 to make those 'wire coordinate directions'.
	//But what matters is the difference between angles so we don't do that.
      }
    } // first
    unsigned int plane3 = 10;
    if ((plane1 == 0 && plane2 == 1)||(plane1 == 1 && plane2 == 0)) plane3 = 2;
    if ((plane1 == 0 && plane2 == 2)||(plane1 == 2 && plane2 == 0)) plane3 = 1;
    if ((plane1 == 1 && plane2 == 2)||(plane1 == 2 && plane2 == 1)) plane3 = 0;
    if (plane3>2) return 999;
    double slope3 = 0.001;
    if (fabs(slope1) > 0.001 && fabs(slope2) > 0.001) slope3 = ((1./slope1)*TMath::Sin(angle[plane3]-angle[plane2])-(1./slope2)*TMath::Sin(angle[plane3]-angle[plane1]))/TMath::Sin(angle[plane1]-angle[plane2]);
    if (slope3) slope3 = 1./slope3;
    else slope3 = 999;

    return slope3;

  } // ThirdPlaneSlope

   
  //......................................................................
  // This function is called if it is determined that two wires in a single TPC must overlap.
  // To determine the yz coordinate of the wire intersection, we need to know the 
  // endpoints of both wires in xyz-space, and also their orientation (angle), and the 
  // inner dimensions of the TPC frame.
  // Note: This calculation is entirely dependent  on an accurate GDML description of the TPC!
  // Mitch - Feb., 2011
  // Changed to use WireIDsIntersect(). It does not check whether the intersection is on both wires (the same as the old behavior). T. Yang - Apr, 2015
  void Geometry::IntersectionPoint(unsigned int wire1, 
                                   unsigned int wire2, 
                                   unsigned int plane1, 
                                   unsigned int plane2,
                                   unsigned int cstat,
                                   unsigned int tpc,
                                   double start_w1[3], 
                                   double end_w1[3], 
                                   double start_w2[3], 
                                   double end_w2[3], 
                                   double &y, double &z)
  {
    geo::WireID wid1(cstat,tpc,plane1,wire1);
    geo::WireID wid2(cstat,tpc,plane2,wire2);
    geo::WireIDIntersection widIntersect;
    this->WireIDsIntersect(wid1,wid2,widIntersect);
    y = widIntersect.y;
    z = widIntersect.z;
    return;
    
  }
    
  // Added shorthand function where start and endpoints are looked up automatically
  //  - whether to use this or the full function depends on optimization of your
  //    particular algorithm.  Ben J, Oct 2011
  //--------------------------------------------------------------------
  void GeometryCore::IntersectionPoint(unsigned int wire1, 
                                   unsigned int wire2, 
                                   unsigned int plane1, 
                                   unsigned int plane2,
                                   unsigned int cstat,
                                   unsigned int tpc, 
                                   double &y, double &z)
  {
    double WireStart1[3] = {0.};
    double WireStart2[3] = {0.};
    double WireEnd1[3]   = {0.};
    double WireEnd2[3]   = {0.};

    this->WireEndPoints(cstat, tpc, plane1, wire1, WireStart1, WireEnd1);
    this->WireEndPoints(cstat, tpc, plane2, wire2, WireStart2, WireEnd2);
    this->IntersectionPoint(wire1, wire2, plane1, plane2, cstat, tpc,
                            WireStart1, WireEnd1, WireStart2, WireEnd2, y, z);                     
  }


  //--------------------------------------------------------------------
  // Return gdml string which gives sensitive opdet name
  std::string GeometryCore::OpDetGeoName(unsigned int c) const
  {
    return Cryostat(c).OpDetGeoName();
  }

  //--------------------------------------------------------------------
  // Convert OpDet, Cryo into unique OpDet number
  unsigned int GeometryCore::OpDetFromCryo(unsigned int o, unsigned int c ) const
  {
    static bool Loaded=false;
    static std::vector<unsigned int> LowestID;
    static unsigned int NCryo;
    // If not yet loaded static parameters, do it
    if(Loaded == false){
        
      Loaded = true;
      
      // Store the lowest ID for each cryostat
      NCryo=Ncryostats();
      LowestID.resize(NCryo + 1);
      LowestID.at(0)=0;        
      for(size_t cryo=0; cryo!=NCryo; ++cryo){
        LowestID.at(cryo+1)=LowestID.at(cryo)+Cryostat(c).NOpDet();
      }
      
    }

    if( (c < NCryo) && (o < Cryostat(c).NOpDet())){
      return LowestID.at(c)+o;
    }
    else{
      throw cet::exception("OpDetCryoToOpID Error") << "Coordinates c=" << c 
                                                    << ", o=" << o 
                                                    << " out of range. Abort\n";
    }
    
    // if all is well, we never get to this point in the method
    // but still a good idea to be sure to always return something.

    return INT_MAX;  
  }

  //--------------------------------------------------------------------
  const OpDetGeo& GeometryCore::OpDetGeoFromOpChannel(unsigned int OpChannel) const
  {
    return this->OpDetGeoFromOpDet(this->OpDetFromOpChannel(OpChannel));
  }

  //--------------------------------------------------------------------
  const OpDetGeo& GeometryCore::OpDetGeoFromOpDet(unsigned int OpDet) const
  {
    static bool Loaded=false;
    static std::vector<unsigned int> LowestID;
    static size_t NCryo;
    // If not yet loaded static parameters, do it
    if(Loaded == false){
      
      Loaded = true;

      // Store the lowest ID for each cryostat
      NCryo=Ncryostats();
      LowestID.resize(NCryo + 1);
      LowestID[0] = 0;        
      for(size_t cryo = 0; cryo != NCryo; ++cryo){
        LowestID[cryo+1] = LowestID[cryo] + Cryostat(cryo).NOpDet();
      }
        
    }

    for(size_t i=0; i!=NCryo; ++i){
      if( (OpDet >= LowestID[i]) && (OpDet < LowestID[i+1]) ){
        int c = i;
        int o = OpDet-LowestID[i]; 
        return this->Cryostat(c).OpDet(o);
      }
    }
    // If we made it here, we didn't find the right combination. abort
    throw cet::exception("OpID To OpDetCryo error")<<"OpID out of range, "<< OpDet << "\n";

    // Will not reach due to exception
    return this->Cryostat(0).OpDet(0);
  }
  
  
  //--------------------------------------------------------------------
  // Find the closest OpChannel to this point, in the appropriate cryostat  
  unsigned int GeometryCore::GetClosestOpDet(double * xyz) const
  {        
    unsigned int c;
    PositionToCryostat(xyz, c);
    int o = Cryostat(c).GetClosestOpDet(xyz);
    return OpDetFromCryo(o, c);
  }
  
  //--------------------------------------------------------------------
  WireGeo const& GeometryCore::WireIDToWireGeo(geo::WireID const& CodeWire) const
  {
    unsigned int cryo  = CodeWire.Cryostat;
    unsigned int tpc   = CodeWire.TPC;
    unsigned int plane = CodeWire.Plane;
    unsigned int wire  = CodeWire.Wire;
    
    return this->Cryostat(cryo).TPC(tpc).Plane(plane).Wire(wire);
  }
  
  
  //--------------------------------------------------------------------
  constexpr details::geometry_iterator_base::BeginPos_t
    details::geometry_iterator_base::begin_pos;
  constexpr details::geometry_iterator_base::EndPos_t
    details::geometry_iterator_base::end_pos;
  constexpr details::geometry_iterator_base::UndefinedPos_t
    details::geometry_iterator_base::undefined_pos;
  
  //--------------------------------------------------------------------
  void cryostat_iterator::next() {
    if (!id.isValid) return;
    if (++id.Cryostat < limits.Cryostat) return;
    id.isValid = false;
  } // cryostat_iterator::next()
  
  
  void cryostat_iterator::prev() {
    if (!id.isValid) return;
    if (id.Cryostat-- >= 0) return;
    id.isValid = false;
  } // cryostat_iterator::prev()
  
  
  const CryostatGeo* cryostat_iterator::get() const
    { return id.isValid? &(pGeo->Cryostat(id.Cryostat)): nullptr; }
  
  
  void cryostat_iterator::set_limits() {
    limits = CryostatID(pGeo->Ncryostats());
    limits.isValid = false;
  } // cryostat_iterator::set_limits()
  
  
  //--------------------------------------------------------------------
  TPC_iterator& TPC_iterator::operator++() {
    if (!tpcid.isValid) return *this;
    
    ++tpcid.TPC;
    while (true) {
      if (tpcid.TPC < limits.TPC) return *this;
      tpcid.TPC = 0;
      if (++tpcid.Cryostat >= limits.Cryostat) break;
      new_cryostat();
    } // while
    tpcid.isValid = false;
    return *this;
  } // TPC_iterator::operator++()
  
  
  const TPCGeo* TPC_iterator::get() const
    { return tpcid.isValid? &(pGeo->TPC(tpcid.TPC, tpcid.Cryostat)): nullptr; }
  
  
  const CryostatGeo* TPC_iterator::getCryostat() const {
    return tpcid.isValid? &(pGeo->Cryostat(tpcid.Cryostat)): nullptr;
  } // TPC_iterator::getCryostat()
  
  
  void TPC_iterator::set_limits_and_validity() {
    tpcid.isValid = false;
    limits.Cryostat = pGeo->Ncryostats();
    if (tpcid.Cryostat >= limits.Cryostat) return;
    limits.TPC = pGeo->NTPC(tpcid.Cryostat);
    if (tpcid.TPC >= limits.TPC) return;
    tpcid.isValid = true;
  } // TPC_iterator::set_limits_and_validity()
  
  
  void TPC_iterator::new_cryostat() {
    tpcid.TPC = 0;
    limits.TPC = pGeo->NTPC(tpcid.Cryostat);
  } // TPC_iterator::new_cryostat()
  
  
  //--------------------------------------------------------------------
  plane_iterator& plane_iterator::operator++() {
    if (!planeid.isValid) return *this;
    
    ++planeid.Plane;
    while (true) {
      if (planeid.Plane < limits.Plane) return *this;
      planeid.Plane = 0;
      if (++planeid.TPC >= limits.TPC) {
        planeid.TPC = 0;
        if (++planeid.Cryostat >= limits.Cryostat) break;
        new_cryostat();
      }
      new_tpc();
    } // while
    planeid.isValid = false;
    return *this;
  } // plane_iterator::operator++()
  
  
  const PlaneGeo* plane_iterator::get() const {
    return planeid.isValid?
      &(pGeo->Plane(planeid.Plane, planeid.TPC, planeid.Cryostat)): nullptr;
  } // plane_iterator::get()
  
  
  const TPCGeo* plane_iterator::getTPC() const {
    return planeid.isValid?
      &(pGeo->TPC(planeid.TPC, planeid.Cryostat)): nullptr;
  } // plane_iterator::getTPC()
  
  
  const CryostatGeo* plane_iterator::getCryostat() const {
    return planeid.isValid? &(pGeo->Cryostat(planeid.Cryostat)): nullptr;
  } // plane_iterator::getCryostat()
  
  
  void plane_iterator::set_limits_and_validity() {
    planeid.isValid = false;
    limits.Cryostat = pGeo->Ncryostats();
    if (planeid.Cryostat >= limits.Cryostat) return;
    const CryostatGeo& cryo = pGeo->Cryostat(planeid.Cryostat);
    limits.TPC = cryo.NTPC();
    if (planeid.TPC >= limits.TPC) return;
    const TPCGeo& TPC = cryo.TPC(planeid.TPC);
    limits.Plane = TPC.Nplanes();
    if (planeid.Plane >= limits.Plane) return;
    planeid.isValid = true;
  } // plane_iterator::set_limits_and_validity()
  
  
  void plane_iterator::new_cryostat() {
    planeid.TPC = 0;
    limits.TPC = pGeo->NTPC(planeid.Cryostat);
  } // plane_iterator::new_cryostat()
  
  
  void plane_iterator::new_tpc() {
    planeid.Plane = 0;
    limits.Plane = pGeo->Nplanes(planeid.TPC, planeid.Cryostat);
  } // plane_iterator::new_tpc()
  
  
  //--------------------------------------------------------------------
  wire_iterator& wire_iterator::operator++() {
    if (!wireid.isValid) return *this;
    
    ++wireid.Wire;
    while (true) {
      if (wireid.Wire < limits.Wire) return *this;
      wireid.Wire = 0;
      if (++wireid.Plane >= limits.Plane) {
        wireid.Plane = 0;
        if (++wireid.TPC >= limits.TPC) {
          wireid.TPC = 0;
          if (++wireid.Cryostat >= limits.Cryostat) break;
          new_cryostat();
        } // if new cryostat
        new_tpc();
      } // if new TPC
      new_plane();
    } // while
    wireid.isValid = false;
    return *this;
  } // wire_iterator::operator++()
  
  
  const WireGeo* wire_iterator::get() const {
    return wireid.isValid? &(getPlane()->Wire(wireid.Wire)): nullptr;
  } // wire_iterator::get()
  
  
  const PlaneGeo* wire_iterator::getPlane() const {
    return wireid.isValid?
      &(pGeo->Plane(wireid.Plane, wireid.TPC, wireid.Cryostat)): nullptr;
  } // wire_iterator::get()
  
  
  const TPCGeo* wire_iterator::getTPC() const {
    return wireid.isValid? &(pGeo->TPC(wireid.TPC, wireid.Cryostat)): nullptr;
  } // wire_iterator::getTPC()
  
  
  const CryostatGeo* wire_iterator::getCryostat() const {
    return wireid.isValid? &(pGeo->Cryostat(wireid.Cryostat)): nullptr;
  } // wire_iterator::getCryostat()
  
  
  void wire_iterator::set_limits_and_validity() {
    wireid.isValid = false;
    limits.Cryostat = pGeo->Ncryostats();
    if (wireid.Cryostat >= limits.Cryostat) return;
    const CryostatGeo& cryo = pGeo->Cryostat(wireid.Cryostat);
    limits.TPC = cryo.NTPC();
    if (wireid.TPC >= limits.TPC) return;
    const TPCGeo& TPC = cryo.TPC(wireid.TPC);
    limits.Plane = TPC.Nplanes();
    if (wireid.Plane >= limits.Plane) return;
    const PlaneGeo& Plane = TPC.Plane(wireid.Plane);
    limits.Wire = Plane.Nwires();
    if (wireid.Wire >= limits.Wire) return;
    wireid.isValid = true;
  } // wire_iterator::set_limits_and_validity()
  
  
  void wire_iterator::new_cryostat() {
    wireid.TPC = 0;
    limits.TPC = pGeo->NTPC(wireid.Cryostat);
  } // wire_iterator::new_cryostat()
  
  
  void wire_iterator::new_tpc() {
    wireid.Plane = 0;
    limits.Plane = pGeo->Nplanes(wireid.TPC, wireid.Cryostat);
  } // wire_iterator::new_tpc()
  
  void wire_iterator::new_plane() {
    wireid.Wire = 0;
    limits.Wire = pGeo->Nwires(wireid.Plane, wireid.TPC, wireid.Cryostat);
  } // wire_iterator::new_plane()
  
  
} // namespace geo
