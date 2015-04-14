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
  unsigned int ChannelMapAlg::NOpChannels(int NOpDets) const
  {
    // By default just return the number of optical detectos
    return NOpDets;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::NOpHardwareChannels(int /*opDet*/) const
  {
    // By defualt, 1 channel per optical detector
    return 1;
  }



  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpChannel(int detNum, int channel) const
  {
    return detNum;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::OpDetFromOpChannel(int opChannel) const
  {
    return opChannel;
  }

  //----------------------------------------------------------------------------
  unsigned int ChannelMapAlg::HardwareChannelFromOpChannel(int opChannel) const
  {
    return 0;
  }

}
