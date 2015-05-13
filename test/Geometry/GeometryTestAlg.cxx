/**
 * @file   GeometryTestAlg.cxx
 * @brief  Unit test for geometry functionalities: implementation file
 * @date   2011/02/17
 * @author brebel@fnal.gov
 * @see    GeometryTestAlg.h
 */

// our header
#include "test/Geometry/GeometryTestAlg.h"

// LArSoft includes
#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/GeometryCore.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "Geometry/OpDetGeo.h"
#include "Geometry/AuxDetGeo.h"
#include "Geometry/geo.h"

// Framework includes
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// ROOT includes
#include "TGeoManager.h"
#include "TStopwatch.h"

// C/C++ standard libraries
#include <cmath>
#include <vector>
#include <iterator> // std::inserter()
#include <algorithm> // std::copy()
#include <set>
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include <limits> // std::numeric_limits<>


namespace {
  template <typename T>
  inline T sqr(T v) { return v*v; }
  
  template <typename T>
  std::string to_string(const T& v) {
    std::ostringstream sstr;
    sstr << v;
    return sstr.str();
  } // ::to_string()
} // local namespace


namespace geo{

  const std::vector<std::string> GeometryTestAlg::DefaultTests = {
   "Cryostat",
   "ChannelToWire",
   "FindPlaneCenters",
   "Projection",
   "NearestWire",
   "WireIntersection",
   "WirePitch",
   "PlanePitch",
   "Stepping"
  }; // GeometryTestAlg::DefaultTests
  
  //......................................................................
  GeometryTestAlg::GeometryTestAlg(fhicl::ParameterSet const& pset) 
    : geom(nullptr)
    , fDisableValidWireIDcheck( pset.get<bool>("DisableWireBoundaryCheck", false) )
  {
    // initialize the list of non-fatal exceptions
    std::vector<std::string> NonFatalErrors(pset.get<std::vector<std::string>>
      ("ForgiveExceptions", std::vector<std::string>()));
    std::copy(NonFatalErrors.begin(), NonFatalErrors.end(),
      std::inserter(fNonFatalExceptions, fNonFatalExceptions.end()));
    
    // initialize the list of tests to be run
    std::vector<std::string> RunTests(pset.get<std::vector<std::string>>
      ("RunTests", std::vector<std::string>()));
    if (RunTests.empty()) RunTests = DefaultTests;
    std::copy(RunTests.begin(), RunTests.end(),
      std::inserter(fRunTests, fRunTests.end()));
    
    if (pset.get<bool>("CheckForOverlaps", false))
      fRunTests.insert("CheckOverlaps");
    
    if (pset.get<bool>("PrintWires", false))
      fRunTests.insert("PrintWires");
    
    std::ostringstream sstr;
    std::ostream_iterator<std::string> iOut(sstr, " ");
    std::copy(fRunTests.begin(), fRunTests.end(), iOut);
    mf::LogInfo("GeometryTestAlg") << "Will run " << fRunTests.size() << " tests: "
      << sstr.str();
    
  } // GeometryTestAlg::GeometryTestAlg()

  //......................................................................
  unsigned int GeometryTestAlg::Run()
  {
    
    if (!geom) {
      throw cet::exception("GeometryTestAlg")
        << "GeometryTestAlg not configured: no valid geometry provided.\n";
    }
    
    unsigned int nErrors = 0; // currently unused
    
    // change the printed version number when changing the "GeometryTest" output
    mf::LogVerbatim("GeometryTest") << "GeometryTest version 1.0";
    
    mf::LogInfo("GeometryTestInfo")
      << "Running on detector: '" << geom->DetectorName() << "'";
    
    
    try{
      mf::LogVerbatim("GeometryTest")
        <<   "Wire Rmax  "         << geom->Plane(1).Wire(10).RMax()
        << "\nWire length "        << 2.*geom->Plane(1).Wire(10).HalfL()
        << "\nWire Rmin  "         << geom->Plane(1).Wire(10).RMin()
        << "\nTotal mass "         << geom->TotalMass()
        << "\nNumber of views "    << geom->Nviews()
        << "\nNumber of channels " << geom->Nchannels()
        ;

      //LOG_DEBUG("GeometryTest") << "print channel information ...";
      //printChannelSummary();
      //LOG_DEBUG("GeometryTest") << "done printing.";
      //mf::LogVerbatim("GeometryTest") << "print Cryo/TPC boundaries in world coordinates ...";
      //printVolBounds();
      //mf::LogVerbatim("GeometryTest") << "done printing.";
      //mf::LogVerbatim("GeometryTest") << "print Cryo/TPC dimensions ...";
      //printDetDim();
      //mf::LogVerbatim("GeometryTest") << "done printing.";
      //mf::LogVerbatim("GeometryTest") << "print wire center positions in world coordinates ...";
      //printWirePos();
      //mf::LogVerbatim("GeometryTest") << "done printing.";

      if (shouldRunTests("CheckOverlaps")) {
        LOG_DEBUG("GeometryTest") << "test for overlaps ...";
        gGeoManager->CheckOverlaps(1e-5);
        gGeoManager->PrintOverlaps();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Cryostat")) {
        LOG_DEBUG("GeometryTest") << "test Cryostat methods ...";
        testCryostat();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("ChannelToWire")) {
        LOG_DEBUG("GeometryTest") << "test channel to plane wire and back ...";
        testChannelToWire();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("FindPlaneCenters")) {
        LOG_DEBUG("GeometryTest") << "test find plane centers...";
        testFindPlaneCenters();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Projection")) {
        LOG_DEBUG("GeometryTest") << "testProject...";
        testProject();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("WirePos")) {
        LOG_DEBUG("GeometryTest") << "testWirePos...";
        // There is a contradiction here, and these must be tested differently
        // Testing based on detector ID should NOT become common practice
        LOG_DEBUG("GeometryTest") << "disabled.";
      }

      if (shouldRunTests("NearestWire")) {
        LOG_DEBUG("GeometryTest") << "testNearestWire...";
        testNearestWire();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("WireIntersection")) {
        LOG_DEBUG("GeometryTest") << "testWireIntersection...";
        testWireIntersection();
        LOG_DEBUG("GeometryTest") << "testWireIntersection complete";
      }

      if (shouldRunTests("WirePitch")) {
        LOG_DEBUG("GeometryTest") << "testWirePitch...";
        testWirePitch();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("PlanePitch")) {
        LOG_DEBUG("GeometryTest") << "testPlanePitch...";
        testPlanePitch();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("Stepping")) {
        LOG_DEBUG("GeometryTest") << "testStepping...";
        testStepping();
        LOG_DEBUG("GeometryTest") << "complete.";
      }

      if (shouldRunTests("PrintWires")) {
        LOG_DEBUG("GeometryTest") << "printAllGeometry...";
        printAllGeometry();
        LOG_DEBUG("GeometryTest") << "complete.";
      }
    }
    catch (cet::exception &e) {
      mf::LogWarning("GeometryTest") << "exception caught: \n" << e;
      if (fNonFatalExceptions.count(e.category()) == 0) throw;
    }
      
    return nErrors;
  }



  //......................................................................
  void GeometryTestAlg::printChannelSummary()
  {
    static unsigned int OneSeg = 0;
    static unsigned int TwoSegs = 0;
    static unsigned int ThreeSegs = 0;
    static unsigned int FourSegs = 0;
    uint32_t channels = geom->Nchannels();
    if(geom->NTPC() > 1) channels /= geom->NTPC()/2;

    for(uint32_t c = 0; c < channels; c++){

      unsigned int ChanSize = geom->ChannelToWire(c).size();

       if     (ChanSize==1) ++OneSeg;
       else if(ChanSize==2) ++TwoSegs;
       else if(ChanSize==3) ++ThreeSegs;
       else if(ChanSize==4) ++FourSegs;

    }

     mf::LogVerbatim("GeometryTest") << "OneSeg: "       << OneSeg 
				     << ",  TwoSegs: "   << TwoSegs
				     << ",  ThreeSegs: " << ThreeSegs
				     << ",  FourSegs: "  << FourSegs;

  }

  //......................................................................
  void GeometryTestAlg::printVolBounds()
  {
      double origin[3] = {0.};
      double world[3] = {0.};
      for(unsigned int c = 0; c < geom->Ncryostats(); ++c){
	geom->Cryostat(c).LocalToWorld(origin, world);

        mf::LogVerbatim("GeometryTest") << "Cryo " << c;
	mf::LogVerbatim("GeometryTest") << "    -x: " << world[0] - geom->Cryostat(c).HalfWidth();
	mf::LogVerbatim("GeometryTest") << "    +x: " << world[0] + geom->Cryostat(c).HalfWidth();
	mf::LogVerbatim("GeometryTest") << "    -y: " << world[1] - geom->Cryostat(c).HalfHeight();
	mf::LogVerbatim("GeometryTest") << "    +y: " << world[1] + geom->Cryostat(c).HalfHeight();
	mf::LogVerbatim("GeometryTest") << "    -z: " << world[2] - geom->Cryostat(c).Length()/2;
	mf::LogVerbatim("GeometryTest") << "    +z: " << world[2] + geom->Cryostat(c).Length()/2;

        for(unsigned int t = 0; t < geom->NTPC(c); ++t){
          geom->Cryostat(c).TPC(t).LocalToWorld(origin, world);

          mf::LogVerbatim("GeometryTest") << "  TPC " << t;
          mf::LogVerbatim("GeometryTest") << "    -x: " << world[0] - geom->Cryostat(c).TPC(t).HalfWidth();
          mf::LogVerbatim("GeometryTest") << "    +x: " << world[0] + geom->Cryostat(c).TPC(t).HalfWidth();
          mf::LogVerbatim("GeometryTest") << "    -y: " << world[1] - geom->Cryostat(c).TPC(t).HalfHeight();
          mf::LogVerbatim("GeometryTest") << "    +y: " << world[1] + geom->Cryostat(c).TPC(t).HalfHeight();
          mf::LogVerbatim("GeometryTest") << "    -z: " << world[2] - geom->Cryostat(c).TPC(t).Length()/2;
          mf::LogVerbatim("GeometryTest") << "    +z: " << world[2] + geom->Cryostat(c).TPC(t).Length()/2;
        }
      }

  }



  //......................................................................
  // great sanity check for geometry, only call in analyze when debugging
  void GeometryTestAlg::printDetDim()
  {
    for(unsigned int c = 0; c < geom->Ncryostats(); ++c){

      mf::LogVerbatim("GeometryTest") << "Cryo " << c;
      mf::LogVerbatim("GeometryTest") << "    width: "
				      << geom->CryostatHalfWidth(c);
      mf::LogVerbatim("GeometryTest") << "    height: "
				      << geom->CryostatHalfHeight(c);
      mf::LogVerbatim("GeometryTest") << "    length: "
				      << geom->CryostatLength(c);

      mf::LogVerbatim("GeometryTest") << "  TPC 0";
      mf::LogVerbatim("GeometryTest") << "    width: "
				      << geom->DetHalfWidth(0,c);
      mf::LogVerbatim("GeometryTest") << "    height: "
				      << geom->DetHalfHeight(0,c);
      mf::LogVerbatim("GeometryTest") << "    length: "
				      << geom->DetLength(0,c);      
    }
  }

  //......................................................................
  // great sanity check for volume sorting, only call in analyze when debugging
  void GeometryTestAlg::printWirePos()
  {
    unsigned int cs = 0;

    for(unsigned int t=0; t<std::floor(geom->NTPC()/12)+1; ++t){
      for(unsigned int p=0; p<3; ++p){
        for(unsigned int w=0; w<geom->Cryostat(0).TPC(t).Plane(p).Nwires(); w++){
        
          double xyz[3] = {0.};
          geom->Cryostat(0).TPC(t).Plane(p).Wire(w).GetCenter(xyz);

          std::cout << "WireID (" << cs << ", " << t << ", " << p << ", " << w
                    << "):  x = " << xyz[0] 
                    << ", y = " << xyz[1]
                    << ", z = " << xyz[2] << std::endl;
        }
      }
    }
  }

  //......................................................................
  // great insanity: print all wires in a TPC
  void GeometryTestAlg::printWiresInTPC
    (const geo::TPCGeo& tpc, std::string indent /* = "" */) const
  {
    const unsigned int nPlanes = tpc.Nplanes();
    const double Origin[3] = { 0., 0., 0. };
    double TPCpos[3];
    tpc.LocalToWorld(Origin, TPCpos);
    mf::LogVerbatim("GeometryTest") << indent << "TPC at ("
      << TPCpos[0] << ", " << TPCpos[1] << ", " << TPCpos[2]
      << ") cm has " << nPlanes << " wire planes (max wires: " << tpc.MaxWires()
      << "):";
    for(unsigned int p = 0; p < nPlanes; ++p) {
      const geo::PlaneGeo& plane = tpc.Plane(p);
      const unsigned int nWires = plane.Nwires();
      double PlanePos[3];
      plane.LocalToWorld(Origin, PlanePos);
      std::string coord, orientation;
      switch (plane.View()) {
        case geo::kU:       coord = "U direction"; break;
        case geo::kV:       coord = "V direction"; break;
        case geo::kZ:       coord = "Z direction"; break;
        case geo::k3D:      coord = "3D coordinate"; break;
        case geo::kUnknown: coord = "an unknown direction"; break;
        default:            coord = "unexpected direction"; break;
      } // switch
      switch (plane.Orientation()) {
        case geo::kHorizontal: orientation = "horizontal"; break;
        case geo::kVertical:   orientation = "vertical"; break;
        default:               orientation = "unexpected"; break;
      }
      mf::LogVerbatim("GeometryTest") << indent << "  plane #" << p << " at ("
        << PlanePos[0] << ", " << PlanePos[1] << ", " << PlanePos[2] << ") cm"
        " has " << orientation << " orientation and "
        << nWires << " wires measuring " << coord << ":";
      for(unsigned int w = 0;  w < nWires; ++w) {
        const geo::WireGeo& wire = plane.Wire(w);
        double xyz[3] = { 0. };
        wire.LocalToWorld(xyz, xyz); // LocalToWorld() supports in place transf.
        double WireS[3],  WireM[3], WireE[3]; // start, middle point and end
        
        // the wire should be aligned on z axis, half on each side of 0,
        // in its local frame
        wire.GetStart(WireS);
        wire.GetCenter(WireM);
        wire.GetEnd(WireE);
        mf::LogVerbatim("GeometryTest") << indent
          << "    wire #" << w
          << " at (" << xyz[0] << ", " << xyz[1] << ", " << xyz[2] << ")"
          << "\n" << indent << "       start at (" << WireS[0] << ", " << WireS[1] << ", " << WireS[2] << ")"
          << "\n" << indent << "      middle at (" << WireM[0] << ", " << WireM[1] << ", " << WireM[2] << ")"
          << "\n" << indent << "         end at (" << WireE[0] << ", " << WireE[1] << ", " << WireE[2] << ")"
          ;
      } // for wire
    } // for plane
  } // GeometryTestAlg::printWiresInTPC()

  
  void GeometryTestAlg::printAllGeometry() const {
    const unsigned int nCryostats = geom->Ncryostats();
    const double Origin[3] = { 0., 0., 0. };
    mf::LogVerbatim("GeometryTest") << "Detector " << geom->DetectorName()
      << " has " << nCryostats << " cryostats:";
    for(unsigned int c = 0; c < nCryostats; ++c) {
      const geo::CryostatGeo& cryostat = geom->Cryostat(c);
      const unsigned int nTPCs = cryostat.NTPC();
      double CryoPos[3];
      cryostat.LocalToWorld(Origin, CryoPos);
      mf::LogVerbatim("GeometryTest") << "  cryostat #" << c << " at ("
				      << CryoPos[0] << ", " << CryoPos[1] << ", " << CryoPos[2] << ") cm has "
				      << nTPCs << " TPC(s):";
      for(unsigned int t = 0;  t < nTPCs; ++t) {
        const geo::TPCGeo& tpc = cryostat.TPC(t);
        if (nTPCs > 1) mf::LogVerbatim("GeometryTest") << "    TPC #" << t;
        printWiresInTPC(tpc, "    ");
      } // for TPC
    } // for cryostat
    mf::LogVerbatim("GeometryTest") << "End of detector "
				    << geom->DetectorName() << " geometry.";
  } // GeometryTestAlg::printAllGeometry()

  //......................................................................
  void GeometryTestAlg::testCryostat()
  {
    mf::LogVerbatim("GeometryTest") << "\tThere are " << geom->Ncryostats() << " cryostats in the detector";

    for(unsigned int c = 0; c < geom->Ncryostats(); ++c){

      mf::LogVerbatim("GeometryTest") << "\n\t\tCryostat " << c 
				      << " " << geom->Cryostat(c).Volume()->GetName()
				      << " Dimensions: " << 2.*geom->Cryostat(c).HalfWidth()
				      << " x "           << 2.*geom->Cryostat(c).HalfHeight() 
				      << " x "           << geom->Cryostat(c).Length()
				      << "\n\t\t mass: " << geom->Cryostat(c).Mass();

      double cryobound[6] = {0.};
      geom->CryostatBoundaries(cryobound, c);
      mf::LogVerbatim("GeometryTest") << "Cryostat boundaries are at:\n"
				      << "\t-x:" << cryobound[0] << " +x:" << cryobound[1]
				      << "\t-y:" << cryobound[2] << " +y:" << cryobound[3]
				      << "\t-z:" << cryobound[4] << " +z:" << cryobound[5];

      // pick a position in the middle of the cryostat in the world coordinates
      double worldLoc[3] = {0.5*(cryobound[1] - cryobound[0]) + cryobound[0],
			    0.5*(cryobound[3] - cryobound[2]) + cryobound[2],
			    0.5*(cryobound[5] - cryobound[4]) + cryobound[4]};
		
      LOG_DEBUG("GeometryTest") << "\t testing GeometryCore::PoitionToCryostat....";
      try{
	unsigned int cstat = 0;
	geom->PositionToCryostat(worldLoc, cstat);
      }
      catch(cet::exception &e){
	mf::LogWarning("FailedToLocateCryostat") << "\n exception caught:" << e;
	if (fNonFatalExceptions.count(e.category()) == 0) throw;
      }
      LOG_DEBUG("GeometryTest") << "done";

      LOG_DEBUG("GeometryTest") << "\t Now test the TPCs associated with this cryostat";
      this->testTPC(c);
    }

    return;
  }

  //......................................................................
  void GeometryTestAlg::testTPC(unsigned int const& c)
  {
    geo::CryostatGeo const& cryo = geom->Cryostat(c);

    mf::LogVerbatim("GeometryTest") << "\tThere are " << cryo.NTPC() 
                                    << " TPCs in the detector";
    
    for(size_t t = 0; t < cryo.NTPC(); ++t){
      geo::TPCGeo const& tpc = cryo.TPC(t);
      
      // figure out the TPC coordinates
      
      std::array<double, 3> TPClocalTemp, TPCstart, TPCstop;
      TPClocalTemp[0] = -tpc.HalfWidth(); // x
      TPClocalTemp[1] = -tpc.HalfHeight(); // y
      TPClocalTemp[2] = -tpc.Length() / 2.; // z
      tpc.LocalToWorld(TPClocalTemp.data(), TPCstart.data());
      for (size_t i = 0; i < TPClocalTemp.size(); ++i) TPClocalTemp[i] = -TPClocalTemp[i];
      tpc.LocalToWorld(TPClocalTemp.data(), TPCstop.data());
      
      mf::LogVerbatim("GeometryTest") << "\n\t\tTPC " << t 
                                      << " " << geom->GetLArTPCVolumeName(t, c) 
                                      << " has " 
                                      << tpc.Nplanes() << " planes."
                                      << "\n\t\tTPC location:"
                                      << " ( " << TPCstart[0] << " ; " << TPCstart[1] << " ; "<< TPCstart[2] << " ) => "
                                      << " ( " << TPCstop[0] << " ; " << TPCstop[1] << " ; "<< TPCstop[2] << " ) [cm]"
                                      << "\n\t\tTPC Dimensions: " << 2.*tpc.HalfWidth()
                                      << " x " << 2.*tpc.HalfHeight() 
                                      << " x " << tpc.Length()
                                      << "\n\t\tTPC Active Dimensions: " 
                                      << 2.*tpc.ActiveHalfWidth()
                                      << " x " << 2.*tpc.ActiveHalfHeight() 
                                      << " x " << tpc.ActiveLength()
                                      << "\n\t\tTPC mass: " << tpc.ActiveMass()
                                      << "\n\t\tTPC drift distance: " 
                                      << tpc.DriftDistance();
      
      for(size_t p = 0; p < tpc.Nplanes(); ++p) {
        geo::PlaneGeo const& plane = tpc.Plane(p);
        mf::LogVerbatim("GeometryTest") << "\t\tPlane " << p << " has " 
                                        << plane.Nwires() 
                                        << " wires and is at (x,y,z) = (" 
                                        << tpc.PlaneLocation(p)[0] << "," 
                                        << tpc.PlaneLocation(p)[1] << "," 
                                        << tpc.PlaneLocation(p)[2] 
                                        << "); \n\t\tpitch from plane 0 is "
                                        << tpc.Plane0Pitch(p) << "; \n\t\tOrientation "
                                        << plane.Orientation() << ", View "
                                        << plane.View() << ", Wire angle "
                                        << plane.Wire(0).ThetaZ();
      } // for plane
      geo::DriftDirection_t dir = tpc.DriftDirection();
      if     (dir == geo::kNegX) 
        mf::LogVerbatim("GeometryTest") << "\t\tdrift direction is towards negative x values";
      else if(dir == geo::kPosX) 
        mf::LogVerbatim("GeometryTest") << "\t\tdrift direction is towards positive x values";
      else{
        throw cet::exception("UnknownDriftDirection") << "\t\tdrift direction is unknown\n";
      }

      LOG_DEBUG("GeometryTest") << "\t testing PositionToTPC...";
      // pick a position in the middle of the cryostat in the world coordinates
      double worldLoc[3] = {0.};
      double localLoc[3] = {0.};
      tpc.LocalToWorld(localLoc, worldLoc);

      const unsigned int tpcNo = cryo.FindTPCAtPosition(worldLoc, 1+1.e-4);

      if(tpcNo != t)
        throw cet::exception("BadTPCLookupFromPosition") << "TPC look up returned tpc = "
                                                         << tpcNo << " should be " << t << "\n";

      LOG_DEBUG("GeometryTest") << "done.";
    } // for TPC
    
    return;
  }


  //......................................................................
  void GeometryTestAlg::testChannelToWire()
  {

    for(unsigned int cs = 0; cs < geom->Ncryostats(); ++cs){
      for(unsigned int tpc = 0; tpc < geom->Cryostat(cs).NTPC(); ++tpc){
	for(unsigned int plane = 0; plane < geom->Cryostat(cs).TPC(tpc).Nplanes(); ++plane){
	  for(unsigned int wire = 0; wire < geom->Cryostat(cs).TPC(tpc).Plane(plane).Nwires(); ++wire){

	    uint32_t channel = geom->PlaneWireToChannel(plane, wire, tpc, cs);
	    //std::cout << "WireID (" << cs << ", " << tpc << ", " << plane << ", " << wire 
	    //	<< ") --> Channel " << channel << std::endl;    
	    std::vector< geo::WireID > wireIDs = geom->ChannelToWire(channel);
	    

	    if ( wireIDs.size() == 0 ) 
	      throw cet::exception("BadChannelLookup") << "requested channel: " << channel 
						       << ";" << cs << "," << tpc
						       << "," << plane << "," << wire << "\n"
						       << "got back an empty vector of WireID " << "\n";

	    bool goodLookup = false;
	    for( auto const& wid : wireIDs){
	      if(wid.Cryostat == cs    && 
		 wid.TPC      == tpc   && 
		 wid.Plane    == plane && 
		 wid.Wire     == wire) goodLookup = true;
	    }
	    
	    if(!goodLookup)
	    {
	      std::cout << "Returned: " << std::endl;
              for(unsigned int id=0; id<wireIDs.size(); ++id)
	      {
		std::cout << "wireIDs[" << id << "] = ("
		          << wireIDs[id].Cryostat << ", "
		          << wireIDs[id].TPC      << ", "
		          << wireIDs[id].Plane    << ", "
		          << wireIDs[id].Wire     << ")" << std::endl;
              }
	      throw cet::exception("BadChannelLookup") << "requested channel " << channel 
						       << "expected to return" << cs << "," << tpc
						       << "," << plane << "," << wire << "\n"
						       << "no returned geo::WireID structs matched\n";
            }

	    if(geom->SignalType(channel) != geom->Plane(plane, tpc, cs).SignalType() )
	      throw cet::exception("BadChannelLookup") << "expected signal type: SignalType(channel) = " 
						       << geom->SignalType(channel)
						       << " for channel " 
						       << channel << ", WireID ("  
						       << cs << ", " << tpc << ", " << plane << ", " << wire
						       << "), got: Plane(" << plane << ", " << tpc 
						                           << ", " << cs << ").SignalType() = "
						       << geom->Plane(plane, tpc, cs).SignalType() << "\n";


	    if(geom->View(channel) != geom->Plane(plane, tpc, cs).View() )
	      throw cet::exception("BadChannelLookup") << "expected view type: View(channel) = " 
						       << geom->View(channel)
						       << " for channel " 
						       << channel << ", WireID ("  
						       << cs << ", " << tpc << ", " << plane << ", " << wire
						       << "), got: Plane(" << plane << ", " << tpc 
						                           << ", " << cs << ").View() = "
						       << geom->Plane(plane, tpc, cs).View() << "\n";

	  }
	}
      }
    }

    return;
  }

  //......................................................................
  void GeometryTestAlg::testFindPlaneCenters()
  {
    double xyz[3] = {0.},   xyzW[3] = {0.};
    for(size_t i = 0; i < geom->Nplanes(); ++i){ 
      geom->Plane(i).LocalToWorld(xyz,xyzW);
      mf::LogVerbatim("GeometryTest") << "\n\tplane " 
				      << i << " is centered at (x,y,z) = (" 
				      << xyzW[0] << "," << xyzW[1]
				      << "," << xyzW[2] << ")";
    } 
  } 

  //......................................................................
  void GeometryTestAlg::testStandardWirePos() 
  {
    double xyz[3] = {0.};
    double xyzprev[3] = {0.};
    for(size_t cs = 0; cs < geom->Ncryostats(); ++cs){
      for(size_t t = 0; t < geom->Cryostat(cs).NTPC(); ++t){
	const geo::TPCGeo* tpc = &geom->Cryostat(cs).TPC(t); 

	for (size_t i=0; i < tpc->Nplanes(); ++i) {
	  const geo::PlaneGeo* plane = &tpc->Plane(i);

	  for (size_t j = 1; j < plane->Nwires(); ++j) {

	    const geo::WireGeo wire = plane->Wire(j);
	    const geo::WireGeo wireprev = plane->Wire(j-1);

	    wire.GetCenter(xyz);
	    wireprev.GetCenter(xyzprev);

	    // wires increase in +z order
	    if(xyz[2] < xyzprev[2])
	      throw cet::exception("WireOrderProblem") 	<< "\n\twires do not increase in +z order in"
							<< "Cryostat " << cs
							<< ", TPC " << t
							<< ", Plane " << i
							<< ";  at wire " << j << "\n";

	  }// end loop over wires
	}// end loop over planes
      }// end loop over tpcs
    }// end loop over cryostats

}

  //......................................................................
  void GeometryTestAlg::testAPAWirePos() 
  {
    double origin[3] = {0.};
    double tpcworld[3] = {0.};
    double xyz[3] = {0.};
    double xyzprev[3] = {0.};
    for(size_t cs = 0; cs < geom->Ncryostats(); ++cs){
      for(size_t t = 0; t < geom->Cryostat(cs).NTPC(); ++t){
	const geo::TPCGeo* tpc = &geom->Cryostat(cs).TPC(t);
	tpc->LocalToWorld(origin, tpcworld);

	for (size_t i=0; i < tpc->Nplanes(); ++i) {
	  const geo::PlaneGeo* plane = &tpc->Plane(i);

	  for (size_t j = 1; j < plane->Nwires(); ++j) {
	    const geo::WireGeo wire = plane->Wire(j);
	    const geo::WireGeo wireprev = plane->Wire(j-1);

	    wire.GetCenter(xyz);
	    wireprev.GetCenter(xyzprev);

            // top TPC wires increase in -y
	    if(tpcworld[1] > 0 && xyz[1] > xyzprev[1])
	      throw cet::exception("WireOrderProblem") 	<< "\n\ttop TPC wires do not increase in -y order in"
							<< "Cryostat " << cs
							<< ", TPC " << t
							<< ", Plane " << i
							<< ";  at wire " << j << "\n";
            // bottom TPC wires increase in +y
	    else if(tpcworld[1] < 0 && xyz[1] < xyzprev[1])
	      throw cet::exception("WireOrderProblem") 	<< "\n\tbottom TPC wires do not increase in +y order in"
                                                        << "Cryostat " << cs
							<< ", TPC " << t
                                                        << ", Plane " << i 
                                                        << ";  at wire " << j << "\n";
	  }// end loop over wires
	}// end loop over planes
      }// end loop over tpcs
    }// end loop over cryostats

  }
  
  
  //......................................................................
  inline std::array<double, 3> GeometryTestAlg::GetIncreasingWireDirection
    (const geo::PlaneGeo& plane)
  {
    TVector3 IncreasingWireDir = plane.GetIncreasingWireDirection();
    return
      { IncreasingWireDir.X(), IncreasingWireDir.Y(), IncreasingWireDir.Z() };
  } // GeometryTestAlg::GetIncreasingWireDirection()
  
  
  //......................................................................
  void GeometryTestAlg::testNearestWire()
  {
    // Even if you comment it out, please leave the TStopWatch code
    // in this code for additional testing. The NearestChannel routine
    // is the most frequently called in the simulation, so its execution time
    // is an important component of LArSoft's speed.
    TStopwatch stopWatch;
    stopWatch.Start();

    bool bTestWireCoordinate = true;
    
    // get a wire and find its center
    geo::GeometryCore::plane_iterator iPlane(&*geom);
    while (iPlane) {
      unsigned int cs = iPlane->Cryostat;
      unsigned int t = iPlane->TPC;
      unsigned int p = iPlane->Plane;
      
      const geo::PlaneGeo& plane = *(iPlane.get());
      const unsigned int NWires = plane.Nwires();
      
      const std::array<double, 3> IncreasingWireDir
        = GetIncreasingWireDirection(plane);
      
      LOG_DEBUG("GeoTestWireCoordinate")
        << "The direction of increasing wires for plane C=" << cs << " T=" << t
        << " P=" << p << " (theta=" << plane.Wire(0).ThetaZ() << " pitch="
        << plane.WirePitch() << " orientation="
        << (plane.Orientation() == geo::kHorizontal? "H": "V")
        << (plane.WireIDincreasesWithZ()? "+": "-")
        << ") is ( " << IncreasingWireDir[0] << " ; "
        << IncreasingWireDir[1] << " ; " << IncreasingWireDir[2] << ")";
      
      for (unsigned int w = 0; w < NWires; ++w) {
        
        geo::WireID wireID(*iPlane, w);
        
        const geo::WireGeo& wire = plane.Wire(w);
        const double pos[3] = {0., 0.0, 0.};
        std::array<double, 3> wire_center;
        wire.LocalToWorld(pos, wire_center.data());
        
        uint32_t nearest = 0;
        std::vector< geo::WireID > wireIDs;
        
        try{
          // The double[] version tested here falls back on the
          // TVector3 version, so this test both.
          nearest = geom->NearestChannel(wire_center.data(), p, t, cs);
          
          // We also want to test the std::vector<duoble> version
          std::array<double, 3> posWorldV;
          for (int i=0; i<3; ++i) {
            posWorldV[i] = wire_center[i] + 0.001;
          }
          nearest = geom->NearestChannel(posWorldV.data(), p, t, cs);
        }
        catch(cet::exception &e){
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
        try{
          wireIDs = geom->ChannelToWire(nearest);
          
          if ( wireIDs.empty() ) {
            throw cet::exception("BadPositionToChannel") << "test point is at " 
                                                         << wire_center[0] << " " 
                                                         << wire_center[1] << " " 
                                                         << wire_center[2] << "\n"
                                                         << "nearest channel is " 
                                                         << nearest << " for " 
                                                         << cs << " " << t << " "
                                                         << p << " " << w << "\n";
          }
        }
        catch(cet::exception &e){
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
        if(std::find(wireIDs.begin(), wireIDs.end(), wireID) == wireIDs.end()) {
          throw cet::exception("BadPositionToChannel") << "Current WireID ("
                                                       << cs << "," << t << "," << p << "," << w << ") "
                                                       << "has a world position at "
                                                       << wire_center[0] << " " 
                                                       << wire_center[1] << " " 
                                                       << wire_center[2] << "\n"
                                                       << "NearestWire for this position is "
                                                       << geom->NearestWire(wire_center.data(),p,t,cs) << "\n"
                                                       << "NearestChannel is " 
                                                       << nearest << " for " 
                                                       << cs << " " << t << " " << p << " " << w << "\n"
                                                       << "Should be channel "
                                                       << geom->PlaneWireToChannel(p,w,t,cs);
        } // if good lookup fails
        
        
        // nearest wire, integral and floating point
        try {
          // The test consists in sampling NStep (=5) points between the current
          // wire and the previous/next, following the normal to the wire.
          // We expect WireCoordinate() to reflect the same shift.
          
          // using absolute value just in case (what happens if w1 > w2?)
          const double pitch
            = std::abs(geom->WirePitch((w > 0)? w - 1: 1, w, p, t, cs));
          
          double wire_shifted[3];
          double step[3];
          for (size_t i = 0; i < 3; ++i) step[i] = pitch * IncreasingWireDir[i];
          
          constexpr int NSteps = 5; // odd value avoids testing half-way
          for (int i = -NSteps; i <= +NSteps; ++i) {
            // we move away by this fraction of wire:
            const double f = NSteps? (double(i) / NSteps): 0.0;
            
            // these are the actual shifts on the positive directions y and z
            std::array<double, 3> delta;
            
            for (size_t i = 0; i < 3; ++i) {
              delta[i] = f * step[i];
              wire_shifted[i] = wire_center[i] + delta[i];
            } // for
            
            // we expect this wire number
            const double expected_wire = w + f;
            
            double wire_from_wc = 0;
            if (bTestWireCoordinate) {
              if (IncreasingWireDir[0] != 0.) {
                // why? because WireCoordinate() has 2D input
                LOG_ERROR("WireCoordinateNotImplemented")
                  << "The direction of increasing wires for plane "
                  << "C=" << cs << " T=" << t << " P=" << p
                  << " (theta=" << plane.Wire(0).ThetaZ() << " orientation="
                  << (plane.Orientation() == geo::kHorizontal? "H": "V")
                  << ") is ( " << IncreasingWireDir[0] << " ; "
                  << IncreasingWireDir[1] << " ; " << IncreasingWireDir[2]
                  << "), not orthogonal to x axis."
                  << " This configuration is not supported"
                  << "\n";
                bTestWireCoordinate = false;
              } // if
              try {
                wire_from_wc = geom->WireCoordinate
                  (wire_shifted[1], wire_shifted[2], p, t, cs);
              }
              catch (cet::exception& e) {
                for (const cet::exception::Category& cat: e.history()) {
                  if (cat != "NotImplemented") continue;
                  LOG_ERROR("WireCoordinateNotImplemented")
                    << "WireCoordinate() is not implemented for your ChannelMap;"
                    " skipping the test";
                  bTestWireCoordinate = false;
                }
                if (bTestWireCoordinate) throw;
              }
            }
            if (bTestWireCoordinate) {
              if (std::abs(wire_from_wc - expected_wire) > 1e-3) {
              //  throw cet::exception("GeoTestErrorWireCoordinate")
                mf::LogError("GeoTestErrorWireCoordinate")
                  << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") shows " << wire_from_wc << ", " << expected_wire
                  << " expected.\n";
              } // if mismatch
              
            } // if testing WireCoordinate
            
            if ((expected_wire > -0.5) && (expected_wire < NWires - 0.5)) {
              const unsigned int expected_wire_number = std::round(expected_wire);
              unsigned int wire_number_from_wc;
              try {
                wire_number_from_wc = geom->NearestWire(wire_shifted, p, t, cs);
              }
              catch (cet::exception& e) {
                throw cet::exception("GeoTestErrorWireCoordinate", "", e)
              //  LOG_ERROR("GeoTestErrorWireCoordinate")
                  << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") failed NearestWire(), " << expected_wire_number
                  << " expected (more precisely, " << expected_wire << ").\n";
              }
              
              if (mf::isDebugEnabled()) {
                // In debug mode, we print a lot and we don't (fatally) complain
                std::stringstream e;
                e << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") near to " << wire_number_from_wc;
                if (wire_number_from_wc != expected_wire_number) {
                  e << ", " << expected_wire_number
                    << " expected (more precisely, " << expected_wire << ").";
                // throw e;
                  LOG_ERROR("GeoTestErrorWireCoordinate") << e.str();
                }
                else {
                  mf::LogVerbatim("GeoTestWireCoordinate") << e.str();
                }
              }
              else if (wire_number_from_wc != expected_wire_number) {
                // In production mode, we don't print anything and throw on error
                throw cet::exception("GeoTestErrorWireCoordinate")
                  << "wire C:" << cs << " T:" << t << " P:" << p << " W:" << w
                  << " [center: (" << wire_center[0] << "; "
                  << wire_center[1] << "; " << wire_center[2] << ")] on step of "
                  << i << "/" << NSteps
                  << " x" << step[1] << "cm along y (" << delta[1]
                  << ") x" << step[2] << "cm along z (" << delta[2]
                  << ") near to " << wire_number_from_wc
                  << ", " << expected_wire_number
                  << " expected (more precisely, " << expected_wire << ").";
              }
            } // if shifted wire not outside boundaries
            
          } // for i
          
        } // try
        catch(cet::exception &e) {
          mf::LogWarning("GeoTestCaughtException") << e;
          if (fNonFatalExceptions.count(e.category()) == 0) throw;
        }
        
      } // for all wires in the plane
      ++iPlane;
    } // end loop over planes

    stopWatch.Stop();
    LOG_DEBUG("GeometryTest") << "\tdone testing closest channel";
    stopWatch.Print();
    
    // trigger an exception with NearestChannel
    mf::LogVerbatim("GeometryTest") << "\tattempt to cause an exception to be caught "
                                    << "when looking for a nearest channel";

    // pick a position out of the world
    double posWorld[3];
    geom->WorldBox(nullptr, posWorld + 0,
      nullptr, posWorld + 1, nullptr, posWorld + 2);
    for (int i = 0; i < 3; ++i) posWorld[i] *= 2.;

    bool hasThrown = false;
    unsigned int nearest_to_what = 0;
    try{
      nearest_to_what = geom->NearestChannel(posWorld, 0, 0, 0);
    }
    catch(const geo::InvalidWireIDError& e){
      mf::LogWarning("GeoTestCaughtException") << e
        << "\nReturned wire would be: " << e.wire_number
        << ", suggested: " << e.better_wire_number;
      hasThrown = true;
    }
    catch(cet::exception& e){
      mf::LogWarning("GeoTestCaughtException") << e;
      hasThrown = true;
    }
    if (!hasThrown) {
      if (fDisableValidWireIDcheck) {
        // ok, then why do we disable it?
        // an implementation might prefer to cap the wire number and go on
        // instead of throwing.
        LOG_WARNING("GeoTestErrorNearestChannel")
          << "GeometryCore::NearestChannel() did not raise an exception"
          " on out-of-world position (" << posWorld[0] << "; "
          << posWorld[1] << "; " << posWorld[2] << "), and returned "
          << nearest_to_what << " instead.\n"
          "This is normally considered a failure.";
      }
      else {
        throw cet::exception("GeoTestErrorNearestChannel")
          << "GeometryCore::NearestChannel() did not raise an exception"
          " on out-of-world position (" << posWorld[0] << "; "
          << posWorld[1] << "; " << posWorld[2] << "), and returned "
          << nearest_to_what << " instead\n";
      }
    }

  }

  //......................................................................
  void GeometryTestAlg::testWireIntersection() const {
    /*
     * This is a test for WireIDsIntersect() function, that returns whether
     * two wires intersect, and where.
     *
     * The test strategy is to check all the TPC one by one:
     * - if a query for wires on different cryostats fails
     * - if a query for wires on different TPCs fails
     * - if a query for wires on the same plane fails
     * - for points at the centre of a grid SplitY x SplitZ on the wire planes,
     *   test these point by testWireIntersectionAt() function (see)
     * All tests are performed; at the end, the test is considered a failure
     * if any of the single tests failed.
     */
    
    unsigned int nErrors = 0;
    for (geo::GeometryCore::TPC_iterator iTPC(&*geom); iTPC; ++iTPC) {
      const geo::TPCGeo& TPC = *(iTPC.get());
      
      LOG_DEBUG("GeometryTest") << "Cryostat #" << iTPC->Cryostat
        << " TPC #" << iTPC->TPC;
      
      // sanity: wires on different cryostats
      if (iTPC->Cryostat < geom->Ncryostats() - 1) {
        geo::WireID w1 { iTPC->Cryostat, iTPC->TPC, 0, 0 },
          w2 { iTPC->Cryostat + 1, iTPC->TPC, 1, 1 };
        geo::WireIDIntersection xing;
        if (geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "WireIDsIntersect() on " << w1
            << " and " << w2 << " returned (" << xing.y << "; " << xing.z
            << ") in TPC=" << xing.TPC
            << ", while should have reported no intersection at all";
          ++nErrors;
        } // if intersect
      } // if not the last cryostat
      
      // sanity: wires on different TPC
      if (iTPC->TPC < geom->NTPC(iTPC->Cryostat) - 1) {
        geo::WireID w1 { iTPC->Cryostat, iTPC->TPC, 0, 0 },
          w2 { iTPC->Cryostat, iTPC->TPC + 1, 1, 1 };
        geo::WireIDIntersection xing;
        if (geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "WireIDsIntersect() on " << w1
            << " and " << w2 << " returned (" << xing.y << "; " << xing.z
            << ") in TPC=" << xing.TPC
            << ", while should have reported no intersection at all";
          ++nErrors;
        } // if intersect
      } // if not the last TPC
      
      // sanity: wires on same plane
      const unsigned int nPlanes = TPC.Nplanes();
      for (unsigned int plane = 0; plane < nPlanes; ++plane) {
        geo::WireID w1 { iTPC->Cryostat, iTPC->TPC, plane, 0 },
          w2 { iTPC->Cryostat, iTPC->TPC, plane, 1 };
        geo::WireIDIntersection xing;
        if (geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "WireIDsIntersect() on " << w1
            << " and " << w2 << " returned (" << xing.y << "; " << xing.z
            << ") in TPC=" << xing.TPC
            << ", while should have reported no intersection at all";
          ++nErrors;
        } // if intersect
      } // for all planes
      
      // detect which plane has wires along z
      unsigned int planeZindex = 0;
      while (planeZindex < nPlanes) {
        if (TPC.Plane(planeZindex).View() == geo::kZ) break;
        ++planeZindex;
      } // while
      if (planeZindex == nPlanes) {
        throw cet::exception("GeometryTestAlg")
          << "No plane with wires along z in " << to_string(*iTPC);
      }
      const geo::PlaneGeo& planeZ = TPC.Plane(planeZindex);
      const unsigned int NWiresZ = planeZ.Nwires();
      
      // let's pick a point:
      constexpr unsigned int SplitZ = 19, SplitY = 17;
      for (unsigned int iZ = 0; iZ < SplitZ; ++iZ) {
        // pick the wire in the middle of the iZ-th region:
        unsigned int wireZindex = NWiresZ * (2*iZ + 1) / (2*SplitZ);
        const geo::WireGeo& wire = planeZ.Wire(wireZindex);
        double WireCoord[3];
        wire.GetCenter(WireCoord);
        
        const double x = WireCoord[0];
        const double min_y = WireCoord[1] - wire.HalfL(),
          max_y = WireCoord[1] + wire.HalfL();
        const double z = WireCoord[2];
        
        for (unsigned int iY = 0; iY < SplitY; ++iY) {
          // pick the coordinate in the middle of the iY-th region:
          const double y = min_y + (max_y - min_y) * (2*iY + 1) / (2*SplitY);
          
          // finally, let's test this point...
          nErrors += testWireIntersectionAt(*iTPC, x, y, z);
        } // for y
      } // for z
      
    } // while iTPC
    
    if (nErrors > 0) {
      throw cet::exception("GeoTestWireIntersection")
        << "Accumulated " << nErrors << " errors (see messages above)\n";
    }
    
  } // GeometryTestAlg::testWireIntersection()
  
  
  unsigned int GeometryTestAlg::testWireIntersectionAt
    (const TPCID& tpcid, double x, double y, double z) const
  {
    /* Tests WireIDsIntersect() on the specified point on the wire planes of
     * a given TPC.
     * 
     * The test follows this strategy:
     * - find the ID of the wires closest to the point on each plane
     * - for all wire plane pairing, ask for the intersection between the wires
     * - fail if the returned point is farther than half a pitch from the
     *   original point
     * 
     * This function returns the number of accumulated failures.
     */
    
    unsigned int nErrors = 0;
    
    const geo::TPCGeo& TPC = geom->TPC(tpcid);
    const unsigned int NPlanes = TPC.Nplanes();
    
    // collect information per plane:
    std::vector<double> ThetaZ(NPlanes), WirePitch(NPlanes); // for convenience
    std::vector<geo::WireID> WireIDs; // ID of the closest wire
    WireIDs.reserve(NPlanes);
    std::vector<float> WireDistances(NPlanes); // distance from the closest wire
    for (unsigned int iPlane = 0; iPlane < NPlanes; ++iPlane) {
      const geo::PlaneGeo& plane = TPC.Plane(iPlane);
      ThetaZ[iPlane] = plane.FirstWire().ThetaZ();
      WirePitch[iPlane] = plane.WirePitch();
      
      const double WireDistance
        = geom->WireCoordinate(y, z, iPlane, tpcid.TPC, tpcid.Cryostat);
      WireIDs.emplace_back(
        tpcid.Cryostat, tpcid.TPC, iPlane,
        (unsigned int) std::round(WireDistance)
        );
      WireDistances[iPlane]
        = (WireDistance - std::round(WireDistance)) * WirePitch[iPlane];
      
      LOG_DEBUG("GeometryTest") << "Nearest wire to"
        " (" << x << ", " << y << ", " << z << ") on plane #" << iPlane
        << " (pitch: " << WirePitch[iPlane] << ", thetaZ=" << ThetaZ[iPlane]
        << ") is " << WireIDs[iPlane] << " (position: " << WireDistance << ")";
    } // for planes
    
    // test all the combinations
    for (unsigned int iPlane1 = 0; iPlane1 < NPlanes; ++iPlane1) {
      
      const geo::WireID& w1 = WireIDs[iPlane1];
      
      for (unsigned int iPlane2 = iPlane1 + 1; iPlane2 < NPlanes; ++iPlane2) {
        const geo::WireID& w2 = WireIDs[iPlane2];
        
        geo::WireIDIntersection xing;
        if (!geom->WireIDsIntersect(w1, w2, xing)) {
          LOG_ERROR("GeometryTest") << "Wires " << w1 << " and " << w2
            << " should intersect around (" << x << ", " << y << ", " << z
            << ") of TPC " << tpcid
            << ", but they seem not to intersect at all!";
          ++nErrors;
          continue;
        }
        
        if (xing.TPC != tpcid.TPC) {
          LOG_ERROR("GeometryTest") << "Wires " << w1 << " and " << w2
            << " should intersect around (" << x << ", " << y << ", " << z
            << ") of TPC " << tpcid
            << ", but they seem to intersect in TPC #" << xing.TPC
            << " at (x, " << xing.y << "; " << xing.z << ")";
          ++nErrors;
          continue;
        }
        
        // the expected distance between the probe point (y, z) and the
        // intersection point is geometrically determined, given the distances
        // of the probe point from the two wires and the angle between the wires
        // the formula is a mix between the Carnot theorem and sine definition;
        // this value is quite sensitive to rounding errors, hence the way it's
        // coded is not how one would write it in mathematic notation
        const double dTheta = ThetaZ[iPlane1] - ThetaZ[iPlane2],
          d1 = std::abs(WireDistances[iPlane1]),
          d2 = std::abs(WireDistances[iPlane2]);
        // a bit of trick here: the angle in Carnot formula is the one between
        // the wires on the quadrant the test point falls in; that can be dTheta
        // or pi - dTheta (if the point is on the same "z size" for both the
        // wires), changing the sign of cosine
        const bool bSupplement
          = (WireDistances[iPlane1] > 0) == (WireDistances[iPlane2] > 0);
        const double expected_d = (d1 + d2)
          * std::sqrt(1. - 2. * 
            (1. - (bSupplement? -1.: 1.) * std::cos(dTheta))
            * d1 * d2 / sqr(d1 + d2))
          / std::abs(std::sin(dTheta));
        
        // the actual distance we have found:
        const double d = std::sqrt(sqr(xing.y - y) + sqr(xing.z - z));
        LOG_DEBUG("GeometryTest")
          << " - wires " << w1 << " and " << w2 << " intersect in TPC #"
          << xing.TPC << " at (x, " << xing.y << "; " << xing.z << "), "
          << d << " cm far from starting point (expected: " << expected_d << ")";
        
        // precision of the test is an issue; the 10^-3 x pitch threshold
        // is roughly tuned so that we don't get errors
        if (std::abs(d - expected_d)
          > std::max(WirePitch[iPlane1], WirePitch[iPlane2]) * 1e-3) // cm
        {
          LOG_ERROR("GeometryTest")
            << "wires " << w1 << " and " << w2 << " intersect in TPC #"
            << xing.TPC << " at (x, " << xing.y << "; " << xing.z << "), "
            << d << " cm far from starting point: too far from the expected "
            << expected_d << " cm!";
          ++nErrors;
          continue;
        } // if too far
        
      } // for iPlane2
    } // for iPlane1
    
    return nErrors;
  } // GeometryTestAlg::testWireIntersectionAt()


  //......................................................................
  void GeometryTestAlg::testWirePitch()
  {
    // loop over all planes and wires to be sure the pitch is consistent

    // hard code the value we think it should be for each detector
    double shouldbe[3];
    if(geom->DetectorName().find("argoneut") != std::string::npos){
      shouldbe[0] = 0.4;
      shouldbe[1] = 0.4;
      shouldbe[2] = 0.4; 
    }
    else if ((geom->DetectorName().find("microboone") != std::string::npos)
      || (geom->DetectorName().find("icarus") != std::string::npos)
      || (geom->DetectorName() == "lartpcdetector"))
    {
      shouldbe[0] = 0.3;
      shouldbe[1] = 0.3;
      shouldbe[2] = 0.3; 
    }
    else if(geom->DetectorName().find("lbne") != std::string::npos){
      shouldbe[0] = 0.49;
      shouldbe[1] = 0.5;
      shouldbe[2] = 0.45;  
    }
    else if(geom->DetectorName().find("bo") != std::string::npos){	
      shouldbe[0] = 0.46977;
      shouldbe[1] = 0.46977;
      shouldbe[2] = 0.46977; 
    }
    else {
      throw cet::exception("UnexpectedWirePitch")
        << "Can't check wire pitch for detector '" << geom->DetectorName()
        << "', since I don't know what to expect.";
    }

    for(size_t c = 0; c < geom->Ncryostats(); ++c){
      for(size_t t = 0; t < geom->Cryostat(c).NTPC(); ++t){
        for(size_t p = 0; p < geom->Cryostat(c).TPC(t).Nplanes(); ++p){
	  for(size_t w = 0; w < geom->Cryostat(c).TPC(t).Plane(p).Nwires()-1; ++w){
	    // get the wire pitch
	    double pitch = geom->Cryostat(c).TPC(t).WirePitch(w, w+1, p);
	    if(std::abs(pitch - shouldbe[p]) > 0.01*shouldbe[p]){
	      throw cet::exception("UnexpectedWirePitch") << "\n\tpitch is " 
	 						  << pitch << " instead of " << shouldbe[p] 
							  << " for cryostat " << c
							  << ", tpc " << t
							  << ", plane " << p
							  << "; wires: " << w << ", " << w+1 << "\n";
	    }// end if pitch is wrong
	  }// end loop over wires
        }// end loop over planes
      }// end loop over TPCs
    }// end loop over cryostats

  }

  //......................................................................
  void GeometryTestAlg::testPlanePitch()
  {
    // loop over all planes to be sure the pitch is consistent

    // hard code the value we think it should be for each detector
    double shouldbe = 0.4; // true for ArgoNeuT
    if((geom->DetectorName().find("microboone") != std::string::npos)
      || (geom->DetectorName() == "lartpcdetector"))
      shouldbe = 0.3;
    else if(geom->DetectorName().find("lbne") != std::string::npos)   shouldbe = 0.5;
    else if(geom->DetectorName().find("bo") != std::string::npos)     shouldbe = 0.65;
    else if(geom->DetectorName().find("icarus") != std::string::npos) shouldbe = 0.476;
    else {
      throw cet::exception("UnexpectedPlanePitch")
        << "Can't check plane pitch for detector '" << geom->DetectorName()
        << "', since I don't know what to expect.";
    }

    for(size_t t = 0; t < geom->NTPC(); ++t){
      for(size_t p = 0; p < geom->TPC(t).Nplanes()-1; ++p){
	double pitch = std::abs(geom->TPC(t).PlanePitch(p, p+1));
	if(std::abs(pitch - shouldbe) > 0.1*shouldbe){
	  throw cet::exception("UnexpectedPlanePitch") << "\n\tunexpected pitch: " 
						       << pitch << "/" << shouldbe << "\n"; 
	}// end if wrong pitch
      }// end loop over planes
    }// end loop over TPCs

  }

  //......................................................................

  void GeometryTestAlg::testStepping()
  {
    //
    // Test stepping. Example is similar to what one would do for photon
    // transport. Rattles photons around inside the scintillator
    // bouncing them off walls.
    //
    double xyz[3]      = {0.};
    double xyzWire[3]  = {0.};
    double dxyz[3]     = {0.};
    double dxyzWire[3] = {0, sin(0.1), cos(0.1)};

    geom->Plane(1).Wire(0).LocalToWorld(xyzWire,xyz);
    geom->Plane(1).Wire(0).LocalToWorldVect(dxyzWire,dxyz);

    mf::LogVerbatim("GeometryTest") << "\n\t" << xyz[0]  << "\t" << xyz[1]  << "\t" << xyz[2] ;
    mf::LogVerbatim("GeometryTest") << "\t"   << dxyz[0] << "\t" << dxyz[1] << "\t" << dxyz[2];

    gGeoManager->InitTrack(xyz, dxyz);
    for (int i=0; i<10; ++i) {
      const double* pos = gGeoManager->GetCurrentPoint();
      const double* dir = gGeoManager->GetCurrentDirection();
      mf::LogVerbatim("GeometryTest") << "\tnode = " 
				      << gGeoManager->GetCurrentNode()->GetName()
				      << "\n\t\tpos=" << "\t"
				      << pos[0] << "\t"
				      << pos[1] << "\t"
				      << pos[2]
				      << "\n\t\tdir=" << "\t"
				      << dir[0] << "\t"
				      << dir[1] << "\t"
				      << dir[2]
				      << "\n\t\tmat = " 
				      << gGeoManager->GetCurrentNode()->GetVolume()->GetMaterial()->GetName();
      
      gGeoManager->FindNextBoundary();
      gGeoManager->FindNormal();
      gGeoManager->Step(kTRUE,kTRUE);
    }

    xyz[0] = 306.108; xyz[1] = -7.23775; xyz[2] = 856.757;
    gGeoManager->InitTrack(xyz, dxyz);
    mf::LogVerbatim("GeometryTest") << "\tnode = " 
				    << gGeoManager->GetCurrentNode()->GetName()
				    << "\n\tmat = " 
				    << gGeoManager->GetCurrentNode()->GetVolume()->GetMaterial()->GetName();

    gGeoManager->GetCurrentNode()->GetVolume()->GetMaterial()->Print();

  }

  //......................................................................

  void GeometryTestAlg::testProject() 
  {
    double xlo, xhi;
    double ylo, yhi;
    double zlo, zhi;
    geom->WorldBox(&xlo, &xhi, &ylo, &yhi, &zlo, &zhi);
  
    double xyz[3]   = { 0.0, 0.0, 0.0};
    double dxyz1[3] = { 1.0, 0.0, 0.0};
    double dxyz2[3] = {-1.0, 0.0, 0.0};
    double dxyz3[3] = { 0.0, 1.0, 0.0};
    double dxyz4[3] = { 0.0,-1.0, 0.0};
    double dxyz5[3] = { 0.0, 0.0, 1.0};
    double dxyz6[3] = { 0.0, 0.0,-1.0};

    double xyzo[3];
    geo::ProjectToBoxEdge(xyz, dxyz1, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[0]-xhi)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz2, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[0]-xlo)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz3, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[1]-yhi)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz4, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[1]-ylo)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz5, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[2]-zhi)>1.E-6) abort();

    geo::ProjectToBoxEdge(xyz, dxyz6, xlo, xhi, ylo, yhi, zlo, zhi, xyzo);
    if (std::abs(xyzo[2]-zlo)>1.E-6) abort();
  }

  
  //......................................................................
  
  inline bool GeometryTestAlg::shouldRunTests(std::string test_name) const {
    return fRunTests.empty() || (fRunTests.count(test_name) > 0);
  } // GeometryTestAlg::shouldRunTests()

}//end namespace
