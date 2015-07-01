/**
 * @file   readout_types.h
 * @brief  Classes identifying readout-related concepts
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   June 19th, 2015
 */

#ifndef READOUT_TYPES_H
#define READOUT_TYPES_H

// LArSoft libraries
#include "SimpleTypesAndConstants/geo_types.h"

namespace readout {
  
  namespace details {
    
    template <typename T>
    inline std::string writeToString(T const& value)
      { return geo::details::writeToString(value); }
    
  } // namespace details
  
  
  // We define our own cryostat ID as an alias of the one from geometry
  typedef geo::CryostatID CryostatID;
  
  
  /**
   * @brief Class identifying a set of TPC sharing readout channels
   * 
   * This identifier provides the cryostat location and a number representing
   * the set of TPCs.
   * This set is defined by not sharing readout channels with any other TPC
   * outside the set.
   */
  struct TPCsetID: public CryostatID {
    typedef unsigned short TPCsetID_t; ///< type for the ID number
    
    // not constexpr because we would need an implementation file to define the
    // constant (and because ROOT 5 does not understand that)
  //  static constexpr TPCsetID_t InvalidID
  //    = std::numeric_limits<TPCsetID_t>::max();
    /// Special code for an invalid ID
    static const TPCsetID_t InvalidID = USHRT_MAX;
    
    TPCsetID_t TPCset; ///< index of the TPC set within its cryostat
    
    /// Default constructor: an invalid TPC set ID
    TPCsetID(): CryostatID(), TPCset(InvalidID) {}

    /// Constructor: TPC set with index g in the cryostat identified by cryoid
    TPCsetID(CryostatID const& cryoid, TPCsetID_t s):
      CryostatID(cryoid), TPCset(s) {}

    /// Constructor: TPC with index t in the cryostat index c
    TPCsetID(CryostatID_t c, TPCsetID_t s): CryostatID(c), TPCset(s) {}

    // comparison operators are out of class

#ifndef __GCCXML__
    /// Human-readable representation of the TPC set ID
    explicit operator std::string() const
      { return readout::details::writeToString(*this); }
#endif // __GCCXML__
    
    /// Returns < 0 if this is smaller than other, 0 if equal, > 0 if larger
    int cmp(TPCsetID const& other) const
      {
        register int cmp_res = CryostatID::cmp(other);
        if (cmp_res == 0) // same cryostat: compare TPC set
          return ThreeWayComparison(TPCset, other.TPCset);
        else              // return the order of cryostats
          return cmp_res;
      } // cmp()
    
    //@{
    /// Conversion to CryostatID (for convenience of notation)
    CryostatID const& asCryostatID() const { return *this; }
    CryostatID& asCryostatID() { return *this; }
    CryostatID const& asConstCryostatID() { return *this; }
    //@}
    
    /// Return the value of the invalid TPC ID as a r-value
    static TPCsetID_t getInvalidID() { return TPCsetID::InvalidID; }
    
  }; // struct TPCsetID
  
  
  
  /**
   * @brief Class identifying a set of planes sharing readout channels
   * 
   * This identifier provides the TPC set location and a number representing
   * the set of planes.
   * This set is defined by not sharing readout channels with any other plane
   * outside the set.
   * 
   * The name stands for "readout plane".
   */
  struct ROPID: public TPCsetID {
    typedef unsigned int ROPID_t; ///< type for the ID number
    
    // not constexpr because we would need an implementation file to define the
    // constant (and because ROOT 5 does not understand that)
  //  static constexpr ROPID_t InvalidID = std::numeric_limits<ROPID_t>::max();
    /// Special code for an invalid ID
    static const ROPID_t InvalidID = USHRT_MAX;
    
    ROPID_t ROP; ///< index of the plane within its TPC
    
    /// Default constructor: an invalid plane ID
    ROPID(): TPCsetID(), ROP(InvalidID) {}

    /// Constructor: readout plane with index r in the TPC set identified by
    /// tpcsetid
    ROPID(TPCsetID const& tpcsetid, ROPID_t r): TPCsetID(tpcsetid), ROP(r) {}

    /// Constructor: plane with index p in the cryostat index c, TPC index t
    ROPID(CryostatID_t c, TPCsetID_t s, ROPID_t r): TPCsetID(c, s), ROP(r) {}

    // comparison operators are out of class
    
#ifndef __GCCXML__
    /// Human-readable representation of the ROP ID
    explicit operator std::string() const
      { return details::writeToString(*this); }
#endif // __GCCXML__
    
    /// Returns < 0 if this is smaller than other, 0 if equal, > 0 if larger
    int cmp(ROPID const& other) const
      {
        register int cmp_res = TPCsetID::cmp(other);
        if (cmp_res == 0) // same TPC set: compare plane
          return ThreeWayComparison(ROP, other.ROP);
        else              // return the order of TPC set
          return cmp_res;
      } // cmp()
    
    //@{
    /// Conversion to TPCsetID (for convenience of notation)
    TPCsetID const& asTPCsetID() const { return *this; }
    TPCsetID& asTPCsetID() { return *this; }
    TPCsetID const& asConstTPCsetID() { return *this; }
    //@}
    
    /// Return the value of the invalid ROP ID as a r-value
    static ROPID_t getInvalidID() { return ROPID::InvalidID; }
    
  }; // struct ROPID
  
  
  
  //----------------------------------------------------------------------------
  //--- ID comparison operators
  //---
  
  /// @{
  /// @name Readout ID comparison operators
  /// @details The result of comparison with invalid IDs is undefined.
  
  /// Comparison: the IDs point to the same TPC set (validity is ignored)
  inline bool operator== (TPCsetID const& a, TPCsetID const& b) {
    return (a.asCryostatID() == b.asCryostatID()) && (a.TPCset == b.TPCset);
  } // operator== (TPCsetID, TPCsetID)
  
  /// Comparison: the IDs point to different TPC sets (validity is ignored)
  inline bool operator!= (TPCsetID const& a, TPCsetID const& b) {
    return (a.asCryostatID() != b.asCryostatID()) || (a.TPCset != b.TPCset);
  } // operator!= (TPCsetID, TPCsetID)
  
  /// Order TPCsetID in increasing Cryo, then TPC set
  inline bool operator< (TPCsetID const& a, TPCsetID const& b) {
    register int cmp_res = a.asCryostatID().cmp(b);
    if (cmp_res == 0) // same cryostat: compare TPC set
      return a.TPCset < b.TPCset;
    else              // return the order of cryostats
      return cmp_res < 0;
  } // operator< (TPCsetID, TPCsetID)
  
  
  /// Comparison: the IDs point to the same ROP (validity is ignored)
  inline bool operator== (ROPID const& a, ROPID const& b) {
    return (a.asTPCsetID() == b.asTPCsetID()) && (a.ROP == b.ROP);
  } // operator== (ROPID, ROPID)
  
  /// Comparison: the IDs point to different ROPs (validity is ignored)
  inline bool operator!= (ROPID const& a, ROPID const& b) {
    return (a.asTPCsetID() != b.asTPCsetID()) || (a.ROP != b.ROP);
  } // operator!= (ROPID, ROPID)
  
  /// Order ROPID in increasing Cryo, then TPC set, then ROP
  inline bool operator< (ROPID const& a, ROPID const& b) {
    register int cmp_res = a.asTPCsetID().cmp(b);
    if (cmp_res == 0) // same TPC set: compare ROP
      return a.ROP < b.ROP;
    else              // return the order of TPC set
      return cmp_res < 0;
  } // operator< (ROPID, ROPID)
  
  
  
  //----------------------------------------------------------------------------
  //--- ID output operators
  //---
  /// Generic output of TPCsetID to stream
  template <typename Stream>
  inline Stream& operator<< (Stream& out, TPCsetID const& sid) {
    out << sid.asCryostatID() << " S:" << sid.TPCset;
    return out;
  } // operator<< (Stream, TPCsetID)
  
  template <typename Stream>
  inline Stream& operator<< (Stream& out, ROPID const& rid) {
    out << rid.asTPCsetID() << " R:" << rid.ROP;
    return out;
  } // operator<< (Stream, ROPID)
  
  
} // namespace readout


#endif // READOUT_TYPES_H
