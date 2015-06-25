////////////////////////////////////////////////////////////////////////
/// \file  ChannelMapStandardAlg.h
/// \brief Interface to algorithm class for a specific detector channel mapping
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////
#ifndef GEO_CHANNELSTANDARDMAPALG_H
#define GEO_CHANNELSTANDARDMAPALG_H

#include <vector>
#include <set>
#include <iostream>

#include "SimpleTypesAndConstants/geo_types.h"
#include "Geometry/ChannelMapAlg.h"
#include "Geometry/GeoObjectSorterStandard.h"
#include "fhiclcpp/ParameterSet.h"

namespace geo{

  class ChannelMapStandardAlg : public ChannelMapAlg{

  public:

    ChannelMapStandardAlg(fhicl::ParameterSet const& p);
    
    void                     Initialize( GeometryData_t& geodata ) override;
    void                     Uninitialize();
    std::vector<WireID>      ChannelToWire(raw::ChannelID_t channel)     const;
    unsigned int             Nchannels()                                 const;

    //@{
    virtual double WireCoordinate
      (double YPos, double ZPos, geo::PlaneID const& planeID) const override;
    virtual double WireCoordinate(double YPos, double ZPos,
                                 unsigned int PlaneNo,
                                 unsigned int TPCNo,
                                 unsigned int cstat) const
      { return WireCoordinate(YPos, ZPos, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    //@}
    
    //@{
    virtual WireID NearestWireID
      (const TVector3& worldPos, geo::PlaneID const& planeID) const override;
    virtual WireID NearestWireID(const TVector3& worldPos,
                                 unsigned int    PlaneNo,
                                 unsigned int    TPCNo,
                                 unsigned int    cstat) const override
      { return NearestWireID(worldPos, geo::PlaneID(cstat, TPCNo, PlaneNo)); }
    //@}
    
    //@{
    virtual raw::ChannelID_t PlaneWireToChannel
      (geo::WireID const& wireID) const override;
    virtual raw::ChannelID_t PlaneWireToChannel(unsigned int plane,
                                                unsigned int wire,
                                                unsigned int tpc,
                                                unsigned int cstat) const override
      { return PlaneWireToChannel(geo::WireID(cstat, tpc, plane, wire)); }
    //@}
    
    virtual View_t                   View( raw::ChannelID_t const channel )       const override;
    virtual SigType_t                SignalType( raw::ChannelID_t const channel ) const override;
    virtual std::set<View_t>  const& Views()                                      const override;
    virtual std::set<PlaneID> const& PlaneIDs()                                   const override;
    
    
    //
    // TPC set interface
    //
    /// @name TPC set mapping
    /// @{
    /**
     * @brief Returns the total number of TPC sets in the specified cryostat
     * @param cryoid cryostat ID
     * @return number of TPC sets in the cryostat, or 0 if no cryostat found
     * 
     * In this mapping, TPCs have independent readout and there is one TPC in
     * each TPC set and one TPC set for each TPC.
     */
    virtual unsigned int NTPCsets
      (readout::CryostatID const& cryoid) const override;
    
    /// Returns the largest number of TPC sets a cryostat in the detector has
    virtual unsigned int MaxTPCsets() const override;
    
    /// Returns whether we have the specified TPC set
    virtual bool HasTPCset(readout::TPCsetID const& tpcsetid) const override;
    
    /**
     * @brief Returns the ID of the TPC set the specified TPC belongs to
     * @param tpcid ID of the TPC
     * @return the ID of the corresponding TPC set, or invalid ID when tpcid is
     *
     * In this mapping, TPC sets and TPCs are mapped one-to-one.
     * The returned value mirrors the TPC ID in the readout space.
     * If the TPC ID is not valid, an invalid TPC set ID is returned.
     * Note that this check is performed on the validity of the TPC ID, that
     * does not necessarily imply that the TPC specified by the ID actually
     * exists.
     */
    virtual readout::TPCsetID TPCtoTPCset
      (geo::TPCID const& tpcid) const override;
    
    /**
     * @brief Returns a list of ID of TPCs belonging to the specified TPC set
     * @param tpcsetid ID of the TPC set to convert into TPC IDs
     * @return the list of TPCs, empty if TPC set is invalid
     *
     * In this mapping, TPC sets and TPCs are mapped one-to-one.
     * The returned list contains always one entry, unless the specified TPC
     * set ID is invalid, in which case the list is empty.
     * Note that the check is performed on the validity of the TPC set ID, that
     * does not necessarily imply that the TPC set specified by the ID actually
     * exists.
     */
    virtual std::vector<geo::TPCID> TPCsetToTPCs
      (readout::TPCsetID const& tpcsetid) const override;
    
    /// @} TPC set mapping
    
    
    
    //
    // Readout plane interface
    //
    /// @name Readout plane mapping
    /// @{

    /**
     * @brief Returns the total number of ROPs in the specified TPC set
     * @param tpcsetid TPC set ID
     * @return number of readout planes in the TPC sets, or 0 if no set found
     * 
     * In this mapping, planes have independent readout and there is one wire
     * plane in each readout plane and one readout plane for each wire plane.
     */
    virtual unsigned int NROPs
      (readout::TPCsetID const& tpcsetid) const override;
    
    /// Returns the largest number of ROPs a TPC set in the detector has
    virtual unsigned int MaxROPs() const override;
    
    /// Returns whether we have the specified ROP
    virtual bool HasROP(readout::ROPID const& ropid) const override;
    
    /**
     * @brief Returns the ID of the ROP planeid belongs to, or invalid if none
     * @param planeid ID of the plane
     * @return the ID of the corresponding ROP, or invalid ID when planeid is
     *
     * In this mapping, readout planes and wire planes are mapped one-to-one.
     * The returned value mirrors the plane ID in the readout space.
     * If the plane ID is not valid, an invalid readout plane ID is returned.
     * Note that this check is performed on the validity of the plane ID, that
     * does not necessarily imply that the plane specified by the ID actually
     * exists.
     */
    virtual readout::ROPID WirePlaneToROP
      (geo::PlaneID const& planeid) const override;
    
    /**
     * @brief Returns a list of ID of wire planes belonging to the specified ROP
     * @param ropid ID of the readout plane to convert into wire planes
     * @return the list of wire plane IDs, empty if readout plane ID is invalid
     *
     * In this mapping, readout planes and wire planes are mapped one-to-one.
     * The returned list contains always one entry, unless the specified readout
     * plane ID is invalid, in which case the list is empty.
     * Note that this check is performed on the validity of the readout plane
     * ID, that does not necessarily imply that the readout plane specified by
     * the ID actually exists.
     */
    virtual std::vector<geo::PlaneID> ROPtoWirePlanes
      (readout::ROPID const& ropid) const override;
    
    /// @} readout plane mapping
  
  private:
    
    unsigned int                                         fNcryostat;      ///< number of cryostats in the detector
    unsigned int                                         fNchannels;      ///< number of channels in the detector
    raw::ChannelID_t                                     fTopChannel;     ///< book keeping highest channel #
    std::vector<unsigned int>                            fNTPC;           ///< number of TPCs in each cryostat
    std::set<View_t>                                     fViews;          ///< vector of the views present in the detector
    std::set<PlaneID>                                    fPlaneIDs;       ///< vector of the PlaneIDs present in the detector
    PlaneInfoMap_t<float>                                fFirstWireProj;  ///< Distance (0,0,0) to first wire          
                                                                          ///< along orth vector per plane per TPC
    PlaneInfoMap_t<float>                                fOrthVectorsY;   ///< Unit vectors orthogonal to wires in
    PlaneInfoMap_t<float>                                fOrthVectorsZ;   ///< each plane - stored as 2 components
                                                                          ///< to avoid having to invoke any bulky
                                                                          ///< TObjects / CLHEP vectors etc         
    PlaneInfoMap_t<float>                                fWireCounts;     ///< Number of wires in each plane - for
                                                                          ///< range checking after calculation   
    TPCInfoMap_t<unsigned int>                           fNPlanes;        ///< Number of planes in each TPC - for
                                                                          ///< range checking after calculation   
    PlaneInfoMap_t<unsigned int>                         fPlaneBaselines; ///< The number of wires in all the 
                                                                          ///< tpcs and planes up to this one 
                                                                          ///< in the heirachy
    PlaneInfoMap_t<unsigned int>                         fWiresPerPlane;  ///< The number of wires in this plane 
                                                                          ///< in the heirachy

    geo::GeoObjectSorterStandard                         fSorter;         ///< class to sort geo objects
    
    
    /// Retrieved the wire cound for the specified plane ID
    unsigned int WireCount(geo::PlaneID const& id) const
      { return AccessElement(fWireCounts, id); }
    
    /// Returns the largest number of TPCs in a single cryostat
    unsigned int MaxTPCs() const;
    
    /// Converts a TPC ID into a TPC set ID using the same numerical indices
    static readout::TPCsetID ConvertTPCtoTPCset(geo::TPCID const& tpcid);
    
    /// Converts a TPC set ID into a TPC ID using the same numerical indices
    static geo::TPCID ConvertTPCsetToTPC(readout::TPCsetID const& tpcsetid);
    
    /// Converts a ROP ID into a wire plane ID using the same numerical indices
    static readout::ROPID ConvertWirePlaneToROP(geo::PlaneID const& planeid);
    
    /// Converts a wire plane ID into a ROP ID using the same numerical indices
    static geo::PlaneID ConvertROPtoWirePlane(readout::ROPID const& ropid);
    
  };


}
#endif // GEO_CHANNELMAPSTANDARDALG_H

