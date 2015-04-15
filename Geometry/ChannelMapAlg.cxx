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
  unsigned int ChannelMapAlg::NOpChannels(unsigned int NOpDets) const
  {
    // By default just return the number of optical detectos
    return NOpDets;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::NOpHardwareChannels(unsigned int /*opDet*/) const
  {
    // By defualt, 1 channel per optical detector
    return 1;
  }



  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpChannel(unsigned int detNum, unsigned int channel) const
  {
    return detNum;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpDetFromOpChannel(unsigned int opChannel) const
  {
    return opChannel;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::HardwareChannelFromOpChannel(unsigned int opChannel) const
  {
    return 0;
  }

  //----------------------------------------------------------------------------
  bool ChannelMapAlg::IsValidOpChannel(unsigned int opChannel, unsigned int NOpDets) const
  {
    // Check channel number
    if ( opChannel >= this->NOpChannels(NOpDets) ) return false;

    // Check opdet number
    unsigned int opdet = this->OpDetFromOpChannel(opChannel);
    if (opdet >= NOpDets) return false;

    // Check hardware channel number
    unsigned int hChan = this->HardwareChannelFromOpChannel(opChannel);
    if (hChan >= this->NOpHardwareChannels(opdet)) return false;
    
    return true;
  }

}
