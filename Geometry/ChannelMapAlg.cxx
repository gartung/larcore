////////////////////////////////////////////////////////////////////////
/// \file  ChannelMapStandardAlg.cxx
/// \brief Interface to algorithm class for a specific detector channel mapping
///
/// \version $Id:  $
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "Geometry/ChannelMapAlg.h"

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


}
