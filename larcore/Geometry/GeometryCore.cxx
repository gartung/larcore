/**
 * @file   GeometryCore.cxx
 * @brief  Access the description of detector geometry - implementation file
 * @author brebel@fnal.gov
 * @see    GeometryCore.h
 *
 */

// class header
#include "larcore/Geometry/GeometryCore.h"

// lar includes
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h" // util::pi<>
#include "larcore/Geometry/OpDetGeo.h"
#include "larcore/Geometry/AuxDetGeo.h"
#include "larcore/Geometry/AuxDetSensitiveGeo.h"

// Framework includes
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// ROOT includes
#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TGeoMatrix.h>
#include <TGeoBBox.h>
#include <TGeoVolume.h>
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
  } // GeometryCore::ApplyChannelMap()

  //......................................................................
  void GeometryCore::LoadGeometryFile(
    std::string gdmlfile, std::string rootfile,
    bool bForceReload /* = false*/
  ) {
    
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
    // Then lock the gGeoManager to prevent future imports, for example
    // in AuxDetGeometry
    if( !gGeoManager || bForceReload ){
      if (gGeoManager) TGeoManager::UnlockGeometry();
      TGeoManager::Import(rootfile.c_str());
      gGeoManager->LockGeometry();
    }

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
  unsigned int GeometryCore::MaxOpChannel() const
  {
    return fChannelMapAlg->MaxOpChannel(this->NOpDets());
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
  unsigned int GeometryCore::NAuxDetSensitive(size_t const& aid) const
  {
    if( aid > NAuxDets() - 1)
      throw cet::exception("Geometry") << "Requested AuxDet index " << aid 
				       << " is out of range: " << NAuxDets();

    return AuxDets()[aid]->NSensitiveVolume();
  }

  //......................................................................
  // Number of different views, or wire orientations
  unsigned int GeometryCore::Nviews() const
  {
    return MaxPlanes();
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
  CryostatGeo const& GeometryCore::Cryostat(CryostatID const& cryoid) const {
    CryostatGeo const* pCryo = CryostatPtr(cryoid);
    if(!pCryo) {
      throw cet::exception("GeometryCore") << "Cryostat #"
                                           << cryoid.Cryostat
                                           << " does not exist\n";
    }
    return *pCryo;
  } // GeometryCore::Cryostat(CryostatID)

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
  const TPCGeo& GeometryCore::PositionToTPC
    (double const  worldLoc[3], geo::TPCID& tpcid) const
  {
    return PositionToCryostat(worldLoc, tpcid.Cryostat)
      .PositionToTPC(worldLoc, tpcid.TPC, 1.+fPositionWiggle);
  }

  const TPCGeo& GeometryCore::PositionToTPC(double const  worldLoc[3],
                                        unsigned int &tpc,
                                        unsigned int &cstat) const
  {
    geo::TPCID tpcid;
    TPCGeo const& TPC = PositionToTPC(worldLoc, tpcid);
    cstat = tpcid.Cryostat;
    tpc = tpcid.TPC;
    return TPC;
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
    return geo::CryostatID::InvalidID;
  } // GeometryCore::FindCryostatAtPosition()

  //......................................................................
  const CryostatGeo& GeometryCore::PositionToCryostat
    (double const  worldLoc[3], geo::CryostatID& cid) const
  {
    geo::CryostatID::CryostatID_t cstat = FindCryostatAtPosition(worldLoc);
    
    if(cstat == geo::CryostatID::InvalidID)
      throw cet::exception("GeometryCore") << "Can't find Cryostat for position (" 
                                       << worldLoc[0] << ","
                                       << worldLoc[1] << "," 
                                       << worldLoc[2] << ")\n";
    cid = geo::CryostatID(cstat);
    return Cryostat(cid);
  }
  
  const CryostatGeo& GeometryCore::PositionToCryostat
    (double const worldLoc[3], unsigned int &cstat) const
  {
    geo::CryostatID cid;
    geo::CryostatGeo const& cryo = PositionToCryostat(worldLoc, cid);
    cstat = cid.Cryostat;
    return cryo;
  }
  
  //......................................................................
  unsigned int GeometryCore::FindAuxDetAtPosition(double const  worldPos[3]) const
  {
    return fChannelMapAlg->NearestAuxDet(worldPos, AuxDets());
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
  void GeometryCore::FindAuxDetSensitiveAtPosition(double const worldPos[3],
					       size_t     & adg,
					       size_t     & sv) const
  {
    adg = this->FindAuxDetAtPosition(worldPos);
    sv  = fChannelMapAlg->NearestSensitiveAuxDet(worldPos, AuxDets());

    return;
  } // GeometryCore::FindAuxDetAtPosition()
  

  
  //......................................................................
  const AuxDetSensitiveGeo& GeometryCore::PositionToAuxDetSensitive(double const worldLoc[3],
								size_t      &ad,
								size_t      &sv) const
  {    
    // locate the desired Auxiliary Detector
    this->FindAuxDetSensitiveAtPosition(worldLoc, ad, sv);    
    return this->AuxDet(ad).SensitiveVolume(sv);
  }
  
  //......................................................................
  const AuxDetGeo& GeometryCore::ChannelToAuxDet(std::string const& auxDetName,
					     uint32_t    const& channel) const
  {
    size_t adIdx = fChannelMapAlg->ChannelToAuxDet(AuxDets(), auxDetName, channel);
    return this->AuxDet(adIdx);
  }

  //......................................................................
  const AuxDetSensitiveGeo& GeometryCore::ChannelToAuxDetSensitive(std::string const& auxDetName,
						      uint32_t    const& channel) const
  {
    auto idx = fChannelMapAlg->ChannelToSensitiveAuxDet(AuxDets(), auxDetName, channel);
    return this->AuxDet(idx.first).SensitiveVolume(idx.second);
  }

  
  //......................................................................
  SigType_t GeometryCore::SignalType(raw::ChannelID_t const channel) const
  {
    return fChannelMapAlg->SignalType(channel);
  }

  //......................................................................
  SigType_t GeometryCore::SignalType(geo::PlaneID const& pid) const
  {
    return Plane(pid.Plane).SignalType();
  }


  //......................................................................
  View_t GeometryCore::View(raw::ChannelID_t const channel) const
  {
    return fChannelMapAlg->View(channel);
  }

  //......................................................................
  View_t GeometryCore::View(geo::PlaneID const& pid) const
  {
    return Plane(pid.Plane).View();
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
  struct NodeNameMatcherClass {
    std::set<std::string> const* vol_names;
    
    NodeNameMatcherClass(std::set<std::string> const& names)
      : vol_names(&names) {}
    
    /// Returns whether the specified node matches a set of names
    bool operator() (TGeoNode const& node) const
      {
        if (!vol_names) return true;
        return vol_names->find(node.GetVolume()->GetName()) != vol_names->end();
      }
    
  }; // NodeNameMatcherClass
  
  struct CollectNodesByName {
    std::vector<TGeoNode const*> nodes;
    
    CollectNodesByName(std::set<std::string> const& names): matcher(names) {}
    
    /// If the name of the node matches, records the end node
    void operator() (TGeoNode const& node)
      { if (matcher(node)) nodes.push_back(&node); }
    
    void operator() (ROOTGeoNodeForwardIterator const& iter)
      { operator() (**iter); }
    
      protected:
    NodeNameMatcherClass matcher;
  }; // CollectNodesByName
  
  struct CollectPathsByName {
    std::vector<std::vector<TGeoNode const*>> paths;
    
    CollectPathsByName(std::set<std::string> const& names): matcher(names) {}
    
    /// If the name of the node matches, records the node full path
    void operator() (ROOTGeoNodeForwardIterator const& iter)
      { if (matcher(**iter)) paths.push_back(iter.get_path()); }
    
      protected:
    NodeNameMatcherClass matcher;
  }; // CollectPathsByName
  
  
  
  std::vector<TGeoNode const*> GeometryCore::FindAllVolumes
    (std::set<std::string> const& vol_names) const
  {
    CollectNodesByName node_collector(vol_names);
    
    ROOTGeoNodeForwardIterator iNode(ROOTGeoManager()->GetTopNode());
    TGeoNode const* pCurrentNode;
    
    while ((pCurrentNode = *iNode)) {
      node_collector(*pCurrentNode);
      ++iNode;
    } // while
    
    return node_collector.nodes;
  } // GeometryCore::FindAllVolumes()
  
  
  std::vector<std::vector<TGeoNode const*>> GeometryCore::FindAllVolumePaths
    (std::set<std::string> const& vol_names) const
  {
    CollectPathsByName path_collector(vol_names);
    
    ROOTGeoNodeForwardIterator iNode(ROOTGeoManager()->GetTopNode());
    
    while (*iNode) {
      path_collector(iNode);
      ++iNode;
    } // while
    
    return path_collector.paths;
  } // GeometryCore::FindAllVolumePaths()
  
  
  
  //......................................................................
  std::string GeometryCore::GetLArTPCVolumeName(geo::TPCID const& tpcid) const
  {
    return std::string(TPC(tpcid).ActiveVolume()->GetName());
  }

  //......................................................................
  std::string GeometryCore::GetCryostatVolumeName(geo::CryostatID const& cid) const
  {
    return std::string(Cryostat(cid).Volume()->GetName());
  }

  //......................................................................
  double GeometryCore::DetHalfWidth(geo::TPCID const& tpcid)  const 
  {
    return TPC(tpcid).ActiveHalfWidth();
  }

  //......................................................................
  double GeometryCore::DetHalfHeight(geo::TPCID const& tpcid) const 
  {
    return TPC(tpcid).ActiveHalfHeight();
  }

  //......................................................................
  double GeometryCore::DetLength(geo::TPCID const& tpcid) const
  { 
    return TPC(tpcid).ActiveLength();
  }

  //......................................................................
  double GeometryCore::CryostatHalfWidth(geo::CryostatID const& cid) const
  {
    return Cryostat(cid).HalfWidth();
  }

  //......................................................................
  double GeometryCore::CryostatHalfHeight(geo::CryostatID const& cid) const
  {
    return Cryostat(cid).HalfHeight();
  }

  //......................................................................
  double GeometryCore::CryostatLength(geo::CryostatID const& cid) const
  {
    return Cryostat(cid).Length();
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
                                    geo::CryostatID const& cid) const
  {
    geo::CryostatGeo const& cryo = Cryostat(cid);
    TGeoBBox const* CryoShape = ((TGeoBBox*) cryo.Volume()->GetShape());
    // get the half width, height, etc of the cryostat
    const double halflength = CryoShape->GetDZ();
    const double halfwidth  = CryoShape->GetDX();
    const double halfheight = CryoShape->GetDY();
    
    double posW[3] = {0.};
    double negW[3] = {0.};
    double pos[3]  = { halfwidth,  halfheight,  halflength};
    double neg[3]  = {-halfwidth, -halfheight, -halflength};
    
    cryo.LocalToWorld(pos, posW);
    cryo.LocalToWorld(neg, negW);

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
  double GeometryCore::PlanePitch(
    geo::TPCID const& tpcid,
    geo::PlaneID::PlaneID_t p1, geo::PlaneID::PlaneID_t p2
    ) const
  {
    return TPC(tpcid).PlanePitch(p1, p2);
  }
  
  double GeometryCore::PlanePitch
    (geo::PlaneID const& pid1, geo::PlaneID const& pid2) const
  {
    return PlanePitch
      (static_cast<geo::TPCID const&>(pid1), pid1.Plane, pid2.Plane);
  }
  
  double GeometryCore::PlanePitch(unsigned int p1, 
                              unsigned int p2, 
                              unsigned int tpc,
                              unsigned int cstat) const
  { 
    return PlanePitch(geo::TPCID(cstat, tpc), p1, p2);
  }
  
  //......................................................................
  // This method returns the distance between the specified wires.
  // w1 < w2.  The wires are assumed to be on the same plane
  double GeometryCore::WirePitch(
    geo::PlaneID const& planeid,
    unsigned int w1 /* = 0 */, unsigned int w2 /* = 1 */
    )
    const
  {
    return Plane(planeid).WirePitch();
  }

  //......................................................................
  // This method returns the distance between wires in the specified view
  // it assumes all planes of a given view have the same pitch
  double GeometryCore::WirePitch(geo::View_t view) const
  { 
    // look in cryostat 0, tpc 0 to find the plane with the 
    // specified view
    return TPC({ 0, 0 }).Plane(view).WirePitch();
  }

  //......................................................................
  // This method returns the distance between wires in the specified view
  // it assumes all planes of a given view have the same pitch
  double GeometryCore::WireAngleToVertical
    (geo::View_t view, geo::TPCID const& tpcid) const
  {
    // loop over the planes in cryostat 0, tpc 0 to find the plane with the 
    // specified view
    geo::TPCGeo const& TPC = this->TPC(tpcid);
    for (unsigned int p = 0; p < TPC.Nplanes(); ++p) {
      geo::PlaneGeo const& plane = TPC.Plane(p);
      if (plane.View() == view) return plane.ThetaZ();
    } // for
    throw cet::exception("GeometryCore") << "WireAngleToVertical(): no view #"
      << ((int) view) << " in " << std::string(tpcid);
  } // GeometryCore::WireAngleToVertical()

  //......................................................................
  unsigned int GeometryCore::MaxTPCs() const {
    unsigned int maxTPCs = 0;
    for (geo::CryostatGeo const* pCryo: Cryostats()) {
      if (!pCryo) continue;
      unsigned int maxTPCsInCryo = pCryo->NTPC();
      if (maxTPCsInCryo > maxTPCs) maxTPCs = maxTPCsInCryo;
    } // for
    return maxTPCs;
  } // GeometryCore::MaxTPCs()
  
  //......................................................................
  unsigned int GeometryCore::TotalNTPC() const {
    // it looks like C++11 lambdas have made STL algorithms easier to use,
    // but only so much:
    return std::accumulate(Cryostats().begin(), Cryostats().end(), 0U,
      [](unsigned int sum, geo::CryostatGeo const* pCryo)
        { return sum + (pCryo? pCryo->NTPC(): 0U); }
      );
  } // GeometryCore::TotalNTPC()
  
  //......................................................................
  unsigned int GeometryCore::MaxPlanes() const {
    unsigned int maxPlanes = 0;
    for (geo::CryostatGeo const* pCryo: Cryostats()) {
      if (!pCryo) continue;
      unsigned int maxPlanesInCryo = pCryo->MaxPlanes();
      if (maxPlanesInCryo > maxPlanes) maxPlanes = maxPlanesInCryo;
    } // for
    return maxPlanes;
  } // GeometryCore::MaxPlanes()
  
  //......................................................................
  unsigned int GeometryCore::MaxWires() const {
    unsigned int maxWires = 0;
    for (geo::CryostatGeo const* pCryo: Cryostats()) {
      if (!pCryo) continue;
      unsigned int maxWiresInCryo = pCryo->MaxWires();
      if (maxWiresInCryo > maxWires) maxWires = maxWiresInCryo;
    } // for
    return maxWires;
  } // GeometryCore::MaxWires()
  
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
  TVector3 GeometryCore::GetTPCFrontFaceCenter(geo::TPCID const& tpcid) const
  {
    return TVector3( 0.5 * DetHalfWidth(tpcid), 0 , 0 );
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
  double GeometryCore::WireCoordinate
    (double YPos, double ZPos, geo::PlaneID const& planeid) const
  {
    return fChannelMapAlg->WireCoordinate(YPos, ZPos, planeid);
  }

  //----------------------------------------------------------------------------
  // The NearestWire and PlaneWireToChannel are attempts to speed
  // up the simulation by memoizing the computationally intensive
  // setup steps for some geometry calculations.  The results are
  // valid assuming the wireplanes are comprised of straight,
  // parallel wires with constant pitch across the entire plane, with
  // a hierarchical numbering scheme - Ben J Oct 2011
  unsigned int GeometryCore::NearestWire
    (const TVector3& worldPos, geo::PlaneID const& planeid) const
  {
    return fChannelMapAlg->NearestWire(worldPos, planeid);
  }

  //----------------------------------------------------------------------------
  unsigned int GeometryCore::NearestWire
    (const double worldPos[3], geo::PlaneID const& planeid) const
  {
    return NearestWire(TVector3(worldPos), planeid);
  }

  //----------------------------------------------------------------------------
  unsigned int GeometryCore::NearestWire
    (std::vector<double> const& worldPos, geo::PlaneID const& planeid) const
  {
    if(worldPos.size() > 3) throw cet::exception("GeometryCore") << "bad size vector for "
                                                             << "worldPos: " 
                                                             << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return NearestWire(wp, planeid);
  }

  //----------------------------------------------------------------------------
  geo::WireID GeometryCore::NearestWireID
    (const TVector3& worldPos, geo::PlaneID const& planeid) const
  {
    return fChannelMapAlg->NearestWireID(worldPos, planeid);
  }

  //----------------------------------------------------------------------------
  geo::WireID GeometryCore::NearestWireID
    (std::vector<double> const& worldPos, geo::PlaneID const& planeid) const
  {
    if(worldPos.size() > 3) throw cet::exception("GeometryCore") << "bad size vector for "
                                                             << "worldPos: " 
                                                             << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return NearestWireID(wp, planeid);
  }

  //----------------------------------------------------------------------------
  geo::WireID GeometryCore::NearestWireID
    (const double worldPos[3], geo::PlaneID const& planeid) const
  {
    return NearestWireID(TVector3(worldPos), planeid);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t GeometryCore::NearestChannel
    (const double worldPos[3], geo::PlaneID const& planeid) const
  {
    return NearestChannel(TVector3(worldPos), planeid);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t GeometryCore::NearestChannel
    (std::vector<double> const& worldPos, geo::PlaneID const& planeid) const
  {
    if(worldPos.size() > 3) throw cet::exception("GeometryCore") << "bad size vector for "
                                                             << "worldPos: " 
                                                             << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return NearestChannel(wp, planeid);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t GeometryCore::NearestChannel
    (const TVector3& worldPos, geo::PlaneID const& planeid) const
  {
    
    // This method is supposed to return a channel number rather than
    //  a wire number.  Perform the conversion here (although, maybe
    //  faster if we deal in wire numbers rather than channel numbers?)
    return PlaneWireToChannel(NearestWireID(worldPos, planeid));
  }

  //--------------------------------------
  raw::ChannelID_t GeometryCore::PlaneWireToChannel(WireID const& wireid) const
  {
    return fChannelMapAlg->PlaneWireToChannel(wireid);
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
  void GeometryCore::WireEndPoints
    (geo::WireID const& wireid, double *xyzStart, double *xyzEnd) const
  {
    geo::WireGeo const& wire = Wire(wireid);
    const double halfL = wire.HalfL();//half-length of wire
    wire.GetCenter(xyzStart,halfL);
    wire.GetCenter(xyzEnd,-halfL);
    
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
    
  }
   
  //Changed to use WireIDsIntersect(). Apr, 2015 T.Yang
  //......................................................................
  bool GeometryCore::ChannelsIntersect(raw::ChannelID_t c1, 
                                   raw::ChannelID_t c2, 
                                   double &y, 
                                   double &z) const
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
  
  
  //----------------------------------------------------------------------------
  geo::PlaneID GeometryCore::ThirdPlane
    (geo::PlaneID const& pid1, geo::PlaneID const& pid2) const
  {
    // how many planes in the TPC pid1 belongs to:
    const unsigned int nPlanes = Nplanes(pid1);
    if(nPlanes != 3) {
      throw cet::exception("GeometryCore")
        << "ThirdPlane() supports only TPCs with 3 planes, and I see "
        << nPlanes << " instead\n";
    }
    
    geo::PlaneID::PlaneID_t target_plane = nPlanes;
    for (geo::PlaneID::PlaneID_t iPlane = 0; iPlane < nPlanes; ++iPlane){
      if ((iPlane == pid1.Plane) || (iPlane == pid2.Plane)) continue;
      if (target_plane != nPlanes) {
        throw cet::exception("GeometryCore")
          << "ThirdPlane() found too many planes that are not "
          << std::string(pid1) << " nor " << std::string(pid2)
          << "! (first " << target_plane << ", then " << iPlane << ")\n";
      } // if we had a target already
      target_plane = iPlane;
    } // for
    if (target_plane == nPlanes) {
      throw cet::exception("GeometryCore")
        << "ThirdPlane() can't find a plane that is not " << std::string(pid1)
        << " nor " << std::string(pid2) << "!\n";
    }
    
    return geo::PlaneID(pid1, target_plane);
  } // GeometryCore::ThirdPlane()
  
  
  void GeometryCore::CheckIndependentPlanesOnSameTPC
    (geo::PlaneID const& pid1, geo::PlaneID const& pid2, const char* caller)
  {
    if(static_cast<geo::TPCID const&>(pid1) != static_cast<geo::TPCID const&>(pid2)) {
      throw cet::exception("GeometryCore")
        << caller << " needs two planes on the same TPC (got "
        << std::string(pid1) << " and " << std::string(pid2) << ")\n";
    }
    if(pid1 == pid2) { // was: return 999;
      throw cet::exception("GeometryCore")
        << caller << " needs two different planes, got "
        << std::string(pid1) << " twice\n";
    }
  } // GeometryCore::CheckIndependentPlanesOnSameTPC()
  
  
  double GeometryCore::ThirdPlaneSlope(
    geo::PlaneID const& pid1, double slope1,
    geo::PlaneID const& pid2, double slope2,
    geo::PlaneID const& output_plane
  ) const {
    
    CheckIndependentPlanesOnSameTPC(pid1, pid2, "ThirdPlaneSlope()");
    
    geo::TPCGeo const& TPC = this->TPC(pid1);

    // We need the "wire coordinate direction" for each plane.
    // This is perpendicular to the wire orientation.
    // PlaneGeo::PhiZ() defines the right orientation too.
    return ComputeThirdPlaneSlope(
      TPC.Plane(pid1).PhiZ(), slope1,
      TPC.Plane(pid2).PhiZ(), slope2,
      TPC.Plane(output_plane).PhiZ()
      );
  } // ThirdPlaneSlope()
  
  
  double GeometryCore::ThirdPlaneSlope(
    geo::PlaneID const& pid1, double slope1,
    geo::PlaneID const& pid2, double slope2
  ) const {
    geo::PlaneID target_plane = ThirdPlane(pid1, pid2);
    return ThirdPlaneSlope(pid1, slope1, pid2, slope2, target_plane);
  } // ThirdPlaneSlope()
  
  
  double GeometryCore::ThirdPlane_dTdW(
    geo::PlaneID const& pid1, double slope1,
    geo::PlaneID const& pid2, double slope2,
    geo::PlaneID const& output_plane
  ) const {
    
    CheckIndependentPlanesOnSameTPC(pid1, pid2, "ThirdPlane_dTdW()");
    
    geo::TPCGeo const& TPC = this->TPC(pid1);
    
    double angle[3], pitch[3];
    geo::PlaneGeo const* const planes[3]
      = { &TPC.Plane(pid1), &TPC.Plane(pid2), &TPC.Plane(output_plane) };
    
    // We need wire pitch and "wire coordinate direction" for each plane.
    // The latter is perpendicular to the wire orientation.
    // PlaneGeo::PhiZ() defines the right orientation too.
    for (size_t i = 0; i < 3; ++i) {
      angle[i] = planes[i]->PhiZ();
      pitch[i] = planes[i]->WirePitch();
    } // for

    return ComputeThirdPlane_dTdW(
      angle[0], pitch[0], slope1,
      angle[1], pitch[1], slope2,
      angle[2], pitch[2]
      );
    
  } // GeometryCore::ThirdPlane_dTdW()
  

  double GeometryCore::ThirdPlane_dTdW(
    geo::PlaneID const& pid1, double slope1,
    geo::PlaneID const& pid2, double slope2
  ) const {
    geo::PlaneID target_plane = ThirdPlane(pid1, pid2);
    return ThirdPlane_dTdW(pid1, slope1, pid2, slope2, target_plane);
  } // ThirdPlane_dTdW()
  
  
  // Given slopes dTime/dWire in two planes, return with the slope in the 3rd plane.
  // Requires slopes to be in the same metrics,
  // e.g. converted in a distances ratio.
  // B. Baller August 2014
  // Rewritten by T. Yang Apr 2015 using the equation in H. Greenlee's talk:
  // https://cdcvs.fnal.gov/redmine/attachments/download/1821/larsoft_apr20_2011.pdf
  // slide 2
  double GeometryCore::ComputeThirdPlaneSlope
    (double angle1, double slope1, double angle2, double slope2, double angle3)
  {
    // note that, if needed, the trigonometric functions can be pre-calculated.
    
    // Can't resolve very small slopes
    if ((std::abs(slope1) < 0.001) && (std::abs(slope2)) < 0.001) return 0.001;
    
    // We need the "wire coordinate direction" for each plane.
    // This is perpendicular to the wire orientation. 
    double slope3 = 0.001;
    if (std::abs(slope1) > 0.001 && std::abs(slope2) > 0.001) {
      slope3
        = (
          + (1./slope1)*std::sin(angle3-angle2)
          - (1./slope2)*std::sin(angle3-angle1)
        ) / std::sin(angle1-angle2)
        ;
    }
    if (slope3 != 0.) slope3 = 1./slope3;
    else slope3 = 999.;
    
    return slope3;
  } // GeometryCore::ComputeThirdPlaneSlope()
  
  
  double GeometryCore::ComputeThirdPlane_dTdW(
    double angle1, double pitch1, double dTdW1,
    double angle2, double pitch2, double dTdW2,
    double angle_target, double pitch_target
      )
  {
    // we need to convert dt/dw into homogeneous coordinates, and then back;
    // slope = [dT * (TDCperiod / driftVelocity)] / [dW * wirePitch]
    // The coefficient of dT is assumed to be the same for all the planes,
    // and it finally cancels out. Pitches cancel out only if they are all
    // the same.
    return pitch_target * ComputeThirdPlaneSlope
      (angle1, dTdW1 / pitch1, angle2, dTdW2 / pitch2, angle_target);
  } // GeometryCore::ComputeThirdPlane_dTdW()
  
  
  //......................................................................
  // This function is called if it is determined that two wires in a single TPC must overlap.
  // To determine the yz coordinate of the wire intersection, we need to know the 
  // endpoints of both wires in xyz-space, and also their orientation (angle), and the 
  // inner dimensions of the TPC frame.
  // Note: This calculation is entirely dependent  on an accurate GDML description of the TPC!
  // Mitch - Feb., 2011
  // Changed to use WireIDsIntersect(). It does not check whether the intersection is on both wires (the same as the old behavior). T. Yang - Apr, 2015
  void GeometryCore::IntersectionPoint(geo::WireID const& wid1,
                                   geo::WireID const& wid2,
                                   double start_w1[3], 
                                   double end_w1[3], 
                                   double start_w2[3], 
                                   double end_w2[3], 
                                   double &y, double &z)
  {
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
  void GeometryCore::IntersectionPoint(geo::WireID const& wid1,
                                   geo::WireID const& wid2,
                                   double &y, double &z)
  {
    double WireStart1[3] = {0.};
    double WireStart2[3] = {0.};
    double WireEnd1[3]   = {0.};
    double WireEnd2[3]   = {0.};

    WireEndPoints(wid1, WireStart1, WireEnd1);
    WireEndPoints(wid2, WireStart2, WireEnd2);
    this->IntersectionPoint
      (wid1, wid2, WireStart1, WireEnd1, WireStart2, WireEnd2, y, z);
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
    geo::CryostatID cid;
    PositionToCryostat(xyz, cid);
    int o = Cryostat(cid).GetClosestOpDet(xyz);
    return OpDetFromCryo(o, cid.Cryostat);
  }
  
  
  //--------------------------------------------------------------------
  constexpr details::geometry_iterator_types::BeginPos_t
    details::geometry_iterator_types::begin_pos;
  constexpr details::geometry_iterator_types::EndPos_t
    details::geometry_iterator_types::end_pos;
  constexpr details::geometry_iterator_types::UndefinedPos_t
    details::geometry_iterator_types::undefined_pos;
  
  //--------------------------------------------------------------------
  //--- ROOTGeoNodeForwardIterator
  //---
  
  ROOTGeoNodeForwardIterator& ROOTGeoNodeForwardIterator::operator++ () {
    if (current_path.empty()) return *this;
    if (current_path.size() == 1) { current_path.pop_back(); return *this; }
    
    // I am done; all my descendants were also done already;
    // first look at my younger siblings
    NodeInfo_t& current = current_path.back();
    NodeInfo_t const& parent = current_path[current_path.size() - 2];
    if (++(current.sibling) < parent.self->GetNdaughters()) {
      // my next sibling exists, let's parse his descendents
      current.self = parent.self->GetDaughter(current.sibling);
      reach_deepest_descendant();
    }
    else current_path.pop_back(); // no sibling, it's time for mum
    return *this;
  } // ROOTGeoNodeForwardIterator::operator++
  
  
  //--------------------------------------------------------------------
  std::vector<TGeoNode const*> ROOTGeoNodeForwardIterator::get_path() const {
    
    std::vector<TGeoNode const*> node_path(current_path.size());
    
    std::transform(current_path.begin(), current_path.end(), node_path.begin(),
      [](NodeInfo_t const& node_info){ return node_info.self; });
    return node_path;
    
  } // ROOTGeoNodeForwardIterator::path()
  
  
  //--------------------------------------------------------------------
  void ROOTGeoNodeForwardIterator::reach_deepest_descendant() {
    Node_t descendent = current_path.back().self;
    while (descendent->GetNdaughters() > 0) {
      descendent = descendent->GetDaughter(0);
      current_path.emplace_back(descendent, 0U);
    } // while
  } // ROOTGeoNodeForwardIterator::reach_deepest_descendant()
  
  //--------------------------------------------------------------------
  void ROOTGeoNodeForwardIterator::init(TGeoNode const* start_node) {
    current_path.clear();
    if (!start_node) return;
    current_path.emplace_back(start_node, 0U);
    reach_deepest_descendant();
  } // ROOTGeoNodeForwardIterator::init()

  //--------------------------------------------------------------------
  
} // namespace geo
