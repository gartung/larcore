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

#include "Geometry/ChannelMapAlg.h"
#include "Geometry/GeoObjectSorterStandard.h"
#include "fhiclcpp/ParameterSet.h"

namespace geo{

  class ChannelMapStandardAlg : public ChannelMapAlg{

  public:

    ChannelMapStandardAlg(fhicl::ParameterSet const& p);
    ~ChannelMapStandardAlg();
    
    void                     Initialize( GeometryData_t& geodata ) override;
    void                     Uninitialize();
    std::vector<WireID>      ChannelToWire(raw::ChannelID_t channel)     const;
    unsigned int             Nchannels()                                 const;

    double                   WireCoordinate(double YPos, double ZPos,
					    unsigned int    PlaneNo,
					    unsigned int    TPCNo,
					    unsigned int    cstat)       const;

    WireID                   NearestWireID(const TVector3& worldPos,
                                           unsigned int    PlaneNo,
                                           unsigned int    TPCNo,
                                           unsigned int    cstat)      const;
    raw::ChannelID_t         PlaneWireToChannel(unsigned int plane,
                                                unsigned int wire,
                                                unsigned int tpc,
                                                unsigned int cstat)    const;
   View_t                    View( raw::ChannelID_t const channel )            const;
   SigType_t                 SignalType( raw::ChannelID_t const channel )      const;
   std::set<View_t>  const&  Views()                                   const;
   std::set<PlaneID> const&  PlaneIDs()                                const;

  private:
    
    unsigned int                                         fNcryostat;      ///< number of cryostats in the detector
    unsigned int                                         fNchannels;      ///< number of channels in the detector
    raw::ChannelID_t                                     fTopChannel;     ///< book keeping highest channel #
    std::vector<unsigned int>                            fNTPC;           ///< number of TPCs in each cryostat
    std::set<View_t>                                     fViews;          ///< vector of the views present in the detector
    std::set<PlaneID>                                    fPlaneIDs;       ///< vector of the PlaneIDs present in the detector
    std::vector<std::vector<std::vector<float>>>         fFirstWireProj;  ///< Distance (0,0,0) to first wire          
                                                                          ///< along orth vector per plane per TPC
    std::vector<std::vector<std::vector<float>>>         fOrthVectorsY;   ///< Unit vectors orthogonal to wires in
    std::vector<std::vector<std::vector<float>>>         fOrthVectorsZ;   ///< each plane - stored as 2 components
                                                                          ///< to avoid having to invoke any bulky
                                                                          ///< TObjects / CLHEP vectors etc         
    std::vector<std::vector<std::vector<float>>>         fWireCounts;     ///< Number of wires in each plane - for
                                                                          ///< range checking after calculation   
    std::vector<std::vector<unsigned int>>                    fNPlanes;        ///< Number of planes in each TPC - for
                                                                          ///< range checking after calculation   
    std::vector<std::vector<std::vector<unsigned int>>>  fPlaneBaselines; ///< The number of wires in all the 
                                                                          ///< tpcs and planes up to this one 
                                                                          ///< in the heirachy
    std::vector<std::vector<std::vector<unsigned int>>>  fWiresPerPlane;  ///< The number of wires in this plane 
                                                                          ///< in the heirachy

    geo::GeoObjectSorterStandard                         fSorter;         ///< class to sort geo objects
  };


}
#endif // GEO_CHANNELMAPSTANDARDALG_H

