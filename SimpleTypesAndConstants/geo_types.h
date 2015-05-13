#ifndef GEO_TYPES_H
#define GEO_TYPES_H

#include <climits>
#include <cmath>
#include <string>
#include <sstream>
// #include <limits> // std::numeric_limits<>

namespace geo {
  namespace details {
    /// Write the argument into a string
    template <typename T>
    std::string writeToString(T const& value);
  } // namespace details
  
  
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

  /// The data type to uniquely identify a cryostat
  struct CryostatID {
    typedef unsigned int ID_t; ///< type for the ID number
    
    bool isValid;  ///< whether this ID points to a valid element
    ID_t Cryostat; ///< index of cryostat
    
    // not constexpr because we would need an implementation file to define the
    // constant (and because ROOT 5 does not understand that)
  //  static constexpr ID_t InvalidID = std::numeric_limits<ID_t>::max();
    /// Special code for an invalid ID
    static const ID_t InvalidID = UINT_MAX;
    
    /// Default constructor: an invalid cryostat
    CryostatID(): isValid(false), Cryostat(InvalidID) {}
    
    /// Constructor: valid ID of cryostat with index c
    explicit CryostatID(unsigned int c): isValid(true), Cryostat(c) {}
    
    /// Constructor: valid ID of cryostat with index c
    CryostatID(unsigned int c, bool valid): isValid(valid), Cryostat(c) {}
    
    /// Returns true if the ID is valid
    operator bool() const { return isValid; }
    
    /// Returns true if the ID is not valid
    bool operator! () const { return !isValid; }
    
    // comparison operators are out of class
    
    /// Human-readable representation of the cryostat ID
    explicit operator std::string() const
      { return details::writeToString(*this); }
    
    /// Returns < 0 if this is smaller than other, 0 if equal, > 0 if larger
    int cmp(CryostatID const& other) const
      { return ThreeWayComparison(Cryostat, other.Cryostat); }
    
    /// Returns < 0 if a < b, 0 if a == b, > 0 if a > b
    static int ThreeWayComparison(ID_t a, ID_t b)
      { return (a == b)? 0: ((a < b)? -1: +1); }
    
  }; // struct CryostatID
  
  
  /// The data type to uniquely identify a TPC
  struct TPCID: public CryostatID {
    
    ID_t TPC; ///< index of the TPC within its cryostat
    
    /// Default constructor: an invalid TPC ID
    TPCID(): CryostatID(), TPC(InvalidID) {}

    /// Constructor: TPC with index t in the cryostat identified by cryoid
    TPCID(CryostatID const& cryoid, ID_t t): CryostatID(cryoid), TPC(t) {}

    /// Constructor: TPC with index t in the cryostat index c
    TPCID(ID_t c, ID_t t): CryostatID(c), TPC(t) {}

    // comparison operators are out of class
    
    /// Human-readable representation of the TPC ID
    explicit operator std::string() const
      { return details::writeToString(*this); }
    
    /// Returns < 0 if this is smaller than other, 0 if equal, > 0 if larger
    int cmp(TPCID const& other) const
      {
        register int cmp_res = CryostatID::cmp(other);
        if (cmp_res == 0) // same cryostat: compare TPC
          return ThreeWayComparison(TPC, other.TPC);
        else              // return the order of cryostats
          return cmp_res;
      } // cmp()
    
  }; // struct TPCID
  
  
  /// The data type to uniquely identify a Plane
  struct PlaneID: public TPCID {
    
    ID_t Plane; ///< index of the plane within its TPC
    
    /// Default constructor: an invalid plane ID
    PlaneID(): TPCID(), Plane(InvalidID) {}

    /// Constructor: plane with index p in the TPC identified by tpcid
    PlaneID(TPCID const& tpcid, ID_t p): TPCID(tpcid), Plane(p) {}

    /// Constructor: plane with index p in the cryostat index c, TPC index t
    PlaneID(ID_t c, ID_t t, ID_t p): TPCID(c, t), Plane(p) {}

    // comparison operators are out of class
    
    /// Human-readable representation of the plane ID
    explicit operator std::string() const
      { return details::writeToString(*this); }
    
    /// Returns < 0 if this is smaller than other, 0 if equal, > 0 if larger
    int cmp(PlaneID const& other) const
      {
        register int cmp_res = TPCID::cmp(other);
        if (cmp_res == 0) // same TPC: compare plane
          return ThreeWayComparison(Plane, other.Plane);
        else              // return the order of TPC
          return cmp_res;
      } // cmp()
    
  }; // struct PlaneID
  
  
  // The data type to uniquely identify a code wire segment.
  struct WireID: public PlaneID {
    
    ID_t Wire; ///< index of the wire within its plane
    
    /// Default constructor: an invalid TPC ID
    WireID(): PlaneID(), Wire(InvalidID) {}

    /// Constructor: wire with index w in the plane identified by planeid
    WireID(PlaneID const& planeid, ID_t w): PlaneID(planeid), Wire(w) {}

    /// Constructor: wire with index w in cryostat index c, TPC index t,
    /// plane index p
    WireID(ID_t c, ID_t t, ID_t p, ID_t w): PlaneID(c, t, p), Wire(w) {}

    /// Human-readable representation of the wire ID
    explicit operator std::string() const
      { return details::writeToString(*this); }
    
    /// Returns < 0 if this is smaller than tpcid, 0 if equal, > 0 if larger
    int cmp(WireID const& other) const
      {
        register int cmp_res = PlaneID::cmp(other);
        if (cmp_res == 0) // same plane: compare wire
          return ThreeWayComparison(Wire, other.Wire);
        else              // return the order of planes
          return cmp_res;
      } // cmp()
    
    
    /// Backward compatibility; use the wire directly or a explicit cast instead
    /// @todo Remove the instances of geo::WireID::planeID() in the code
    PlaneID const& planeID() const { return *this; }
    
  }; // struct WireID
  

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

  //----------------------------------------------------------------------------
  //--- ID comparison operators
  //---
  
  /// @{
  /// @name ID comparison operators
  /// @details The result of comparison with invalid IDs is undefined.
  
  /// Comparison: the IDs point to the same cryostat (validity is ignored)
  inline bool operator== (CryostatID const& a, CryostatID const& b)
    { return a.Cryostat == b.Cryostat; }
  
  /// Comparison: the IDs point to different cryostats (validity is ignored)
  inline bool operator!= (CryostatID const& a, CryostatID const& b)
    { return a.Cryostat != b.Cryostat; }
  
  /// Order cryostats with increasing ID
  inline bool operator< (CryostatID const& a, CryostatID const& b)
    { return a.Cryostat < b.Cryostat; }
  
  
  /// Comparison: the IDs point to the same TPC (validity is ignored)
  inline bool operator== (TPCID const& a, TPCID const& b) {
    return
      (static_cast<CryostatID const&>(a) == static_cast<CryostatID const&>(b))
      && (a.TPC == b.TPC);
  } // operator== (TPCID, TPCID)
  
  /// Comparison: the IDs point to different TPCs (validity is ignored)
  inline bool operator!= (TPCID const& a, TPCID const& b) {
    return
      (static_cast<CryostatID const&>(a) != static_cast<CryostatID const&>(b))
      || (a.TPC != b.TPC);
  } // operator!= (TPCID, TPCID)
  
  /// Order TPCID in increasing Cryo, then TPC
  inline bool operator< (TPCID const& a, TPCID const& b) {
    register int cmp_res = (static_cast<CryostatID const&>(a)).cmp(b);
    if (cmp_res == 0) // same cryostat: compare TPC
      return a.TPC < b.TPC;
    else              // return the order of cryostats
      return cmp_res < 0;
  } // operator< (TPCID, TPCID)
  
  
  /// Comparison: the IDs point to the same plane (validity is ignored)
  inline bool operator== (PlaneID const& a, PlaneID const& b) {
    return
      (static_cast<TPCID const&>(a) == static_cast<TPCID const&>(b))
      && (a.Plane == b.Plane);
  } // operator== (PlaneID, PlaneID)
  
  /// Comparison: the IDs point to different planes (validity is ignored)
  inline bool operator!= (PlaneID const& a, PlaneID const& b) {
    return
      (static_cast<TPCID const&>(a) != static_cast<TPCID const&>(b))
      || (a.Plane != b.Plane);
  } // operator!= (PlaneID, PlaneID)
  
  /// Order PlaneID in increasing TPC, then plane
  inline bool operator< (PlaneID const& a, PlaneID const& b) {
    register int cmp_res = (static_cast<TPCID const&>(a)).cmp(b);
    if (cmp_res == 0) // same TPC: compare plane
      return a.Plane < b.Plane;
    else              // return the order of TPC
      return cmp_res < 0;
  } // operator< (PlaneID, PlaneID)
  
  
  /// Comparison: the IDs point to the same wire (validity is ignored)
  inline bool operator== (WireID const& a, WireID const& b) {
    return
      (static_cast<PlaneID const&>(a) == static_cast<PlaneID const&>(b))
      && (a.Wire == b.Wire);
  } // operator== (WireID, WireID)
  
  /// Comparison: the IDs point to different wires (validity is ignored)
  inline bool operator!= (WireID const& a, WireID const& b) {
    return
      (static_cast<PlaneID const&>(a) != static_cast<PlaneID const&>(b))
      || (a.Wire != b.Wire);
  } // operator!= (WireID, WireID)
  
  // Order WireID in increasing plane, then wire
  inline bool operator< (WireID const& a, WireID const& b) {
    register int cmp_res = (static_cast<PlaneID const&>(a)).cmp(b);
    if (cmp_res == 0) // same plane: compare wire
      return a.Wire < b.Wire;
    else              // return the order of planes
      return cmp_res < 0;
  } // operator< (WireID, WireID)
  
  ///@}
  
  //----------------------------------------------------------------------------
  //--- ID output operators
  //---
  /// Generic output of CryostatID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, CryostatID const& cid) {
    out << "C:" << cid.Cryostat;
    return out;
  } // operator<< (Stream, CryostatID)


  /// Generic output of TPCID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, TPCID const& tid) {
    out << ((CryostatID const&) tid) << " T:" << tid.TPC;
    return out;
  } // operator<< (Stream, TPCID)


  /// Generic output of PlaneID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, PlaneID const& pid) {
    out << ((TPCID const&) pid) << " P:" << pid.Plane;
    return out;
  } // operator<< (Stream, PlaneID)


  /// Generic output of WireID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, WireID const& wid) {
    out << ((PlaneID const&) wid) << " W:" << wid.Wire;
    return out;
  } // operator<< (Stream, WireID)
  
  
  namespace details {
    
    template <typename T>
    inline std::string writeToString(T const& value) {
      std::ostringstream sstr;
      sstr << value;
      return sstr.str();
    } // writeToString()
    
  } // namespace details

} // namespace geo
#endif
