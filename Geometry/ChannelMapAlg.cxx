////////////////////////////////////////////////////////////////////////
/// \file  ChannelMapStandardAlg.cxx
/// \brief Interface to algorithm class for a specific detector channel mapping
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "Geometry/ChannelMapAlg.h"
#include "Geometry/Geometry.h"

namespace geo{


  //----------------------------------------------------------------------------
  ChannelMapAlg::ChannelMapAlg()
  {
  }

  ChannelMapAlg::~ChannelMapAlg()
  {
  }


  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::NearestWire(const TVector3& worldPos,
					  unsigned int    PlaneNo,
					  unsigned int    TPCNo,
					  unsigned int    cstat) const
  {
    
    return this->NearestWireID(worldPos, PlaneNo, TPCNo, cstat).Wire;

  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::NUniqueOpChannels() const
  {
    // By default return NOpChannels
    // Geometry tmp_geo(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);
    // return tmp_geo->NOpChannels;
    return 12;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpChanUniqueID(int detNum, int channel) const
  {
    return detNum;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpDetFromUniqueChanID(int uniqueChannel) const
  {
    return opChannel;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpDetChannelFromUniqueChanID(int uniqueChannel) const
  {
    return 0;
  }

}
