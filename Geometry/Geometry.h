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
#include "SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t
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
#include <iterator> // std::forward_iterator_tag

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
    unsigned int Nchannels()                                      const;
    // Number of cryostats in the detector
    unsigned int Ncryostats()                                     const { return fCryostats.size();}
    // Number of TPCs in the detector
    unsigned int NTPC(unsigned int cstat = 0)                     const;
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
    const TPCGeo&       TPC(const geo::TPCID& tpcid)              const
      { return TPC(tpcid.TPC, tpcid.Cryostat); }


    /**
     * @brief Returns the ID of the TPC at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the TPC ID, or an invalid one if no TPC is there
     */
    geo::TPCID FindTPCAtPosition(double const worldLoc[3]) const;
    
    const TPCGeo&       PositionToTPC(double const  worldLoc[3],
				      unsigned int &tpc,
				      unsigned int &cstat)        const; // return the TPCGeo object containing
                                                                         // the world position worldLoc
    /**
     * @brief Returns the index of the cryostat at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the index of the cryostat, or UINT_MAX if no cryostat is there
     */
    unsigned int FindCryostatAtPosition(double const worldLoc[3]) const;
    
    const CryostatGeo&  PositionToCryostat(double const  worldLoc[3],
					   unsigned int &cstat)   const; // return the CryostatGeo object containing
                                                                         // the world position worldLoc
    const PlaneGeo&     Plane(unsigned int const p,
			      unsigned int const tpc   = 0,
			      unsigned int const cstat = 0)       const;
    
    const PlaneGeo&     Plane(const geo::PlaneID& pid)            const
      { return Plane(pid.Plane, pid.TPC, pid.Cryostat); }

    const AuxDetGeo&    AuxDet(unsigned int const ad = 0)         const;
    
    /**
     * @brief Returns the index of the auxiliary detector at specified location
     * @param worldLoc 3D coordinates of the point (world reference frame)
     * @return the index of the detector, or UINT_MAX if no detector is there
     */
    unsigned int FindAuxDetAtPosition(double const worldLoc[3]) const;
    
    const AuxDetGeo&    PositionToAuxDet(double const  worldLoc[3],
                                            unsigned int &ad)     const;    // return the AuxDetGeo object containing
                                                                                // the world position worldLoc

    std::vector< geo::WireID > ChannelToWire(raw::ChannelID_t const channel) const; // convert channel number to
                                                                            // list of possible
                                                                            // WireIDs

    SigType_t         SignalType(raw::ChannelID_t const channel)    const; // return the signal type for a given channel
    SigType_t         SignalType(geo::PlaneID const pid)        const; // return the signal type for a given channel
    View_t            View(raw::ChannelID_t const channel)            const; // return the view type for a given channel
    View_t            View(geo::PlaneID const pid)              const; // return the view type for a given channel
    std::set<View_t>  const& Views()                              const; // return vector of possible views in the detector
    std::set<PlaneID> const& PlaneIDs()                           const; // return vector of possible PlaneIDs in the detector

    raw::ChannelID_t  PlaneWireToChannel(unsigned int const plane,
					   unsigned int const wire,
					   unsigned int const tpc = 0,
					   unsigned int const cstat = 0) const; // convert plane, wire to channel

    raw::ChannelID_t  PlaneWireToChannel(WireID const& wireid)  const;

    //  assuming heirachical numbering scheme
    raw::ChannelID_t  NearestChannel(const double worldLoc[3],
				       unsigned int const PlaneNo,
				       unsigned int const TPCNo = 0,
				       unsigned int const cstat = 0) const; // find the nearest channel to
                                                                            // input world coordinates
    raw::ChannelID_t  NearestChannel(std::vector<double> const worldLoc,
				       unsigned int const PlaneNo,
				       unsigned int const TPCNo = 0,
				       unsigned int const cstat = 0) const; // find the nearest channel to
                                                                            // input world coordinates
    raw::ChannelID_t  NearestChannel(const TVector3& worldLoc,
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
    double WireCoordinate(double YPos, double ZPos,
                          unsigned int PlaneNo,
                          unsigned int TPCNo,
                          unsigned int cstat) const;

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
    double              WireAngleToVertical(geo::View_t view, int TPC=0, int Cryo=0)     const; // assumes all wires in the
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
    std::string         ROOTFile()                                const { return fROOTfile; }
    std::string         GDMLFile()                                const { return fGDMLfile; }
    // The name of the detector.
    std::string         DetectorName()                            const { return std::string(fDetectorName); }

    // There are some issues that require detector-specific queries.
    // This method returns an enumerated type that can be tested in those cases
    //geo::DetId_t DetId()                                          const {return fDetId; }


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
		      double max) const;
    void WireEndPoints(unsigned int cstat,
		       unsigned int tpc,
		       unsigned int plane,
		       unsigned int wire,
		       double *xyzStart,
		       double *xyzEnd) const;
    bool ChannelsIntersect(raw::ChannelID_t c1,
			   raw::ChannelID_t c2,
			   double &y,
			   double &z);
    bool WireIDsIntersect(const WireID& wid1,
			  const WireID& wid2,
			  WireIDIntersection & widIntersect) const;
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

    // Given a slope dTime/dWire in two planes, return with the slope in the 3rd plane
    double ThirdPlaneSlope(unsigned int plane1, double slope1, 
                           unsigned int plane2, double slope2,
                           unsigned int tpc, unsigned int cstat);

    
    ////////////////////////////////
    // Optical Detector functions //
    ////////////////////////////////

    // Number of OpDets in the whole detector
    unsigned int NOpDets()                                        const;

    // Number of electronics channels for all the optical detectors
    unsigned int NOpChannels()                                    const;

    // Number of hardware channels for a given optical detector
    unsigned int NOpHardwareChannels(int opDet)                   const;

    // Convert detector number and hardware channel to unique channel
    // and vice versa
    unsigned int OpChannel(int detNum, int hardwareChannel)       const;
    unsigned int OpDetFromOpChannel(int opChannel)                const;
    unsigned int HardwareChannelFromOpChannel(int opChannel)      const;

    // Is this a valid OpChannel number?
    bool IsValidOpChannel(int opChannel)                          const;

    // Get unique opdet number from cryo and internal count
    unsigned int OpDetFromCryo(unsigned int o, unsigned int c )   const;

    // Access the OpDetGeo object by OpDet or Channel Number
    const OpDetGeo& OpDetGeoFromOpChannel(unsigned int OpChannel) const;
    const OpDetGeo& OpDetGeoFromOpDet(unsigned int OpDet)         const;


    // Return gdml string which gives sensitive opdet name
    std::string            OpDetGeoName(unsigned int c=0) const;


    // Find the nearest OpChannel to some point, in the appropriate cryostat
    unsigned int  GetClosestOpDet(double * xyz) const;

    const WireGeo& WireIDToWireGeo(WireID CodeWire) const;

    
    
    /** ************************************************************************
     * @brief Forward iterator browsing all cryostats in the detector
     * 
     * Usage example:
     * @code
     * geo::Geometry::cryostat_iterator iCryostat;
     * while (iCryostat) {
     *   std::cout << "Cryo: " << iCryostat->Cryostat << std::endl;
     *   const geo::CryostatGeo* pCryo = iCryostat.get();
     *   // ...
     *   ++iCryostat;
     * } // while
     * @endcode
     */
    class cryostat_iterator: public std::forward_iterator_tag {
        public:
      typedef unsigned int CryoID; /// type of cryostat ID
      
      /// Default constructor: points to the first cryostat
      cryostat_iterator() { init_geometry(); set_limits_and_validity(); }
      
      /// Constructor: points to the specified cryostat
      cryostat_iterator(CryoID start_from): cryoid(start_from)
        { init_geometry(); set_limits_and_validity(); }
      
      bool operator== (const cryostat_iterator& as) const
        { return cryoid == as.cryoid; }
      bool operator!= (const cryostat_iterator& as) const
        { return cryoid != as.cryoid; }
      
      /// Returns a copy of the TPCID the iterator points to
      CryoID operator* () const { return cryoid; }
      
      /// Prefix increment: returns this iterator pointing to the next cryostat
      cryostat_iterator& operator++ ();
      
      /// Prefix decrement: returns this iterator pointing to the previous cryostat
      cryostat_iterator& operator-- ();
      
      /// Postfix increment: returns the current iterator, then increments it
      cryostat_iterator operator++ (int)
        { cryostat_iterator old(*this); this->operator++(); return old; }
      
      /// Postfix decrement: returns the current iterator, then decrements it
      cryostat_iterator operator-- (int)
        { cryostat_iterator old(*this); this->operator--(); return old; }
      
      /// Returns whether the iterator is pointing to a valid cryostat
      operator bool() const { return isValid; }
      
      /// Skips to the next cryostat
      cryostat_iterator& next() { return this->operator++(); }
      
      /// Returns a pointer to cryostat, or nullptr if the iterator is not valid
      const CryostatGeo* get() const;
      
        protected:
      const Geometry* pGeo; ///< pointer to the geometry
      
      bool isValid = false;  ///< whether the current iterator position is valid
      CryoID cryoid = { 0 }; ///< current cryostat
      CryoID limits = { 0 }; ///< maxima of the indices
      
      void init_geometry();
      void set_limits_and_validity();
    }; // class cryostat_iterator
    
    
    
    /** ************************************************************************
     * @brief Forward iterator browsing all TPCs in the detector
     * 
     * Usage example:
     * @code
     * geo::Geometry::TPC_iterator iTPC;
     * while (iTPC) {
     *   std::cout << "Cryo: " << iTPC->Cryostat << " TPC: " << iTPC->TPC
     *     << std::endl;
     *   const geo::TPCGeo* pTPC = iTPC.get();
     *   // ...
     *   ++iTPC;
     * } // while
     * @endcode
     */
    class TPC_iterator: public std::forward_iterator_tag {
        public:
      /// Default constructor: points to the first TPC
      TPC_iterator() { init_geometry(); set_limits_and_validity(); }
      
      /// Constructor: points to the specified TPC
      TPC_iterator(TPCID start_from): tpcid(start_from)
        { init_geometry(); set_limits_and_validity(); }
      
      bool operator== (const TPC_iterator& as) const
        { return tpcid == as.tpcid; }
      bool operator!= (const TPC_iterator& as) const
        { return tpcid != as.tpcid; }
      
      /// Returns a copy of the TPCID the iterator points to
      const TPCID& operator* () const { return tpcid; }
      
      /// Returns a constant pointer to the TPCID the iterator points to
      const TPCID* operator-> () const { return &tpcid; }
      
      /// Prefix increment: returns this iterator pointing to the next TPC
      TPC_iterator& operator++ ();
      
      /// Postfix increment: returns the current iterator, then increments it
      TPC_iterator operator++ (int)
        { TPC_iterator old(*this); this->operator++(); return old; }
      
      /// Returns whether the iterator is pointing to a valid TPC
      operator bool() const { return tpcid.isValid; }
      
      /// Skips to the next TPC
      TPC_iterator& next() { return this->operator++(); }
      
      /// Returns a pointer to the TPC, or nullptr if the iterator is not valid
      const TPCGeo* get() const;
      
      /// Returns a pointer to the cryostat the plane belongs to
      const CryostatGeo* getCryostat() const;
      
        protected:
      const Geometry* pGeo; ///< pointer to the geometry
      
      TPCID tpcid = { 0, 0 }; ///< current TPC
      TPCID limits = { 0, 0 }; ///< maxima of the indices
      
      void init_geometry();
      void set_limits_and_validity();
      void new_cryostat();
    }; // class TPC_iterator
  
  
    /** ************************************************************************
     * @brief Forward iterator browsing all planes in the detector
     * 
     * Usage example:
     * @code
     * geo::Geometry::plane_iterator iPlane;
     * while (iPlane) {
     *   std::cout << "Cryo: " << iPlane->Cryostat << " TPC: " << iPlane->TPC
     *     << " plane: " << iPlane->Plane << std::endl;
     *   const geo::PlaneGeo* Plane = iPlane.get();
     *   // ...
     *   ++iPlane;
     * } // while
     * @endcode
     */
    class plane_iterator: public std::forward_iterator_tag {
        public:
      /// Default constructor: points to the first plane
      plane_iterator() { init_geometry(); set_limits_and_validity(); }
      
      /// Constructor: points to the specified plane
      plane_iterator(PlaneID start_from): planeid(start_from)
        { init_geometry(); set_limits_and_validity(); }
      
      bool operator== (const plane_iterator& as) const
        { return planeid == as.planeid; }
      bool operator!= (const plane_iterator& as) const
        { return planeid != as.planeid; }
      
      /// Returns a copy of the PlaneID the iterator points to
      const PlaneID& operator* () const { return planeid; }
      
      /// Returns a constant pointer to the PlaneID the iterator points to
      const PlaneID* operator-> () const { return &planeid; }
      
      /// Prefix increment: returns this iterator pointing to the next plane
      plane_iterator& operator++ ();
      
      /// Postfix increment: returns the current iterator, then increments it
      plane_iterator operator++ (int)
        { plane_iterator old(*this); this->operator++(); return old; }
      
      /// Returns whether the iterator is pointing to a valid plane
      operator bool() const { return planeid.isValid; }
      
      /// Skips to the next plane
      plane_iterator& next() { return this->operator++(); }
      
      /// Returns a pointer to plane, or nullptr if the iterator is not valid
      const PlaneGeo* get() const;
      
      /// Returns a pointer to the TPC the plane belongs to
      const TPCGeo* getTPC() const;
      
      /// Returns a pointer to the cryostat the plane belongs to
      const CryostatGeo* getCryostat() const;
      
        protected:
      const Geometry* pGeo; ///< pointer to the geometry
      
      PlaneID planeid = { 0, 0, 0 }; ///< current plane
      PlaneID limits = { 0, 0, 0 }; ///< maxima of the indices
      
      void init_geometry();
      void set_limits_and_validity();
      void new_cryostat();
      void new_tpc();
    }; // class plane_iterator
    
    
    /** ************************************************************************
     * @brief Forward iterator browsing all wires in the detector
     * 
     * Usage example:
     * @code
     * geo::Geometry::wire_iterator iWire;
     * while (iWire) {
     *   std::cout << "Cryo: " << iWire->Cryostat << " TPC: " << iWire->TPC
     *     << " plane: " << iWire->Plane << " << " wire: " << iWire->Wire
     *     << std::endl;
	  *   const geo::WireGeo* Wire = iWire.get();
     *   // ...
     *   ++iWire;
     * } // while
     * @endcode
     */
    class wire_iterator: public std::forward_iterator_tag {
        public:
      /// Default constructor: points to the first wire
      wire_iterator() { init_geometry(); set_limits_and_validity(); }
      
      /// Constructor: points to the specified wire
      wire_iterator(WireID start_from): wireid(start_from)
        { init_geometry(); set_limits_and_validity(); }
      
      bool operator== (const wire_iterator& as) const
        { return wireid == as.wireid; }
      bool operator!= (const wire_iterator& as) const
        { return wireid != as.wireid; }
      
      /// Returns a copy of the WireID the iterator points to
      const WireID& operator* () const { return wireid; }
      
      /// Returns a constant pointer to the WireID the iterator points to
      const WireID* operator-> () const { return &wireid ; }
      
      /// Prefix increment: returns this iterator pointing to the next wire
      wire_iterator& operator++ ();
      
      /// Postfix increment: returns the current iterator, then increments it
      wire_iterator operator++ (int)
        { wire_iterator old(*this); this->operator++(); return old; }
      
      /// Returns whether the iterator is pointing to a valid wire
      operator bool() const { return wireid.isValid; }
      
      /// Skips to the next wire
      wire_iterator& next() { return this->operator++(); }
      
      /// Returns a pointer to wire, or nullptr if the iterator is not valid
      const WireGeo* get() const;
      
      /// Returns a pointer to the plane the wire belongs to
      const PlaneGeo* getPlane() const;
      
      /// Returns a pointer to the TPC the wire belongs to
      const TPCGeo* getTPC() const;
      
      /// Returns a pointer to the cryostat the wire belongs to
      const CryostatGeo* getCryostat() const;
      
        protected:
      const Geometry* pGeo; ///< pointer to the geometry
      
      WireID wireid = { 0, 0, 0, 0 }; ///< current wire
      WireID limits = { 0, 0, 0, 0 }; ///< maxima of the indices
      
      void init_geometry();
      void set_limits_and_validity();
      void new_cryostat();
      void new_tpc();
      void new_plane();
    }; // class wire_iterator
    
    
    
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
