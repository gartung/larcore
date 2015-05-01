#ifndef GEO_TYPES_H
#define GEO_TYPES_H

#include <climits>
#include <cmath>
#include <limits> // std::numeric_limits<>

namespace geo {
  typedef enum coordinates {
    kXCoord, ///< X coordinate
    kYCoord, ///< Y coordinate
    kZCoord  ///< Z coordinate
  } Coord_t;

  typedef enum detid {
    kBo,          ///< Bo id
    kArgoNeuT,    ///< ArgoNeuT id
    kMicroBooNE,  ///< MicroBoone id
    kLBNE10kt,    ///< LBNE 10kt id
    kJP250L,      ///< JPARC 250 L id
    kLBNE35t,     ///< 35t prototype id
    kLBNE34kt,    ///< LBNE 34kt id
    kCSU40L,      ///< CSU 40 L id
    kLArIAT,	  ///< LArIAT id
    kICARUS,      ///< ICARUS T600 id
    kUnknownDetId ///< unknown detector id
  } DetId_t;

  /// Enumerate the possible plane projections
  typedef enum _plane_proj {
    kU,       ///< planes which measure U
    kV,       ///< planes which measure V
    kW,       ///< soon to be deprecated, planes which measure W (third view for Bo, MicroBooNE, etc)
    kZ=kW,    ///< planes which measure Z direction (ie wires are vertical)
    k3D,      ///< 3 dimensional objects, potentially hits, clusters, prongs, etc
    kUnknown  ///< unknown view
  } View_t;

  typedef enum _plane_orient {
    kHorizontal, ///< planes that are in the horizontal plane (depricated as of 8/3/11 bjr)
    kVertical,   ///< planes that are in the vertical plane (ie ArgoNeuT)
  } Orient_t;

  typedef enum _plane_sigtype {
    kInduction,   ///< signal from induction planes
    kCollection,  ///< signal from collection planes
    kMysteryType  ///< who knows?
  } SigType_t;


  typedef enum driftdir {
    kUnknownDrift, ///< drift direction is unknown
    kPosX,         ///< drift towards positive X values			
    kNegX 	   ///< drift towards negative X values			
  } DriftDirection_t;

  // The data type to uniquely identify a cryostat
  struct CryostatID {
    using ID_t = unsigned int; ///< type for the ID number
    
    CryostatID(): isValid(false), Cryostat(std::numeric_limits<ID_t>::max()) {}
    
    explicit CryostatID(unsigned int c): isValid(true), Cryostat(c) {}
    
    bool isValid;  ///< whether this ID points to a valid TPC
    ID_t Cryostat; ///< index of cryostat the TPC belongs
    
    /// Returns true if the ID is valid
    operator bool() const { return isValid; }
    
    /// Returns true if the ID is not valid
    bool operator! () const { return !isValid; }
    
    bool operator== (const CryostatID& cid) const
      { return Cryostat == cid.Cryostat; }
    
    bool operator!= (const CryostatID& cid) const
      { return Cryostat != cid.Cryostat; }
    
    /// Order cryostats with increasing ID
    bool operator< (const CryostatID& cid) const
      { return Cryostat < cid.Cryostat; }
    
  }; // struct CryostatID

  // The data type to uniquely identify a TPC
  struct TPCID { 
    TPCID(): isValid(false), Cryostat(UINT_MAX), TPC(UINT_MAX) {}

    TPCID(unsigned int c, unsigned int t): isValid(true), Cryostat(c), TPC(t) {}

    bool         isValid;  ///< whether this ID points to a valid TPC
    unsigned int Cryostat; ///< index of cryostat the TPC belongs
    unsigned int TPC;      ///< index of the TPC within its cryostat

    /// Returns true if the ID is valid
    operator bool() const { return isValid; }
    
    /// Returns true if the ID is not valid
    bool operator! () const { return !isValid; }
    
    bool operator== (const TPCID& pid) const
      { return ((Cryostat == pid.Cryostat) && (TPC == pid.TPC)); }

    bool operator!= (const TPCID& pid) const
      { return ((Cryostat != pid.Cryostat) || (TPC != pid.TPC)); }

    // Order TPCID in increasing Cryo, then TPC
    bool operator<( const TPCID& tpcid ) const
      {
        if(      Cryostat != tpcid.Cryostat ) return Cryostat < tpcid.Cryostat;
        else if(      TPC != tpcid.TPC      ) return TPC      < tpcid.TPC;
        else return false;
      } // operator<

  }; // struct TPCID

  // The data type to uniquely identify a Plane
  struct PlaneID { 
    PlaneID()
    : Cryostat(UINT_MAX)
    , TPC(UINT_MAX)
    , Plane(UINT_MAX)
    , isValid(false)
    {}

    PlaneID(unsigned int c, 
	    unsigned int t,
	    unsigned int p)
    : Cryostat(c)
    , TPC(t)
    , Plane(p)
    , isValid(true)
    {}

    PlaneID(const TPCID& tpcid, unsigned int p)
      : Cryostat(tpcid.Cryostat), TPC(tpcid.TPC), Plane(p)
      , isValid(tpcid.isValid)
      {}

    unsigned int Cryostat;
    unsigned int TPC;
    unsigned int Plane;
    bool         isValid;

    bool operator==( const PlaneID& pid ) const {
      return ( Cryostat == pid.Cryostat &&
	       TPC      == pid.TPC      &&
	       Plane    == pid.Plane      );
    }

    bool operator!=( const PlaneID& pid ) const {
      return ( Cryostat != pid.Cryostat ||
	       TPC      != pid.TPC      ||
	       Plane    != pid.Plane      );
    }

    // Order WireIDs in increasing Cryo, 
    // TPC, Plane, Wire number direction
    bool operator<( const PlaneID& pid ) const {
      if(      Cryostat != pid.Cryostat ) return Cryostat < pid.Cryostat;
      else if(      TPC != pid.TPC      ) return TPC      < pid.TPC;
      else if(    Plane != pid.Plane    ) return Plane    < pid.Plane;
      else return false;
    }

  };

  // The data type to uniquely identify a code wire segment.
  struct WireID { 
    WireID()
    : Cryostat(UINT_MAX)
    , TPC(UINT_MAX)
    , Plane(UINT_MAX)
    , Wire(UINT_MAX)
    , isValid(false)
    {}

    WireID(unsigned int c, 
	   unsigned int t,
	   unsigned int p,
	   unsigned int w)
    : Cryostat(c)
    , TPC(t)
    , Plane(p)
    , Wire(w)
    , isValid(true)
    {}

    WireID(const PlaneID& planeid, unsigned int w)
      : Cryostat(planeid.Cryostat), TPC(planeid.TPC), Plane(planeid.Plane)
      , Wire(w)
      , isValid(planeid.isValid)
      {}

    unsigned int Cryostat;
    unsigned int TPC;
    unsigned int Plane;
    unsigned int Wire;
    bool         isValid;

    PlaneID planeID() const { return PlaneID(Cryostat, TPC, Plane); }

    bool operator==( const WireID& wid ) const {
      return ( Cryostat == wid.Cryostat &&
	       TPC      == wid.TPC      &&
	       Plane    == wid.Plane    &&
	       Wire     == wid.Wire        );
    }

    bool operator!=( const WireID& wid ) const {
      return ( Cryostat != wid.Cryostat ||
	       TPC      != wid.TPC      ||
	       Plane    != wid.Plane    ||
	       Wire     != wid.Wire        );
    }

    // Order WireIDs in increasing Cryo, 
    // TPC, Plane, Wire number direction
    bool operator<( const WireID& wid ) const {
      if(      Cryostat != wid.Cryostat ) return Cryostat < wid.Cryostat;
      else if(      TPC != wid.TPC      ) return TPC      < wid.TPC;
      else if(    Plane != wid.Plane    ) return Plane    < wid.Plane;
      else if(     Wire != wid.Wire     ) return Wire     < wid.Wire;
      else return false;
    }

    /// Returns 0 if equal to wid, < 0 if smaller, > 0 if larger
    int cmp(WireID const& wid) const {
      if (Cryostat != wid.Cryostat) return (Cryostat < wid.Cryostat)? -1: +1;
      if (TPC != wid.TPC) return (TPC < wid.TPC)? -1: +1;
      if (Plane != wid.Plane) return (Plane < wid.Plane)? -1: +1;
      if (Wire != wid.Wire) return (Wire < wid.Wire)? -1: +1;
      return 0;
    } // cmp()

  };


  struct WireIDIntersection{
    double y;                  ///< y position of intersection
    double z;                  ///< z position of intersection
    unsigned int TPC;          ///< TPC of intersection

    // In APAs, we want this to increase in the direction wireID 
    // index increases in: moving inward vertically towards y=0
    bool operator<( const WireIDIntersection& otherIntersect ) const {
      return std::abs( y ) > std::abs( otherIntersect.y );
    }
  };

  /// Generic output of CryostatID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, const CryostatID& cid) {
    out << "C:" << cid.Cryostat;
    return out;
  } // operator<< (Stream, CryostatID)


  /// Generic output of TPCID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, const TPCID& tid) {
    out << "C:" << tid.Cryostat << " T:" << tid.TPC;
    return out;
  } // operator<< (Stream, TPCID)


  /// Generic output of PlaneID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, const PlaneID& pid) {
    out << "C:" << pid.Cryostat << " T:" << pid.TPC << " P:" << pid.Plane;
    return out;
  } // operator<< (Stream, PlaneID)


  /// Generic output of WireID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, const WireID& wid) {
    out << "C:" << wid.Cryostat << " T:" << wid.TPC
       << " P:" << wid.Plane << " W:" << wid.Wire;
    return out;
  } // operator<< (Stream, WireID)


}
#endif
