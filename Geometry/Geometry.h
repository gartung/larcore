////////////////////////////////////////////////////////////////////////
/// \file  Geometry.h
/// \brief Encapsulate the geometry of one entire detector
///
/// \version $Id: Geometry.h,v 1.16 2009/11/03 22:53:20 brebel Exp $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
///
/// Revised <seligman@nevis.columbia.edu> 29-Jan-2009
///         Revise the class to make it into more of a general detector
///         interface.
///

#ifndef GEO_GEOMETRY_H
#define GEO_GEOMETRY_H

#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/ChannelMapAlg.h"
#include "Geometry/ExptGeoHelperInterface.h"

#include <TString.h>
#include <TVector3.h>
#include <Rtypes.h>

#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <memory>
#include <stdint.h>

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"

class TGeoManager;
class TGeoVolume;
class TGeoNode;
class TGeoMaterial;
class TGeoHMatrix;

namespace geo {

  // Foward declarations within namespace.
  class CryostatGeo;
  class TPCGeo;
  class PlaneGeo;
  class WireGeo;
  class AuxDetGeo;
  
  // The geometry of one entire detector.
  class Geometry
  {
  public:

    // Access to the single instance of thie class.  For example:
    Geometry(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);
    ~Geometry();

    void preBeginRun(art::Run const& run);

    // Number of readout channels in the detector
    uint32_t     Nchannels()                                      const;
    // Number of cryostats in the detector
    unsigned int Ncryostats()                                     const { return fCryostats.size();}
    // Number of TPCs in the detector
    unsigned int NTPC(unsigned int cstat = 0)                     const;
    // Number of OpDet in the detector (for particular cryostat)
    unsigned int NOpDet(unsigned int cstat = 0)                   const;
    // Number of OpChannels in the detector
    unsigned int NOpChannels()                                    const;
    // Number of views (different wire orientations) in the detector
    unsigned int Nviews()                                         const;
    // Number of wire planes in TPC "tpc" of cryostat "cstat".
    unsigned int Nplanes(unsigned int tpc   = 0,
			 unsigned int cstat = 0)                  const;
    // Number of wires in plane "p" of TPC "tpc" of cryostat "cstat".
    unsigned int Nwires(unsigned int p,
			unsigned int tpc   = 0,
			unsigned int cstat = 0)                   const;
    // Number of scintillator paddles (Auxiliary Detectors aka AuxDet) outside of the cryostat
    unsigned int NAuxDets()                                       const { return fAuxDets.size(); }

    const CryostatGeo&  Cryostat(unsigned int const cstat = 0)    const;
    const TPCGeo&       TPC(unsigned int const tpc   = 0,
			    unsigned int const cstat = 0)         const;
    const TPCGeo&       PositionToTPC(double const  worldLoc[3],
				      unsigned int &tpc,
				      unsigned int &cstat)        const; // return the TPCGeo object containing
                                                                         // the world position worldLoc
    const CryostatGeo&  PositionToCryostat(double const  worldLoc[3],
					   unsigned int &cstat)   const; // return the CryostatGeo object containing
                                                                         // the world position worldLoc
    const PlaneGeo&     Plane(unsigned int const p,
			      unsigned int const tpc   = 0,
			      unsigned int const cstat = 0)       const;

    const AuxDetGeo&    AuxDet(unsigned int const ad = 0)         const;
    const AuxDetGeo&    PositionToAuxDet(double const  worldLoc[3],
                                            unsigned int &ad)     const;    // return the AuxDetGeo object containing
                                                                                // the world position worldLoc

    std::vector< geo::WireID > ChannelToWire(uint32_t const channel) const; // convert channel number to
                                                                            // list of possible
                                                                            // WireIDs

    const SigType_t     SignalType(uint32_t     const channel)    const; // return the signal type for a given channel
    const SigType_t     SignalType(geo::PlaneID const pid)        const; // return the signal type for a given channel
    const View_t        View(uint32_t   const channel)            const; // return the view type for a given channel
    const View_t        View(geo::PlaneID const pid)              const; // return the view type for a given channel
    std::set<View_t>  const& Views()                              const; // return vector of possible views in the detector
    std::set<PlaneID> const& PlaneIDs()                           const; // return vector of possible PlaneIDs in the detector

    uint32_t            PlaneWireToChannel(unsigned int const plane,
					   unsigned int const wire,
					   unsigned int const tpc = 0,
					   unsigned int const cstat = 0) const; // convert plane, wire to channel

    uint32_t            PlaneWireToChannel(WireID const& wireid)  const;

    //  assuming heirachical numbering scheme
    uint32_t            NearestChannel(const double worldLoc[3],
				       unsigned int const PlaneNo,
				       unsigned int const TPCNo = 0,
				       unsigned int const cstat = 0) const; // find the nearest channel to
                                                                            // input world coordinates
    uint32_t            NearestChannel(std::vector<double> const worldLoc,
				       unsigned int const PlaneNo,
				       unsigned int const TPCNo = 0,
				       unsigned int const cstat = 0) const; // find the nearest channel to
                                                                            // input world coordinates
    uint32_t            NearestChannel(const TVector3& worldLoc,
				       unsigned int const PlaneNo,
				       unsigned int const TPCNo = 0,
				       unsigned int const cstat = 0) const; // find the nearest channel to
                                                                            // input world coordinates
    const geo::WireID   NearestWireID(const double worldLoc[3],
				      unsigned int const PlaneNo,
				      unsigned int const TPCNo = 0,
				      unsigned int const cstat = 0)  const; // nearest wire to input
                                                                            // world coordinates
    const geo::WireID   NearestWireID(std::vector<double> worldLoc,
				      unsigned int const PlaneNo,
				      unsigned int const TPCNo = 0,
				      unsigned int const cstat = 0)  const; // nearest wire to input
                                                                            // world coordinate
    const geo::WireID   NearestWireID(const TVector3& worldLoc,
				      unsigned int const PlaneNo,
				      unsigned int const TPCNo = 0,
				      unsigned int const cstat = 0)  const; // nearest wire to input
                                                                            // world coordinates

    unsigned int       NearestWire(const double worldLoc[3],
				   unsigned int const PlaneNo,
				   unsigned int const TPCNo = 0,
				   unsigned int const cstat = 0)  const; // nearest wire to input
                                                                         // world coordinates
    unsigned int       NearestWire(std::vector<double> worldLoc,
				   unsigned int const PlaneNo,
				   unsigned int const TPCNo = 0,
				   unsigned int const cstat = 0)  const; // nearest wire to input
                                                                         // world coordinate
    unsigned int       NearestWire(const TVector3& worldLoc,
				   unsigned int const PlaneNo,
				   unsigned int const TPCNo = 0,
				   unsigned int const cstat = 0)  const; // nearest wire to input
                                                                         // world coordinates

    const TGeoMaterial* Material(double x,
				 double y,
				 double z)                        const;
    double              DetHalfWidth(unsigned int tpc = 0,
				     unsigned int cstat = 0)      const; // half width of the TPC
    double              DetHalfHeight(unsigned int tpc = 0,
				      unsigned int cstat = 0)     const; // half height of the TPC
    double              DetLength(unsigned int tpc = 0,
				  unsigned int cstat = 0)         const; // length of the TPC
    double              CryostatHalfWidth(unsigned int cstat = 0) const; // half width of the cryostat
    double              CryostatHalfHeight(unsigned int cstat = 0)const; // half height of the cryostat
    double              CryostatLength(unsigned int cstat = 0)    const; // length of the cryostat
    void                CryostatBoundaries(double* boundaries,
					   unsigned int cstat = 0)const; // boundaries of cryostat, 3 pairs of +/- coord
    double              PlanePitch(unsigned int p1 = 0,                  // distance between planes
				   unsigned int p2 = 1,
				   unsigned int tpc = 0,
				   unsigned int cstat = 0)        const; // p1 < p2
    double              WirePitch(unsigned int w1 = 0,                   // distance between wires
				  unsigned int w2 = 1,                   // on the same plane
				  unsigned int plane = 0,
				  unsigned int tpc = 0,
				  unsigned int cstat = 0)         const; // w1 < w2
    double              WirePitch(geo::View_t view)               const; // assumes all planes in
                                                                         // a view have the same pitch
    double              WireAngleToVertical(geo::View_t view)     const; // assumes all wires in the
                                                                         // view have the same angle

    void                WorldBox(double* xlo,
				 double* xhi,
				 double* ylo,
				 double* yhi,
				 double* zlo,
				 double* zhi)                     const; // volume box
    double              TotalMass(const char* vol="volWorld")     const; // total mass of the
                                                                         // specified volume
    double              MassBetweenPoints(double *p1,
					  double *p2)             const; // mass between two points
                                                                         // in the world

    // A typical y-position value at the surface (where earth meets air)
    // for this detector site
    //
    // \returns typical y position at surface in units of cm
    double              SurfaceY()                                const { return fSurfaceY; }

    // Access to the ROOT geometry description.
    TGeoManager*        ROOTGeoManager()                          const;

    // The full directory path to the GDML file that was the source
    // of the detector geometry.
    std::string         GetGDMLPath()                             const { return fGDMLfile; }
    std::string         GetROOTPath()                             const { return fROOTfile; }
    std::string         ROOTFile()                                const { return fROOTfile; }
    std::string         GDMLFile()                                const { return fGDMLfile; }
    // The name of the detector.
    const TString       GetDetectorName()                         const { return fDetectorName; }

    // There are some issues that require detector-specific queries.
    // This method returns an enumerated type that can be tested in those cases
    geo::DetId_t DetId()                                          const {return fDetId; }


    // The Geant4 simulation needs to know the name of the world volume.
    const std::string GetWorldVolumeName()                        const;

    // The Geant4 simulation needs to know the name of the LAr TPC volume.
    const std::string GetLArTPCVolumeName(unsigned int const tpc = 0,
					  unsigned int const cstat = 0) const;

    // The event display needs to know the name of the cryostat.
    const std::string GetCryostatVolumeName(unsigned int const cstat = 0)const;

    // As of Aug-2009, the origin of the co-ordinate system for
    // ArgoNEUT and MicroBooNE is for z=0 and y=0 to be at the center
    // of the front face of the detector, but x=0 to be the edge of
    // the TPC.  This is convenient for read-out, but a pain for
    // simulation.  This method returns the center of the front face
    // of the TPC in the world co-ordinate system, making it easier
    // to write detector-independent simulation code.
    const TVector3 GetTPCFrontFaceCenter(unsigned int tpc = 0,
					 unsigned int cstat = 0)  const;

    // Name of the deepest volume containing the point xyz
    // returns volume containing the origin by default
    const std::string VolumeName(TVector3 point);

    // Name of the deepest material containing the point xyz
    // returns material of the origin by default
    const std::string MaterialName(TVector3 point);

    // The following functions are utilized to determine if two wires
    // in the TPC intersect or not, and if they do then
    // determine the coordinates of the intersection.
    // Starting point of wire is end with lower z-coordinate.
    bool ValueInRange(double value,
		      double min,
		      double max);
    void WireEndPoints(unsigned int cstat,
		       unsigned int tpc,
		       unsigned int plane,
		       unsigned int wire,
		       double *xyzStart,
		       double *xyzEnd);
    bool ChannelsIntersect(uint32_t c1,
			   uint32_t c2,
			   double &y,
			   double &z);
    bool WireIDsIntersect(WireID wid1,
			  WireID wid2,
			  WireIDIntersection & widIntersect);
    void IntersectionPoint(unsigned int wire1,
			   unsigned int wire2,
			   unsigned int plane1,
			   unsigned int plane2,
			   unsigned int cstat,
			   unsigned int tpc,
                           double start_w1[3],
			   double end_w1[3],
			   double start_w2[3],
			   double end_w2[3],
                           double &y,
			   double &z);
    void IntersectionPoint(unsigned int wire1,
			   unsigned int wire2,
			   unsigned int plane1,
			   unsigned int plane2,
			   unsigned int cstat,
			   unsigned int tpc,
                           double &y,
			   double &z);

    // Return gdml string which gives sensitive opdet name
    std::string            OpDetGeoName(unsigned int c=0) const;

    // Convert OpDet, Cryo into OpChannel
    int         OpDetCryoToOpChannel(unsigned int o, 
				     unsigned int c=0) const;

    // Convert OpChannel into Cryo and OpDet
    void        OpChannelToCryoOpDet(unsigned int OpChannel, 
				     unsigned int& o, 
				     unsigned int & c) const;

    // Find the nearest OpChannel to some point, in the appropriate cryostat
    unsigned int  GetClosestOpChannel(double * xyz) const;

    const WireGeo& WireIDToWireGeo(WireID CodeWire) const;


  private:

    void LoadGeometryFile(std::string gdmlfile,
			  std::string rootfile);
    void InitializeChannelMap();

    void FindCryostat(std::vector<const TGeoNode*>& path,
		      unsigned int depth);
    void MakeCryostat(std::vector<const TGeoNode*>& path,
		      int depth);
    void FindAuxDet(std::vector<const TGeoNode*>& path,
                    unsigned int depth);
    void MakeAuxDet(std::vector<const TGeoNode*>& path,
                    int depth);

    std::vector<CryostatGeo*> fCryostats;        ///< The detector cryostats
    std::vector<AuxDetGeo*>   fAuxDets;          ///< The auxiliary detectors

    double                    fSurfaceY;         ///< The point where air meets earth for this detector.
    TString                   fDetectorName;     ///< Name of the detector.
    std::string               fGDMLfile;         ///< The GDML file used for the detector geometry
    std::string               fROOTfile;         ///< The GDML file used for the detector geometry
    std::string               fRelPath;          ///< Relative path added to FW_SEARCH_PATH to search for 
                                                 ///< geometry file
    double                    fMinWireZDist;     ///< Minimum distance in Z from a point in which
                                                 ///< to look for the closest wire
    bool                      fDisableWiresInG4; ///< If set true, supply G4 with GDMLfileNoWires
                                                 ///< rather than GDMLfile
    bool                      fForceUseFCLOnly;  ///< Force Geometry to only use the geometry
                                                 ///< files specified in the fcl file
    geo::DetId_t              fDetId;            ///< Detector type
    double                    fPositionWiggle;   ///< accounting for rounding errors when testing positions
    std::shared_ptr<const geo::ChannelMapAlg>
                              fChannelMapAlg;    ///< Object containing the channel to wire mapping
    fhicl::ParameterSet       fSortingParameters;///< Parameter set to define the channel map sorting
    art::ServiceHandle<geo::ExptGeoHelperInterface>
                              fExptGeoHelper;    ///< Expt-specific geometry helper service
  };

} // namespace geo

DECLARE_ART_SERVICE(geo::Geometry, LEGACY)
#endif // GEO_GEOMETRY_H
