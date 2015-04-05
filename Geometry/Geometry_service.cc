////////////////////////////////////////////////////////////////////////
/// \file Geometry.cxx
///
/// \version $Id: Geometry.cxx,v 1.19 2010/04/27 14:20:10 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

// C/C++ includes
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
}
#include <vector>
#include <algorithm>
#include <cstring>
#include <ostream> // std::endl
#include <iomanip>
#include <sstream>
#include <cstdlib> // for std::abort()
#include <limits> // std::numeric_limits<>

// lar includes
#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/Geometry.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "Geometry/OpDetGeo.h"
#include "Geometry/AuxDetGeo.h"
#include "SummaryData/RunData.h"
#include "Geometry/ChannelMapStandardAlg.h"
// #include "Geometry/ChannelMapAPAAlg.h"
// #include "Geometry/ChannelMap35Alg.h"

// ROOT includes
#include <TMath.h>
#include <TString.h>
#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TGeoMatrix.h>
#include <TGeoBBox.h>
#include <TVector3.h>
#include <Rtypes.h>

// Framework includes
#include "cetlib/exception.h"
#include "cetlib/search_path.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace geo {


  //......................................................................
  // Constructor.
  Geometry::Geometry(fhicl::ParameterSet const& pset, art::ActivityRegistry &reg)
    : fSurfaceY         (pset.get< double            >("SurfaceY"               ))
    , fDetectorName     (pset.get< std::string       >("Name"                   ))
    , fRelPath          (pset.get< std::string       >("RelativePath",     ""   ))
    , fMinWireZDist     (pset.get< double            >("MinWireZDist",     3.0  ))
    , fDisableWiresInG4 (pset.get< bool              >("DisableWiresInG4", false))
    , fForceUseFCLOnly  (pset.get< bool              >("ForceUseFCLOnly" , false))
    , fPositionWiggle   (pset.get< double            >("PositionEpsilon",  1.e-4))
    , fChannelMapAlg    (nullptr)
    , fSortingParameters(pset.get<fhicl::ParameterSet>("SortingParameters", fhicl::ParameterSet() ))
    , fExptGeoHelper(art::ServiceHandle<geo::ExptGeoHelperInterface>())
  {
    reg.sPreBeginRun.watch(this, &Geometry::preBeginRun);

    fDetectorName.ToLower();
    
    // Search all reasonable locations for the GDML file that contains
    // the detector geometry.
    // constructor decides if initialized value is a path or an environment variable
    cet::search_path sp("FW_SEARCH_PATH");

    std::string GDMLFileName(fRelPath);
    std::string ROOTFileName(fRelPath);

    ROOTFileName.append(pset.get< std::string >("GDML"));
    GDMLFileName.append(pset.get< std::string >("GDML"));

    if(fDisableWiresInG4) GDMLFileName.insert(GDMLFileName.find(".gdml"), "_nowires");
    
    if( !sp.find_file(GDMLFileName, fGDMLfile) )
      throw cet::exception("Geometry") << "cannot find the gdml geometry file: \n" 
				       << GDMLFileName
				       << "\n bail ungracefully.\n";

    if( !sp.find_file(ROOTFileName, fROOTfile) )
      throw cet::exception("Geometry") << "cannot find the root geometry file: \n" 
				       << ROOTFileName
				       << "\n bail ungracefully.\n";

    
    this->LoadGeometryFile(fGDMLfile, fROOTfile);

    return;
  }

  //......................................................................
  Geometry::~Geometry() 
  {
    for (unsigned int i=0; i<fCryostats.size(); ++i) {
      if (fCryostats[i]) { delete fCryostats[i]; fCryostats[i] = 0; }
    }
    for (unsigned int i=0; i<fAuxDets.size(); ++i) {
      if (fAuxDets[i]) { delete fAuxDets[i]; fAuxDets[i] = 0; }
    }
  }

  //......................................................................
  // 5.15.12 BJR: use the gdml file for both the fGDMLFile and fROOTFile
  // variables as ROOT v5.30.06 is once again able to read in gdml files
  // during batch operation, in this case think of fROOTFile meaning the
  // file used to make the ROOT TGeoManager.  I don't want to remove
  // the separate variables in case ROOT breaks again
  void Geometry::preBeginRun(art::Run const& run)
  {
    // check here to see if we need to load a new geometry.
    // get the detector id from the run object
    std::vector< art::Handle<sumdata::RunData> > rdcol;
    run.getManyByType(rdcol);
    if(rdcol.size() > 0 && !fForceUseFCLOnly){ 
      if(fDetectorName == rdcol[0]->DetName().c_str() ) return;
      
      std::string relpathgdml(fRelPath);
      std::string relpathroot(fRelPath);

      // check to see if the detector name in the RunData
      // object has not been set.  If that is the case, 
      // try the old DetId_t code
      std::string const nodetname("nodetectorname");
      if(rdcol[0]->DetName().compare(nodetname) == 0){
	LOG_WARNING("Geometry") << "Detector name not set: " << rdcol[0]->DetName()
				<< " use detector id: " << rdcol[0]->DetId()
				<< " This is expected behavior for legacy files" ;

	fDetId = rdcol[0]->DetId();
      
	switch(fDetId){
	case geo::kBo         : 
	  relpathgdml += "bo";         relpathroot += "bo";         fDetectorName = "bo";         break;
	case geo::kArgoNeuT   : 
	  relpathgdml += "argoneut";   relpathroot += "argoneut";   fDetectorName = "argoneut";   break;
	case geo::kLArIAT   : 
	  relpathgdml += "lariat";     relpathroot += "lariat.gdml";     fDetectorName = "lariat";     break;	
	case geo::kMicroBooNE : 
	  // We're going to tack on the suffix gdml later. Don't do it here too. Don't presume
	  // yet to do this for anyone but MicroBooNE, as falling back to the read-in geometry
	  // is a use case not everyone intends to perform, as we are doing now with CI tests.
	  relpathgdml += "microboone"; relpathroot += "microboone"; fDetectorName = "microboone"; break;
	case geo::kLBNE10kt   : 
	  relpathgdml += "lbne10kt";   relpathroot += "lbne10kt.gdml";   fDetectorName = "lbne10kt";   break;
	case geo::kLBNE34kt   : 
	  relpathgdml += "lbne34kt";   relpathroot += "lbne34kt.gdml";   fDetectorName = "lbne34kt";   break;
	case geo::kLBNE35t    : 
	  relpathgdml += "lbne35t";    relpathroot += "lbne35t.gdml";    fDetectorName = "lbne35t";    break;
	case geo::kJP250L     : 
	  relpathgdml += "jp250L";     relpathroot += "jp250L.gdml";     fDetectorName = "jp250L";     break;
	case geo::kCSU40L     : 
	  relpathgdml += "csu40l";     relpathroot += "csu40l.gdml";     fDetectorName = "csu40l";     break;
	case geo::kICARUS     : 
	  relpathgdml += "icarus";     relpathroot += "icarus.gdml";     fDetectorName = "icarus";     break;
	default               : 
	  throw cet::exception("LoadNewGeometry") << "detid invalid, " << fDetId << " give up\n";
	}
      }
      else{
	// the detector name is specified in the RunData object
	fDetectorName = rdcol[0]->DetName();
	relpathgdml += fDetectorName;
	relpathroot += fDetectorName;
      }
    
      if(fDisableWiresInG4) {
	relpathgdml+="_nowires.gdml";
	relpathroot+=".gdml";
      }
      else {
	relpathgdml+=".gdml";
	relpathroot+=".gdml";
      }

            
      // constructor decides if initialized value is a path or an environment variable
      cet::search_path sp("FW_SEARCH_PATH");
      
      if( !sp.find_file(relpathgdml, fGDMLfile) )
	throw cet::exception("Geometry") << "cannot find the gdml geometry file: \n" 
					 << relpathgdml
					 << "\n bail ungracefully.\n";
      
      if( !sp.find_file(relpathroot, fROOTfile) )
	throw cet::exception("Geometry") << "cannot find the root geometry file: \n" 
					 << relpathroot
					 << "\n bail ungracefully.\n";
      
      this->LoadGeometryFile(fGDMLfile, fROOTfile);
    }
    else 
      mf::LogWarning("LoadNewGeometry") << "cannot find sumdata::RunData object to grab detector name\n" 
					<< "this is expected if generating MC files\n"
					<< "using default geometry from configuration file\n";
    return;
  }

  //......................................................................
  void Geometry::InitializeChannelMap()
  {
    // if(fChannelMapAlg) delete fChannelMapAlg;
    // fChannelMapAlg = 0;

    fExptGeoHelper->ConfigureChannelMapAlg( fDetectorName, fSortingParameters, fCryostats, fAuxDets );
    
    fChannelMapAlg = fExptGeoHelper->GetChannelMapAlg();
    if ( ! fChannelMapAlg ) {
      throw cet::exception("ChannelMapLoadFail") << " failed to load new channel map";
    }

    return;
  }

  //......................................................................
  void Geometry::LoadGeometryFile(std::string gdmlfile, std::string rootfile)
  {
  
    struct stat sb;
    if (gdmlfile.empty() || stat(gdmlfile.c_str(), &sb)!=0)
      // Failed to resolve the file name
      throw cet::exception("Geometry") << "No GDML Geometry file " << gdmlfile << " found!\n";

    if (rootfile.empty() || stat(rootfile.c_str(), &sb)!=0)
      // Failed to resolve the file name
      throw cet::exception("Geometry") << "No ROOT Geometry file " << rootfile << " found!\n";
 
    // clear the Cryostat array
    for (size_t i = 0; i < fCryostats.size(); ++i) {
      if (fCryostats[i]) { delete fCryostats[i]; fCryostats[i] = 0; }
    }
    fCryostats.clear();
    
    // clear the AuxDet array
    for (size_t i = 0; i < fAuxDets.size(); ++i) {
      if (fAuxDets[i]) { delete fAuxDets[i]; fAuxDets[i] = 0; }
    }
    fAuxDets.clear();

    // Open the GDML file, and convert it into ROOT TGeoManager
    // format.
    TGeoManager::Import(rootfile.c_str());

    std::vector<const TGeoNode*> path(8);
    path[0] = gGeoManager->GetTopNode();
    this->FindCryostat(path, 0);
    this->FindAuxDet(path, 0);

    // sort volume objects and map channels accordingly
    this->InitializeChannelMap();

    mf::LogWarning("LoadNewGeometry") << "New detector geometry loaded from "   
				      << "\n\t" << fROOTfile 
				      << "\n\t" << fGDMLfile << std::endl;
    
  }

  //......................................................................
  TGeoManager* Geometry::ROOTGeoManager() const
  {
    return gGeoManager;
  }
  
  //......................................................................
  unsigned int Geometry::Nchannels() const
  {
    return fChannelMapAlg->Nchannels();
  }

  //......................................................................
  unsigned int Geometry::NTPC(unsigned int cstat) const
  {
    return this->Cryostat(cstat).NTPC();
  }

  //......................................................................
  unsigned int Geometry::NOpDet(unsigned int cstat) const
  {
    return this->Cryostat(cstat).NOpDet();
  }

  //......................................................................
  unsigned int Geometry::NOpDets() const
  {
    int N=0;
    for(size_t cstat=0; cstat!=Ncryostats(); cstat++)
      N += this->Cryostat(cstat).NOpDet();
    return N;
  }

  //......................................................................
  unsigned int Geometry::NOpChannels() const
  {
    return fChannelMapAlg->NOpChannels(this->NOpDets());
  }

  //......................................................................
  unsigned int Geometry::NHardwareChannels(int opDet) const
  {
    return fChannelMapAlg->NHardwareChannels(opDet);
  }

  //......................................................................
  unsigned int Geometry::OpChannel(int detNum, int hardwareChannel) const
  {
    return fChannelMapAlg->OpChannel(detNum, hardwareChannel);
  }

  //......................................................................
  unsigned int Geometry::OpDetFromOpChannel(int opChannel) const
  {
    return fChannelMapAlg->OpDetFromOpChannel(opChannel);
  }

  //......................................................................
  unsigned int Geometry::HardwareChannelFromOpChannel(int opChannel) const
  {
    return fChannelMapAlg->HardwareChannelFromOpChannel(opChannel);
  }

  //......................................................................
  unsigned int Geometry::Nplanes(unsigned int tpc,
				 unsigned int cstat) const
  {
    return this->Cryostat(cstat).TPC(tpc).Nplanes();
  }

  //......................................................................
  unsigned int Geometry::Nwires(unsigned int p, 
				unsigned int tpc,
				unsigned int cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc).Plane(p).Nwires();
  }

  //......................................................................
  // Number of different views, or wire orientations
  // The function assumes that all TPCs in all cryostats of
  // a detector have the same number of planes, which should be 
  // a safe assumption
  unsigned int Geometry::Nviews() const
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
  const CryostatGeo& Geometry::Cryostat(unsigned int const cstat) const 
  {
    if(cstat >= fCryostats.size()) 
      throw cet::exception("Geometry") << "Cryostat " 
				       << cstat 
				       << " does not exist\n";

    return *fCryostats[cstat];
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
  const AuxDetGeo& Geometry::AuxDet(unsigned int const ad) const
  {
    if(ad >= fAuxDets.size())
    throw cet::exception("Geometry") << "AuxDet "
    << ad
    << " does not exist\n";
    
    return *fAuxDets[ad];
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
  const TPCGeo& Geometry::TPC(unsigned int const tpc,
			      unsigned int const cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc);
  }

  
  //......................................................................
  geo::TPCID Geometry::FindTPCAtPosition(double const worldLoc[3]) const {
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
  } // Geometry::FindTPCAtPosition()
  
  
  //......................................................................
  const TPCGeo& Geometry::PositionToTPC(double const  worldLoc[3],
					unsigned int &tpc,
					unsigned int &cstat) const
  {
    return this->PositionToCryostat(worldLoc,cstat).PositionToTPC(worldLoc,tpc, 1.+fPositionWiggle);
  }

  //......................................................................
  unsigned int Geometry::FindCryostatAtPosition(double const worldLoc[3]) const
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
  } // Geometry::FindCryostatAtPosition()

  //......................................................................
  const CryostatGeo& Geometry::PositionToCryostat(double const  worldLoc[3],
						  unsigned int &cstat) const
  {
    cstat = FindCryostatAtPosition(worldLoc);
    
    if(cstat == UINT_MAX)
      throw cet::exception("Geometry") << "Can't find Cryostat for position (" 
				       << worldLoc[0] << ","
				       << worldLoc[1] << "," 
				       << worldLoc[2] << ")\n";
			
    return this->Cryostat(cstat);
  }
    
  
  //......................................................................
  unsigned int Geometry::FindAuxDetAtPosition(double const  worldPos[3]) const
  {

    double local[3] = {0.};
    for(unsigned int a = 0; a < this->NAuxDets(); ++a) {

      this->AuxDet(a).WorldToLocal(worldPos, local);      
      double HalfCenterWidth = (this->AuxDet(a).HalfWidth1() + this->AuxDet(a).HalfWidth2()) / 2;

      if( local[2] >= - this->AuxDet(a).Length()/2       &&
	  local[2] <=   this->AuxDet(a).Length()/2       &&
	  local[1] >= - this->AuxDet(a).HalfHeight()     &&
	  local[1] <=   this->AuxDet(a).HalfHeight()     &&
	  // if AuxDet a is a box, then HalfSmallWidth = HalfWidth
	  local[0] >= - HalfCenterWidth + local[2]*(HalfCenterWidth-this->AuxDet(a).HalfWidth2())/(this->AuxDet(a).Length()/2) &&
	  local[0] <=   HalfCenterWidth - local[2]*(HalfCenterWidth-this->AuxDet(a).HalfWidth2())/(this->AuxDet(a).Length()/2)
        )  return a;

    }// for loop over AudDet a

    return UINT_MAX;
  } // Geometry::FindAuxDetAtPosition()
  

  
  //......................................................................
  const AuxDetGeo& Geometry::PositionToAuxDet(double const  worldLoc[3],
                                              unsigned int &ad) const
  {
    
    // locate the desired Auxiliary Detector
    ad = FindAuxDetAtPosition(worldLoc);
    if(ad == UINT_MAX)
    throw cet::exception("Geometry") << "Can't find AuxDet for position ("
      << worldLoc[0] << ","
      << worldLoc[1] << ","
      << worldLoc[2] << ")\n";
    
    return this->AuxDet(ad);
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
  const PlaneGeo& Geometry::Plane(unsigned int const p, 
				  unsigned int const tpc,
				  unsigned int const cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc).Plane(p);
  }

  //......................................................................
  SigType_t Geometry::SignalType(raw::ChannelID_t const channel) const
  {
    return fChannelMapAlg->SignalType(channel);
  }

  //......................................................................
  SigType_t Geometry::SignalType(geo::PlaneID const pid) const
  {
    return this->Cryostat(pid.Cryostat).TPC(pid.TPC).Plane(pid.Plane).SignalType();
  }


  //......................................................................
  View_t Geometry::View(raw::ChannelID_t const channel) const
  {
    return fChannelMapAlg->View(channel);
  }

  //......................................................................
  View_t Geometry::View(geo::PlaneID const pid) const
  {
    return this->Cryostat(pid.Cryostat).TPC(pid.TPC).Plane(pid.Plane).View();
  }

  //......................................................................
  std::set<View_t> const& Geometry::Views() const
  {
    return fChannelMapAlg->Views();
  }

  //......................................................................
  std::set<PlaneID> const& Geometry::PlaneIDs() const
  {
    return fChannelMapAlg->PlaneIDs();
  }

  //......................................................................
  const std::string Geometry::GetWorldVolumeName() const
  {
    // For now, and possibly forever, this is a constant (given the
    // definition of "nodeNames" above).
    return std::string("volWorld");
  }

  //......................................................................
  const std::string Geometry::GetLArTPCVolumeName(unsigned int const tpc,
						  unsigned int const cstat) const
  {

    return std::string(this->Cryostat(cstat).TPC(tpc).ActiveVolume()->GetName()); 
  }

  //......................................................................
  const std::string Geometry::GetCryostatVolumeName(unsigned int const cstat) const
  {
    return this->Cryostat(cstat).Volume()->GetName();
  }

  //......................................................................
  double Geometry::DetHalfWidth(unsigned int tpc,
				unsigned int cstat)  const 
  {
    return this->Cryostat(cstat).TPC(tpc).ActiveHalfWidth();
  }

  //......................................................................
  double Geometry::DetHalfHeight(unsigned int tpc,
				 unsigned int cstat) const 
  {
    return this->Cryostat(cstat).TPC(tpc).ActiveHalfHeight();
  }

  //......................................................................
  double Geometry::DetLength(unsigned int tpc,
			     unsigned int cstat) const
  { 
    return this->Cryostat(cstat).TPC(tpc).ActiveLength();
  }

  //......................................................................
  double Geometry::CryostatHalfWidth(unsigned int cstat) const
  {
    return this->Cryostat(cstat).HalfWidth();
  }

  //......................................................................
  double Geometry::CryostatHalfHeight(unsigned int cstat) const
  {
    return this->Cryostat(cstat).HalfHeight();
  }

  //......................................................................
  double Geometry::CryostatLength(unsigned int cstat) const
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
  void Geometry::CryostatBoundaries(double* boundaries,
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
  double Geometry::PlanePitch(unsigned int p1, 
			      unsigned int p2, 
			      unsigned int tpc,
			      unsigned int cstat) const
  { 
    return this->Cryostat(cstat).TPC(tpc).PlanePitch(p1, p2);
  }
      
  //......................................................................
  // This method returns the distance between the specified wires.
  // w1 < w2.  The wires are assumed to be on the same plane
  double Geometry::WirePitch(unsigned int w1,  
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
  double Geometry::WirePitch(geo::View_t view) const
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
  double Geometry::WireAngleToVertical(geo::View_t view, int TPC, int Cryo) const
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
  void Geometry::WorldBox(double* xlo, double* xhi,
			  double* ylo, double* yhi,
			  double* zlo, double* zhi) const
  {
    const TGeoShape* s = gGeoManager->GetVolume("volWorld")->GetShape();
    if(!s)
      throw cet::exception("Geometry") << "no pointer to world volume TGeoShape\n";

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
  const TVector3 Geometry::GetTPCFrontFaceCenter(unsigned int tpc,
  						 unsigned int cstat) const
  {
    return TVector3( 0.5 * this->DetHalfWidth(tpc, cstat), 0 , 0 );
  }

  //......................................................................
  const std::string Geometry::VolumeName(TVector3 point)
  {
    // check that the given point is in the World volume at least
    TGeoVolume *volWorld = gGeoManager->FindVolumeFast(this->GetWorldVolumeName().c_str());
    double halflength = ((TGeoBBox*)volWorld->GetShape())->GetDZ();
    double halfheight = ((TGeoBBox*)volWorld->GetShape())->GetDY();
    double halfwidth  = ((TGeoBBox*)volWorld->GetShape())->GetDX();
    if(TMath::Abs(point.x()) > halfwidth  ||
       TMath::Abs(point.y()) > halfheight ||
       TMath::Abs(point.z()) > halflength
       ){
      mf::LogWarning("GeometryBadInputPoint") << "point (" << point.x() << ","
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
  const std::string Geometry::MaterialName(TVector3 point)
  {
    // check that the given point is in the World volume at least
    TGeoVolume *volWorld = gGeoManager->FindVolumeFast(this->GetWorldVolumeName().c_str());
    double halflength = ((TGeoBBox*)volWorld->GetShape())->GetDZ();
    double halfheight = ((TGeoBBox*)volWorld->GetShape())->GetDY();
    double halfwidth  = ((TGeoBBox*)volWorld->GetShape())->GetDX();
    if(TMath::Abs(point.x()) > halfwidth  ||
       TMath::Abs(point.y()) > halfheight ||
       TMath::Abs(point.z()) > halflength
       ){ 
      mf::LogWarning("GeometryBadInputPoint") << "point (" << point.x() << ","
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
  void Geometry::FindCryostat(std::vector<const TGeoNode*>& path,
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
      throw cet::exception("Geometry") << "exceeded maximum TGeoNode depth\n";
    }

    const TGeoVolume *v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for(int i = 0; i < nd; ++i){
      path[deeper] = v->GetNode(i);
      this->FindCryostat(path, deeper);
    }

  }

  //......................................................................
  void Geometry::MakeCryostat(std::vector<const TGeoNode*>& path, int depth) 
  {
    fCryostats.push_back(new CryostatGeo(path, depth));
  }

  //......................................................................
  void Geometry::FindAuxDet(std::vector<const TGeoNode*>& path,
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
      throw cet::exception("Geometry") << "exceeded maximum TGeoNode depth\n";
    }
    
    const TGeoVolume *v = path[depth]->GetVolume();
    int nd = v->GetNdaughters();
    for(int i = 0; i < nd; ++i){
      path[deeper] = v->GetNode(i);
      this->FindAuxDet(path, deeper);
    }
    
  }
  
  //......................................................................
  void Geometry::MakeAuxDet(std::vector<const TGeoNode*>& path, int depth)
  {
    fAuxDets.push_back(new AuxDetGeo(path, depth));
  }

  //......................................................................
  //
  // Return the total mass of the detector
  //
  //
  double Geometry::TotalMass(const char *vol) const
  {
    //the TGeoNode::GetVolume() returns the TGeoVolume of the detector outline
    //and ROOT calculates the mass in kg for you
    TGeoVolume *gvol = gGeoManager->FindVolumeFast(vol);
    if(gvol) return gvol->Weight();

    throw cet::exception("Geometry") << "could not find specified volume " 
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
  double Geometry::MassBetweenPoints(double *p1, double *p2) const
  {

    //The purpose of this method is to determine the column density
    //between the two points given.  Do that by starting at p1 and 
    //stepping until you get to the node of p2.  calculate the distance
    //between the point just inside that node and p2 to get the last
    //bit of column density
    double columnD = 0.;

    //first initialize a track - get the direction cosines
    double length = TMath::Sqrt(TMath::Power(p2[0]-p1[0], 2.)
				+ TMath::Power(p2[1]-p1[1], 2.)
				+ TMath::Power(p2[2]-p1[2], 2.));
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
    length = TMath::Sqrt(TMath::Power(p2[0]-current[0], 2.)
			 + TMath::Power(p2[1]-current[1], 2.)
			 + TMath::Power(p2[2]-current[2], 2.));
    columnD += length*node->GetMedium()->GetMaterial()->GetDensity();

    return columnD;
  }

  //......................................................................
  std::vector< geo::WireID > Geometry::ChannelToWire( raw::ChannelID_t channel ) const
  {
    return fChannelMapAlg->ChannelToWire(channel);
  }

  //----------------------------------------------------------------------------
  double Geometry::WireCoordinate(double YPos, double ZPos,
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
  unsigned int Geometry::NearestWire(const TVector3& worldPos, 
				     unsigned int const PlaneNo, 
				     unsigned int const TPCNo,
				     unsigned int const cstat) const
  {
    return fChannelMapAlg->NearestWire(worldPos, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  unsigned int Geometry::NearestWire(const double worldPos[3], 
				     unsigned int const PlaneNo, 
				     unsigned int const TPCNo,
				     unsigned int const cstat) const
  {
    TVector3 wp(worldPos);
    return this->NearestWire(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  unsigned int Geometry::NearestWire(std::vector<double> const worldPos, 
				     unsigned int const PlaneNo, 
				     unsigned int const TPCNo,
				     unsigned int const cstat) const
  {
    if(worldPos.size() > 3) throw cet::exception("Geometry") << "bad size vector for "
							     << "worldPos: " 
							     << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return this->NearestWire(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  const geo::WireID Geometry::NearestWireID(const TVector3& worldPos, 
					    unsigned int const PlaneNo, 
					    unsigned int const TPCNo,
					    unsigned int const cstat) const
  {
    return fChannelMapAlg->NearestWireID(worldPos,PlaneNo,TPCNo,cstat);
  }

  //----------------------------------------------------------------------------
  const geo::WireID Geometry::NearestWireID(std::vector<double> worldPos, 
					    unsigned int const  PlaneNo, 
					    unsigned int const  TPCNo,
					    unsigned int const  cstat) const
  {
    if(worldPos.size() > 3) throw cet::exception("Geometry") << "bad size vector for "
							     << "worldPos: " 
							     << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return this->NearestWireID(wp,PlaneNo,TPCNo,cstat);
  }

  //----------------------------------------------------------------------------
  const geo::WireID Geometry::NearestWireID(const double        worldPos[3], 
					    unsigned int const  PlaneNo, 
					    unsigned int const  TPCNo,
					    unsigned int const  cstat) const
  {
    TVector3 wp(worldPos);
    return this->NearestWireID(wp,PlaneNo,TPCNo,cstat);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t Geometry::NearestChannel(const double worldPos[3], 
				    unsigned int const PlaneNo, 
				    unsigned int const TPCNo,
				    unsigned int const cstat) const
  {
    TVector3 wp(worldPos);
    return this->NearestChannel(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t Geometry::NearestChannel(std::vector<double> const worldPos, 
				    unsigned int const PlaneNo, 
				    unsigned int const TPCNo,
				    unsigned int const cstat) const
  {
    if(worldPos.size() > 3) throw cet::exception("Geometry") << "bad size vector for "
							     << "worldPos: " 
							     << worldPos.size() << "\n";
    TVector3 wp(&(worldPos[0]));
    return this->NearestChannel(wp, PlaneNo, TPCNo, cstat);
  }

  //----------------------------------------------------------------------------
  raw::ChannelID_t Geometry::NearestChannel(const TVector3& worldPos, 
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
  raw::ChannelID_t Geometry::PlaneWireToChannel(unsigned int const plane,
					unsigned int const wire,
					unsigned int const tpc,
					unsigned int const cstat) const
  {
    return fChannelMapAlg->PlaneWireToChannel(plane, wire, tpc, cstat);
  }

  //......................................................................
  raw::ChannelID_t Geometry::PlaneWireToChannel(WireID const& wireid) const
  {
    return this->PlaneWireToChannel(wireid.Plane, wireid.Wire, wireid.TPC, wireid.Cryostat);   
  }

  // Functions to allow determination if two wires intersect, and if so where.
  // This is useful information during 3D reconstruction.
  //......................................................................
  bool Geometry::ValueInRange(double value, double min, double max) const
  {
    if(min>max) std::swap(min,max);//protect against funny business due to wire angles
    if (std::abs(value-min)<1e-6||std::abs(value-max)<1e-6) return true;
    return (value>=min) && (value<=max);
  }

  //......................................................................
  void Geometry::WireEndPoints(unsigned int cstat,
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
   
  //......................................................................
  bool Geometry::ChannelsIntersect(raw::ChannelID_t c1, 
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

    unsigned int cs1, tpc1, plane1, wire1;
    unsigned int cs2, tpc2, plane2, wire2;

    cs1 = chan1wires[0].Cryostat;
    tpc1 = chan1wires[0].TPC;
    plane1 = chan1wires[0].Plane;
    wire1 = chan1wires[0].Wire;

    cs2 = chan2wires[0].Cryostat;
    tpc2 = chan2wires[0].TPC;
    plane2 = chan2wires[0].Plane;
    wire2 = chan2wires[0].Wire;

    if( cs1 != cs2 || tpc1 != tpc2 ) {
      mf::LogWarning("ChannelsIntersect") << "attempting to find intersection between wires"
					  << " from different TPCs or Cryostats, return false";
      return false;
    }

    double wire1_Start[3] = {0.};
    double wire1_End[3]   = {0.};
    double wire2_Start[3] = {0.};
    double wire2_End[3]   = {0.};
	
    this->WireEndPoints(cs1, tpc1, plane1, wire1, wire1_Start, wire1_End);
    this->WireEndPoints(cs2, tpc1, plane2, wire2, wire2_Start, wire2_End);

    if(plane1 == plane2){
      mf::LogWarning("ChannelsIntersect") << "You are comparing two wires in the same plane!";
      return false;
    }

    // if endpoint of one input wire is within range of other input wire in 
    // BOTH y AND z, wires overlap 
    bool overlapY = this->ValueInRange(wire1_Start[1], wire2_Start[1], wire2_End[1]) ||
      this->ValueInRange(wire1_End[1], wire2_Start[1], wire2_End[1]);
    
    bool overlapZ = this->ValueInRange(wire1_Start[2], wire2_Start[2], wire2_End[2]) ||
      this->ValueInRange(wire1_End[2], wire2_Start[2], wire2_End[2]);
    
    // reverse ordering of wires...this is necessitated for now due to precision 
    // of placement of wires 
    bool overlapY_reverse = this->ValueInRange(wire2_Start[1], wire1_Start[1], wire1_End[1]) ||
      this->ValueInRange(wire2_End[1], wire1_Start[1], wire1_End[1]);
    
    bool overlapZ_reverse = this->ValueInRange(wire2_Start[2], wire1_Start[2], wire1_End[2]) ||
      this->ValueInRange(wire2_End[2], wire1_Start[2], wire1_End[2]);
 
    // override y overlap checks if a vertical plane exists:
    if( this->Cryostat(cs1).TPC(tpc1).Plane(plane1).Wire(wire1).isVertical() || 
	this->Cryostat(cs2).TPC(tpc1).Plane(plane2).Wire(wire2).isVertical() ){
      overlapY         = true;	
      overlapY_reverse = true;
    }

    //catch to get vertical wires, where the standard overlap might not work, Andrzej
    if(std::abs(wire2_Start[2] - wire2_End[2]) < 0.01) overlapZ = overlapZ_reverse;

    if(overlapY && overlapZ){
      this->IntersectionPoint(wire1, wire2, 
			      plane1, plane2, 
			      cs1, tpc1,
			      wire1_Start, wire1_End, 
			      wire2_Start, wire2_End, 
			      y, z);
      return true;
    }
    else if(overlapY_reverse && overlapZ_reverse){
      this->IntersectionPoint(wire2, wire1, 
			      plane2, plane1, 
			      cs1, tpc1,
			      wire2_Start, wire2_End, 
			      wire1_Start, wire1_End, 
			      y, z);
      return true;
    }
    
    return false;    
  }


  //......................................................................
  bool Geometry::WireIDsIntersect(const geo::WireID& wid1, const geo::WireID& wid2, 
				   geo::WireIDIntersection & widIntersect   ) const
  {

    double w1_Start[3] = {0.};
    double w1_End[3]   = {0.};
    double w2_Start[3] = {0.};
    double w2_End[3]   = {0.};

    if( wid1.Plane == wid2.Plane ){
      mf::LogWarning("APAChannelsIntersect") << "Comparing two wires in the same plane, return false";
      return false;     }
    if( wid1.TPC != wid2.TPC ){
      mf::LogWarning("APAChannelsIntersect") << "Comparing two wires in different TPCs, return false";
      return false;     }
    if( wid1.Cryostat != wid2.Cryostat ){
      mf::LogWarning("APAChannelsIntersect") << "Comparing two wires in different Cryostats, return false";
      return false;     }

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

    if (this->ValueInRange(x,x1,x2) &&
	this->ValueInRange(x,x3,x4) &&
	this->ValueInRange(y,y1,y2) &&
	this->ValueInRange(y,y3,y4)){
      widIntersect.y = x;
      widIntersect.z = y;
      widIntersect.TPC = wid1.TPC;
      return true;
    }
    else{
      return false;
    }

    return false;
  }

  // Given slopes dTime/dWire in two planes, return with the slope in the 3rd plane.
  // B. Baller August 2014
  double Geometry::ThirdPlaneSlope(unsigned int plane1, double slope1, 
                                   unsigned int plane2, double slope2, 
                                   unsigned int tpc, unsigned int cstat)
  
  {

    if(Nplanes(tpc,cstat) != 3) return 999;
    if(plane1 > 2 || plane2 > 2) return 999;
    
    // Can't resolve very small slopes
    if(fabs(slope1) < 0.001 && fabs(slope2) < 0.001) return 0.001;

    // Calculate static variables on the first call
    // Cosines are needed for later calls. Sines are not.
    static bool first = true;
    static double c0, c1, c2;
    static double d01, d12, d20;
    if(first) {
      first = false;
      double angle0 = this->Cryostat(cstat).TPC(tpc).Plane(0).Wire(0).ThetaZ();
      double angle1 = this->Cryostat(cstat).TPC(tpc).Plane(1).Wire(0).ThetaZ();
      double angle2 = this->Cryostat(cstat).TPC(tpc).Plane(2).Wire(0).ThetaZ();
      // We need the "wire coordinate direction" for each plane. This is perpendicular
      // to the wire orientation. 
              c0 = TMath::Cos(angle0 - M_PI/2);
      double  s0 = TMath::Sin(angle0 - M_PI/2);
              c1 = TMath::Cos(angle1 - M_PI/2);
      double  s1 = TMath::Sin(angle1 - M_PI/2);
              c2 = TMath::Cos(angle2 - M_PI/2);
      double  s2 = TMath::Sin(angle2 - M_PI/2);
      // "Denominator" variables
      d01 = 1 / (s0 * c1 - s1 * c0);
      d12 = 1 / (s1 * c2 - s2 * c1);
      d20 = 1 / (s2 * c0 - s0 * c2);
    } // first
    
    unsigned int lopln = plane1;
    unsigned int hipln = plane2;
    double loplnslp = slope1;
    double hiplnslp = slope2;
    // re-order if the user didn't pass it in the expected order
    if(plane1 > plane2) {
      lopln = plane2;
      hipln = plane1;
      loplnslp = slope2;
      hiplnslp = slope1;
    }

    double slope3 = 0;
    double rfact = 0;

    // Three cases
    if(lopln == 0 && hipln == 1) {
      double r01 = hiplnslp / loplnslp;
      rfact = (d12 * c2 + d01 * c0 - d01 * r01 * c1) / (d12 * c1);
      slope3 = hiplnslp / rfact;
    } else if(lopln == 1 && hipln == 2) {
      double r12 = hiplnslp / loplnslp;
      rfact = (d20 * c0 + d12 * c1 - d12 * r12 * c2) / (d20 * c2);
      slope3 = hiplnslp / rfact;
    } else {
      double r20 = loplnslp / hiplnslp;
      rfact = (d01 * c1 + d20 * c2 - d20 * r20 * c0) / (d01 * c0);
      slope3 = loplnslp / rfact;
    }

    return slope3;

  } // ThirdPlaneSlope

   
  //......................................................................
  // This function is called if it is determined that two wires in a single TPC must overlap.
  // To determine the yz coordinate of the wire intersection, we need to know the 
  // endpoints of both wires in xyz-space, and also their orientation (angle), and the 
  // inner dimensions of the TPC frame.
  // Note: This calculation is entirely dependent  on an accurate GDML description of the TPC!
  // Mitch - Feb., 2011
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

    //angle of wire1 wrt z-axis in Y-Z plane...in radians
    double angle1 = this->Cryostat(cstat).TPC(tpc).Plane(plane1).Wire(wire1).ThetaZ();
    //angle of wire2 wrt z-axis in Y-Z plane...in radians
    double angle2 = this->Cryostat(cstat).TPC(tpc).Plane(plane2).Wire(wire2).ThetaZ();
    
    if(angle1 == angle2) return;//comparing two wires in the same plane...pointless.

    //coordinates of "upper" endpoints...(z1,y1) = (a,b) and (z2,y2) = (c,d) 
    double a = 0.;
    double b = 0.;
    double c = 0.; 
    double d = 0.;
    double angle = 0.;
    double anglex = 0.;
    
    // below is a special case of calculation when one of the planes is vertical. 
    angle1 < angle2 ? angle = angle1 : angle = angle2;//get angle closest to the z-axis
    
    // special case, one plane is vertical
    if(angle1 == M_PI/2 || angle2 == M_PI/2){
      if(angle1 == M_PI/2){
		
	anglex = (angle2-M_PI/2);
	a = end_w1[2];
	b = end_w1[1];
	c = end_w2[2];
	d = end_w2[1];
	// the if below can in principle be replaced by the sign of anglex (inverted) 
	// in the formula for y below. But until the geometry is fully symmetric in y I'm 
	// leaving it like this. Andrzej
	if((anglex) > 0 ) b = start_w1[1];
		    
      }
      else if(angle2 == M_PI/2){
	anglex = (angle1-M_PI/2);
	a = end_w2[2];
	b = end_w2[1];
	c = end_w1[2];
	d = end_w1[1];
	// the if below can in principle be replaced by the sign of anglex (inverted) 
	// in the formula for y below. But until the geometry is fully symmetric in y I'm 
	// leaving it like this. Andrzej
	if((anglex) > 0 ) b = start_w2[1];  
      }

      y = b + ((c-a) - (b-d)*tan(anglex))/tan(anglex);
      z = a;   // z is defined by the wire in the vertical plane
      
      return;
    }

    // end of vertical case
   
    z = 0;y = 0;
                                                                      
    if(angle1 < (TMath::Pi()/2.0)){
      c = end_w1[2];
      d = end_w1[1];
      a = start_w2[2];
      b = start_w2[1];
    }
    else{
      c = end_w2[2];
      d = end_w2[1];
      a = start_w1[2];
      b = start_w1[1];
    }
    
    //Intersection point of two wires in the yz plane is completely
    //determined by wire endpoints and angle of inclination.
    z = 0.5 * ( c + a + (b-d)/TMath::Tan(angle) );
    y = 0.5 * ( b + d + (a-c)*TMath::Tan(angle) );
    
    return;

  }
    
  // Added shorthand function where start and endpoints are looked up automatically
  //  - whether to use this or the full function depends on optimization of your
  //    particular algorithm.  Ben J, Oct 2011
  //--------------------------------------------------------------------
  void Geometry::IntersectionPoint(unsigned int wire1, 
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
  std::string Geometry::OpDetGeoName(unsigned int c) const
  {
    return Cryostat(c).OpDetGeoName();
  }

  //--------------------------------------------------------------------
  // Convert OpDet, Cryo into OpChannel
  int Geometry::OpDetCryoToOpChannel(unsigned int o, unsigned int c ) const
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
  void  Geometry::OpChannelToCryoOpDet(unsigned int OpChannel, 
				       unsigned int& o, 
				       unsigned int& c) const
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
      LowestID.at(0) = 0;	
      for(size_t cryo = 0; cryo != NCryo; ++cryo){
	LowestID.at(cryo+1)=LowestID.at(cryo)+Cryostat(c).NOpDet();
      }
	
    }

    for(size_t i=0; i!=NCryo; ++i){
      if( ( OpChannel >= LowestID.at(i) ) && (OpChannel < LowestID.at(i+1))){
	c = i; o = OpChannel-LowestID.at(i); 
	return;
      }
    }
    // If we made it here, we didn't find the right combination. abort
    throw cet::exception("OpID To OpDetCryo error")<<"OpID out of range, "<< OpChannel << "\n";

    return;
  }
  
  
  //--------------------------------------------------------------------
  // Find the closest OpChannel to this point, in the appropriate cryostat  
  unsigned int Geometry::GetClosestOpChannel(double * xyz) const
  {	
    unsigned int c;
    PositionToCryostat(xyz, c);
    return OpDetCryoToOpChannel(Cryostat(c).GetClosestOpDet(xyz), c);    
  }
  
  //--------------------------------------------------------------------
  const WireGeo& Geometry::WireIDToWireGeo(geo::WireID CodeWire) const
  {
    unsigned int cryo  = CodeWire.Cryostat;
    unsigned int tpc   = CodeWire.TPC;
    unsigned int plane = CodeWire.Plane;
    unsigned int wire  = CodeWire.Wire;
    
    return this->Cryostat(cryo).TPC(tpc).Plane(plane).Wire(wire);
  }
  
  
  //--------------------------------------------------------------------
  Geometry::cryostat_iterator& Geometry::cryostat_iterator::operator++() {
    if (!isValid) return *this;
    if (++cryoid < limits) return *this;
    isValid = false;
    return *this;
  } // Geometry::cryostat_iterator::operator++()
  
  
  Geometry::cryostat_iterator& Geometry::cryostat_iterator::operator--() {
    if (!isValid) return *this;
    if (cryoid-- >= 0) return *this;
    isValid = false;
    return *this;
  } // Geometry::cryostat_iterator::operator--()
  
  
  const CryostatGeo* Geometry::cryostat_iterator::get() const
    { return isValid? &(pGeo->Cryostat(cryoid)): nullptr; }
  
  
  void Geometry::cryostat_iterator::init_geometry()
    { pGeo = &*(art::ServiceHandle<Geometry>()); }
  
  
  void Geometry::cryostat_iterator::set_limits_and_validity() {
    limits = pGeo->Ncryostats();
    isValid = (cryoid < limits);
  } // Geometry::cryostat_iterator::set_limits_and_validity()
  
  
  //--------------------------------------------------------------------
  Geometry::TPC_iterator& Geometry::TPC_iterator::operator++() {
    if (!tpcid.isValid) return *this;
    
    ++tpcid.TPC;
    while (true) {
      if (tpcid.TPC < limits.TPC) return *this;
      if (++tpcid.Cryostat >= limits.Cryostat) break;
      new_cryostat();
    } // while
    tpcid.isValid = false;
    return *this;
  } // Geometry::TPC_iterator::operator++()
  
  
  const TPCGeo* Geometry::TPC_iterator::get() const
    { return tpcid.isValid? &(pGeo->TPC(tpcid.TPC, tpcid.Cryostat)): nullptr; }
  
  
  const CryostatGeo* Geometry::TPC_iterator::getCryostat() const {
    return tpcid.isValid? &(pGeo->Cryostat(tpcid.Cryostat)): nullptr;
  } // Geometry::TPC_iterator::getCryostat()
  
  
  void Geometry::TPC_iterator::init_geometry()
    { pGeo = &*(art::ServiceHandle<Geometry>()); }
  
  
  void Geometry::TPC_iterator::set_limits_and_validity() {
    tpcid.isValid = false;
    limits.Cryostat = pGeo->Ncryostats();
    if (tpcid.Cryostat >= limits.Cryostat) return;
    limits.TPC = pGeo->NTPC(tpcid.Cryostat);
    if (tpcid.TPC >= limits.TPC) return;
    tpcid.isValid = true;
  } // Geometry::TPC_iterator::set_limits_and_validity()
  
  
  void Geometry::TPC_iterator::new_cryostat() {
    tpcid.TPC = 0;
    limits.TPC = pGeo->NTPC(tpcid.Cryostat);
  } // Geometry::TPC_iterator::new_cryostat()
  
  
  //--------------------------------------------------------------------
  Geometry::plane_iterator& Geometry::plane_iterator::operator++() {
    if (!planeid.isValid) return *this;
    
    ++planeid.Plane;
    while (true) {
      if (planeid.Plane < limits.Plane) return *this;
      if (++planeid.TPC >= limits.TPC) {
        if (++planeid.Cryostat >= limits.Cryostat) break;
        new_cryostat();
      }
      new_tpc();
    } // while
    planeid.isValid = false;
    return *this;
  } // Geometry::plane_iterator::operator++()
  
  
  const PlaneGeo* Geometry::plane_iterator::get() const {
    return planeid.isValid?
      &(pGeo->Plane(planeid.Plane, planeid.TPC, planeid.Cryostat)): nullptr;
  } // Geometry::plane_iterator::get()
  
  
  const TPCGeo* Geometry::plane_iterator::getTPC() const {
    return planeid.isValid?
      &(pGeo->TPC(planeid.TPC, planeid.Cryostat)): nullptr;
  } // Geometry::plane_iterator::getTPC()
  
  
  const CryostatGeo* Geometry::plane_iterator::getCryostat() const {
    return planeid.isValid? &(pGeo->Cryostat(planeid.Cryostat)): nullptr;
  } // Geometry::plane_iterator::getCryostat()
  
  
  void Geometry::plane_iterator::init_geometry()
    { pGeo = &*(art::ServiceHandle<Geometry>()); }
  
  
  void Geometry::plane_iterator::set_limits_and_validity() {
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
  } // Geometry::plane_iterator::set_limits_and_validity()
  
  
  void Geometry::plane_iterator::new_cryostat() {
    planeid.TPC = 0;
    limits.TPC = pGeo->NTPC(planeid.Cryostat);
  } // Geometry::plane_iterator::new_cryostat()
  
  
  void Geometry::plane_iterator::new_tpc() {
    planeid.Plane = 0;
    limits.Plane = pGeo->Nplanes(planeid.TPC, planeid.Cryostat);
  } // Geometry::plane_iterator::new_tpc()
  
  
  //--------------------------------------------------------------------
  Geometry::wire_iterator& Geometry::wire_iterator::operator++() {
    if (!wireid.isValid) return *this;
    
    ++wireid.Wire;
    while (true) {
      if (wireid.Wire < limits.Wire) return *this;
      if (++wireid.Plane >= limits.Plane) {
        if (++wireid.TPC >= limits.TPC) {
          if (++wireid.Cryostat >= limits.Cryostat) break;
          new_cryostat();
        } // if new cryostat
        new_tpc();
      } // if new TPC
      new_plane();
    } // while
    wireid.isValid = false;
    return *this;
  } // Geometry::wire_iterator::operator++()
  
  
  const WireGeo* Geometry::wire_iterator::get() const {
    return wireid.isValid? &(getPlane()->Wire(wireid.Wire)): nullptr;
  } // Geometry::wire_iterator::get()
  
  
  const PlaneGeo* Geometry::wire_iterator::getPlane() const {
    return wireid.isValid?
      &(pGeo->Plane(wireid.Plane, wireid.TPC, wireid.Cryostat)): nullptr;
  } // Geometry::wire_iterator::get()
  
  
  const TPCGeo* Geometry::wire_iterator::getTPC() const {
    return wireid.isValid? &(pGeo->TPC(wireid.TPC, wireid.Cryostat)): nullptr;
  } // Geometry::wire_iterator::getTPC()
  
  
  const CryostatGeo* Geometry::wire_iterator::getCryostat() const {
    return wireid.isValid? &(pGeo->Cryostat(wireid.Cryostat)): nullptr;
  } // Geometry::wire_iterator::getCryostat()
  
  
  void Geometry::wire_iterator::init_geometry()
    { pGeo = &*(art::ServiceHandle<Geometry>()); }
  
  
  void Geometry::wire_iterator::set_limits_and_validity() {
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
  } // Geometry::wire_iterator::set_limits_and_validity()
  
  
  void Geometry::wire_iterator::new_cryostat() {
    wireid.TPC = 0;
    limits.TPC = pGeo->NTPC(wireid.Cryostat);
  } // Geometry::wire_iterator::new_cryostat()
  
  
  void Geometry::wire_iterator::new_tpc() {
    wireid.Plane = 0;
    limits.Plane = pGeo->Nplanes(wireid.TPC, wireid.Cryostat);
  } // Geometry::wire_iterator::new_tpc()
  
  void Geometry::wire_iterator::new_plane() {
    wireid.Wire = 0;
    limits.Wire = pGeo->Nwires(wireid.Plane, wireid.TPC, wireid.Cryostat);
  } // Geometry::wire_iterator::new_plane()
  
  
  DEFINE_ART_SERVICE(Geometry)
} // namespace geo
