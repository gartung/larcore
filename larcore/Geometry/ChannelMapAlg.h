////////////////////////////////////////////////////////////////////////
/// \file  ChannelMapAlg.h
/// \brief Interface to algorithm class for a specific detector channel mapping
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_CHANNELMAPALG_H
#define GEO_CHANNELMAPALG_H

// LArSoft  libraries
#include "larcoreobj/SimpleTypesAndConstants/readout_types.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcoreobj/SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t

// Framework libraries
#include "cetlib/exception.h"

// ROOT libraries
#include "TVector3.h"

// C/C++ standard libraries
#include <vector>
#include <map>
#include <set>


namespace geo{
  
  // forward-declaration from geometry
  class GeometryData_t;
  class AuxDetGeo;
  
  
  /// Exception thrown on invalid wire number (e.g. NearestWireID())
  class InvalidWireIDError: public cet::exception {
      public:
    InvalidWireIDError(std::string cat): cet::exception(cat) {}
    
    InvalidWireIDError(std::string cat, int bad_wire, int better_wire = -1):
      cet::exception(cat),
      wire_number(bad_wire), better_wire_number(better_wire)
      {}
    
    int wire_number = -1; ///< the invalid wire number
    int better_wire_number = -1; ///< a suggestion for a good wire number
  }; // class InvalidWireIDError
  
 
 /**
  * @brief Interface for a class providing readout channel mapping to geometry
  * 
  * @note A number of methods react specifically when provided with invalid IDs
  * as arguments. An invalid ID is an ID with the isValid flag unset, or, in
  * case of channel IDs, an ID with value `raw::InvalidChannelID`.
  * An ID that does not present this feature is by definition "valid"; this
  * does not imply that the represented entity (channel, geometry entity or
  * readout group) actually exists. *The behaviour of the methods to valid,
  * non-existent IDs should be considered undefined*, and it is recommended
  * that the existence of the entity is checked beforehand.
  * Unless the documentation explicitly defines a behaviour, an undefined
  * behaviour should be assumed; nevertheless, the documentation of some of the
  * methods still reminds of this.
  */
 class ChannelMapAlg{

 public:

   virtual ~ChannelMapAlg() = default;

   virtual void                     Initialize(GeometryData_t& geodata) = 0;
   virtual void                     Uninitialize() = 0;
   
   /// Returns a list of TPC wires connected to the specified readout channel ID
   /// @throws cet::exception (category: "Geometry") if non-existent channel
   virtual std::vector<WireID>      ChannelToWire(raw::ChannelID_t channel)   const = 0;
   
   virtual unsigned int             Nchannels()                               const = 0;
   /// @brief Returns the number of channels in the specified ROP
   /// @return number of channels in the specified ROP, 0 if non-existent
   virtual unsigned int             Nchannels(readout::ROPID const& ropid)    const = 0;
   virtual unsigned int             NOpChannels(unsigned int NOpDets)         const;
   virtual unsigned int             MaxOpChannel(unsigned int NOpDets)        const;
   virtual unsigned int             NOpHardwareChannels(unsigned int opDet)   const;
   
   
   //
   // channel interface
   //
   
   /// @brief Returns whether the specified channel is valid
   /// This default implementation assumes all channels up to Nchannels() valid.
   virtual bool HasChannel(raw::ChannelID_t channel) const
     { return raw::isValidChannelID(channel)? channel < Nchannels(): false; }
   
   //@{
   /**
    * @brief Returns the index of the wire nearest to the specified position
    * @param YPos y coordinate on the wire plane
    * @param ZPos z coordinate on the wire plane
    * @param PlaneNo number of plane
    * @param TPCNo number of TPC
    * @param cstat number of cryostat
    * @param planeID ID of the plane
    * @return an index interpolation between the two nearest wires
    * @see NearestWireID()
    *
    * Respect to NearestWireID(), this method returns a real number,
    * representing a continuous coordinate in the wire axis, with the round
    * values corresponding to the actual wires.
    */
   virtual double WireCoordinate(double YPos,
                                 double ZPos,
                                 unsigned int PlaneNo,
                                 unsigned int TPCNo,
                                 unsigned int cstat) const = 0;
   virtual double WireCoordinate(double YPos,
                                 double ZPos,
                                 geo::PlaneID const& planeID) const
     { return WireCoordinate(YPos, ZPos, planeID.Plane, planeID.TPC, planeID.Cryostat); }
   //@}
   
   //@{
   virtual WireID                          NearestWireID(const TVector3& worldPos,
                                                  unsigned int    PlaneNo,
                                                  unsigned int    TPCNo,
                                                  unsigned int    cstat)   const = 0;
   virtual WireID                          NearestWireID(const TVector3& worldPos,
                                                  geo::PlaneID const& planeID) const
     { return NearestWireID(worldPos, planeID.Plane, planeID.TPC, planeID.Cryostat); }
   //@}
   //@{
   virtual raw::ChannelID_t                PlaneWireToChannel(unsigned int plane,
                                                              unsigned int wire,
                                                              unsigned int tpc,
                                                              unsigned int cstat)    const = 0;
   virtual raw::ChannelID_t                PlaneWireToChannel(geo::WireID const& wireID)    const
     { return PlaneWireToChannel(wireID.Plane, wireID.Wire, wireID.TPC, wireID.Cryostat); }
   //@}
   virtual View_t                           View( raw::ChannelID_t const channel )               const = 0;
   
   /**
    * @brief Return the signal type of the specified channel
    * @param channel ID of the channel
    * @return signal type of the channel, or geo::kMysteryType if not known
    * 
    * On any type of error (e.g., invalid or unknown channel ID),
    * geo::kMysteryType is returned.
    */
   virtual SigType_t SignalType( raw::ChannelID_t const channel )      const = 0;
   
   /**
    * @brief Return the signal type on the specified readout plane
    * @param ropid ID of the readout plane
    * @return signal type on the plane, or geo::kMysteryType if not known
    * 
    * If the readout plane ID is marked invalid, geo::kMysteryType is returned.
    * If the readout plane is not marked invalid, but it does not match an
    * existing readout plane, the result is undefined.
    * 
    * The default implementation uses readout plane to channel mapping.
    * Other implementation may decide to do the opposite.
    */
   virtual SigType_t SignalType(readout::ROPID const& ropid) const
     { return SignalType(FirstChannelInROP(ropid)); }
   
   virtual std::set<View_t>  const& Views()                                   const = 0;
   virtual std::set<PlaneID> const& PlaneIDs()                                const = 0;
   //@{
   unsigned int                     NearestWire(const TVector3& worldPos,
                                           unsigned int    PlaneNo,
                                           unsigned int    TPCNo,
                                           unsigned int    cstat)        const
     { return NearestWire(worldPos, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
   unsigned int                     NearestWire(const TVector3& worldPos,
                                           geo::PlaneID const& planeID)  const;
   //@}
   
   virtual unsigned int OpChannel(unsigned int detNum, unsigned int channel = 0) const;
   virtual unsigned int OpDetFromOpChannel(unsigned int opChannel)               const;
   virtual unsigned int HardwareChannelFromOpChannel(unsigned int opChannel)     const;
   virtual bool         IsValidOpChannel(unsigned int opChannel, unsigned int NOpDets) const;

   // These methods retrieve the private fFirstChannel*
   // vectors for testing.
   const std::vector<std::vector<std::vector<raw::ChannelID_t>>> FirstChannelInNextPlane() const
   { return fFirstChannelInThisPlane; }

   const std::vector<std::vector<std::vector<raw::ChannelID_t>>> FirstChannelInThisPlane() const
   { return fFirstChannelInNextPlane; }

      // methods for the auxiliary detectors

   // method returns the entry in the sorted AuxDetGeo vector so that the 
   // Geometry in turn can return that object
   virtual size_t  NearestAuxDet          (const double* point, 
					   std::vector<geo::AuxDetGeo*> const& auxDets) const;
   virtual size_t  NearestSensitiveAuxDet (const double* point, 
					   std::vector<geo::AuxDetGeo*> const& auxDets) const;
   virtual size_t  ChannelToAuxDet        (std::vector<geo::AuxDetGeo*> const& auxDets,
					   std::string                  const& detName,
					   uint32_t                     const& channel) const;
   virtual std::pair<size_t, size_t>  ChannelToSensitiveAuxDet(std::vector<geo::AuxDetGeo*> const& auxDets,
							       std::string                  const& detName,
							       uint32_t                     const& channel) const;
    
    //
    // TPC set interface
    //
    /// @name TPC set mapping
    /// @{
    /**
     * @brief Returns the total number of TPC sets in the specified cryostat
     * @param cryoid cryostat ID
     * @return number of TPC sets in the cryostat, or 0 if no cryostat found
     */
    virtual unsigned int NTPCsets(readout::CryostatID const& cryoid) const = 0;
    
    /// Returns the largest number of TPC sets any cryostat in the detector has
    virtual unsigned int MaxTPCsets() const = 0;
    
    /// Returns whether we have the specified TPC set
    /// @return whether the TPC set is valid and exists
    virtual bool HasTPCset(readout::TPCsetID const& tpcsetid) const = 0;
    
    /// Returns the ID of the TPC set tpcid belongs to
    virtual readout::TPCsetID TPCtoTPCset(geo::TPCID const& tpcid) const = 0;
    
    /**
     * @brief Returns a list of ID of TPCs belonging to the specified TPC set
     * @param tpcsetid ID of the TPC set to convert into TPC IDs
     * @return the list of TPCs, empty if TPC set is invalid
     *
     * Note that the check is performed on the validity of the TPC set ID, that
     * does not necessarily imply that the TPC set specified by the ID actually
     * exists. Check the existence of the TPC set first (HasTPCset()).
     * Behaviour on valid, non-existent TPC set IDs is undefined.
     */
    virtual std::vector<geo::TPCID> TPCsetToTPCs
      (readout::TPCsetID const& tpcsetid) const = 0;
    
    /// Returns the ID of the first TPC belonging to the specified TPC set
    virtual geo::TPCID FirstTPCinTPCset
      (readout::TPCsetID const& tpcsetid) const = 0;
    
    /// @} TPC set mapping
    
    
    //
    // Readout plane interface
    //
    /// @name Readout plane mapping
    /// @{
    /**
     * @brief Returns the total number of ROP in the specified TPC set
     * @param tpcsetid TPC set ID
     * @return number of readout planes in the TPC set, or 0 if no TPC set found
     * 
     * Note that this methods explicitly check the existence of the TPC set.
     */
    virtual unsigned int NROPs(readout::TPCsetID const& tpcsetid) const = 0;
    
    /// Returns the largest number of ROPs a TPC set in the detector has
    virtual unsigned int MaxROPs() const = 0;
    
    /// Returns whether we have the specified ROP
    /// @return whether the readout plane is valid and exists
    virtual bool HasROP(readout::ROPID const& ropid) const = 0;
    
    /// Returns the ID of the ROP planeid belongs to
    virtual readout::ROPID WirePlaneToROP
      (geo::PlaneID const& planeid) const = 0;
    
    /// Returns a list of ID of planes belonging to the specified ROP
    virtual std::vector<geo::PlaneID> ROPtoWirePlanes
      (readout::ROPID const& ropid) const = 0;
    
    /// Returns the ID of the first plane belonging to the specified ROP
    virtual geo::PlaneID FirstWirePlaneInROP
      (readout::ROPID const& ropid) const = 0;
    
    /**
     * @brief Returns a list of ID of TPCs the specified ROP spans
     * @param ropid ID of the readout plane
     * @return the list of TPC IDs, empty if readout plane ID is invalid
     *
     * Note that this check is performed on the validity of the readout plane
     * ID, that does not necessarily imply that the readout plane specified by
     * the ID actually exists. Check if the ROP exists with HasROP().
     * The behaviour on non-existing readout planes is undefined.
     */
    virtual std::vector<geo::TPCID> ROPtoTPCs
      (readout::ROPID const& ropid) const = 0;
    
    /// Returns the ID of the ROP the channel belongs to
    /// @throws cet::exception (category: "Geometry") if non-existent channel
    virtual readout::ROPID ChannelToROP(raw::ChannelID_t channel) const = 0;
    
    /**
     * @brief Returns the ID of the first channel in the specified readout plane
     * @param ropid ID of the readout plane
     * @return ID of first channel, or raw::InvalidChannelID if ID is invalid
     * 
     * Note that this check is performed on the validity of the readout plane
     * ID, that does not necessarily imply that the readout plane specified by
     * the ID actually exists. Check if the ROP exists with HasROP().
     * The behaviour for non-existing readout planes is undefined.
     */
    virtual raw::ChannelID_t FirstChannelInROP
      (readout::ROPID const& ropid) const = 0;
    
    /// @} Readout plane mapping
    
    
    
 protected:

   /// Data type for per-TPC information
   template <typename T>
   using TPCInfoMap_t = std::vector<std::vector<T>>;
   
   /// Data type for per-plane information
   template <typename T>
   using PlaneInfoMap_t = TPCInfoMap_t<std::vector<T>>;
   
   // These 3D vectors are used in initializing the Channel map.
   // Only a 1D vector is really needed so far, but these are more general.
   PlaneInfoMap_t<raw::ChannelID_t> fFirstChannelInThisPlane;
   PlaneInfoMap_t<raw::ChannelID_t> fFirstChannelInNextPlane;
   
   std::map<std::string, size_t>          fADNameToGeo;             ///< map the names of the dets to the AuxDetGeo objects
   std::map<size_t, std::vector<size_t> > fADChannelToSensitiveGeo; ///< map the AuxDetGeo index to a vector of 
                                                                    ///< indices corresponding to the AuxDetSensitiveGeo index
   
   
   /**
    * @name Internal structure data access
    *
    * These functions allow access to the XxxInfoMap_t types based on geometry
    * element IDs.
    * They are strictly internal.
    */
   /// @{
   
   /// Returns the specified element of the TPC map
   template <typename T>
   T const& AccessElement
     (TPCInfoMap_t<T> const& map, geo::TPCID const& id) const
     { return map[id.Cryostat][id.TPC]; }
   
   /// Returns the number of elements in the specified cryostat of the TPC map
   template <typename T>
   size_t AccessElementSize
     (TPCInfoMap_t<T> const& map, geo::CryostatID const& id) const
     { return map[id.Cryostat].size(); }
   
   //@{
   /// Returns whether the ID specifies a valid entry
   template <typename T>
   bool isValidElement
     (TPCInfoMap_t<T> const& map, geo::CryostatID const& id) const
     { return id.Cryostat < map.size(); }
   template <typename T>
   bool isValidElement(TPCInfoMap_t<T> const& map, geo::TPCID const& id) const
     {
       return isValidElement(map, static_cast<geo::CryostatID const&>(id))
         && (id.TPC < map[id.Cryostat].size());
     }
   //@}
   
   
   /// Returns the specified element of the plane map
   template <typename T>
   T const& AccessElement
     (PlaneInfoMap_t<T> const& map, geo::PlaneID const& id) const
     { return map[id.Cryostat][id.TPC][id.Plane]; }
   
   /// Returns the number of elements in the specified TPC of the plane map
   template <typename T>
   size_t AccessElementSize
     (PlaneInfoMap_t<T> const& map, geo::TPCID const& id) const
     { return map[id.Cryostat][id.TPC].size(); }
   
   //@{
   /// Returns whether the ID specifies a valid entry
   template <typename T>
   bool isValidElement
     (PlaneInfoMap_t<T> const& map, geo::CryostatID const& id) const
     { return id.Cryostat < map.size(); }
   template <typename T>
   bool isValidElement(PlaneInfoMap_t<T> const& map, geo::TPCID const& id) const
     {
       return isValidElement(map, static_cast<geo::CryostatID const&>(id))
         && (id.TPC < map[id.Cryostat].size());
     }
   template <typename T>
   bool isValidElement
     (PlaneInfoMap_t<T> const& map, geo::PlaneID const& id) const
     {
       return isValidElement(map, static_cast<geo::TPCID const&>(id))
         && (id.Plane < AccessSize(map, static_cast<geo::TPCID const&>(id)));
     }
   //@}
   
   /// Returns a pointer to the specified element, or nullptr if invalid
   template <typename T>
   T const* GetElementPtr
     (PlaneInfoMap_t<T> const& map, geo::PlaneID const& id) const
     {
       if (id.Cryostat >= map.size()) return nullptr;
       auto const& cryo_map = map[id.Cryostat];
       if (id.TPC >= cryo_map.size()) return nullptr;
       auto const& TPC_map = cryo_map[id.TPC];
       if (id.Plane >= TPC_map.size()) return nullptr;
       auto const& plane_map = TPC_map[id.Plane];
       return &plane_map;
     } // GetElementPtr()
   //@}
   
   ///@} Internal structure data access
   
 };
}
#endif // GEO_CHANNELMAPALG_H

