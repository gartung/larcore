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
  unsigned int ChannelMapAlg::NearestWire(const TVector3& worldPos,
                                          geo::PlaneID const& planeID) const
  {
    return NearestWireID(worldPos, planeID.Plane, planeID.TPC, planeID.Cryostat).Wire;
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

  //----------------------------------------------------------------------------
  size_t ChannelMapAlg::NearestAuxDet(const double* point, 
				      std::vector<geo::AuxDetGeo*> const& auxDets) const
  {
    double HalfCenterWidth = 0.;
    double localPoint[3] = {0.};

    for(size_t a = 0; a < auxDets.size(); ++a) {

      auxDets[a]->WorldToLocal(point, localPoint);

      HalfCenterWidth = 0.5 * (auxDets[a]->HalfWidth1() + auxDets[a]->HalfWidth2());

      if( localPoint[2] >= - auxDets[a]->Length()/2       &&
	  localPoint[2] <=   auxDets[a]->Length()/2       &&
	  localPoint[1] >= - auxDets[a]->HalfHeight()     &&
	  localPoint[1] <=   auxDets[a]->HalfHeight()     &&
	  // if AuxDet a is a box, then HalfSmallWidth = HalfWidth
	  localPoint[0] >= - HalfCenterWidth + localPoint[2]*(HalfCenterWidth - auxDets[a]->HalfWidth2())/(0.5 * auxDets[a]->Length()) &&
	  localPoint[0] <=   HalfCenterWidth - localPoint[2]*(HalfCenterWidth - auxDets[a]->HalfWidth2())/(0.5 * auxDets[a]->Length())
	  ) return a;

    }// for loop over AudDet a

    // throw an exception because we couldn't find the sensitive volume
    throw cet::exception("ChannelMapLArIAT") << "Can't find AuxDet for position ("
					     << point[0] << ","
					     << point[1] << ","
					     << point[2] << ")\n";

    return UINT_MAX;

  }

  //----------------------------------------------------------------------------
  size_t ChannelMapAlg::NearestSensitiveAuxDet(const double* point, 
					       std::vector<geo::AuxDetGeo*> const& auxDets) const
  {
    double HalfCenterWidth = 0.;
    double localPoint[3] = {0.};

    size_t auxDetIdx = this->NearestAuxDet(point, auxDets);
    
    geo::AuxDetGeo* adg = auxDets[auxDetIdx];

    for(size_t a = 0; a < adg->NSensitiveVolume(); ++a) {

      geo::AuxDetSensitiveGeo const& adsg = adg->SensitiveVolume(a);
      adsg.WorldToLocal(point, localPoint);    
  
      HalfCenterWidth = 0.5 * (adsg.HalfWidth1() + adsg.HalfWidth2());

      if( localPoint[2] >= - adsg.Length()/2       &&
	  localPoint[2] <=   adsg.Length()/2       &&
	  localPoint[1] >= - adsg.HalfHeight()     &&
	  localPoint[1] <=   adsg.HalfHeight()     &&
	  // if AuxDet a is a box, then HalfSmallWidth = HalfWidth
	  localPoint[0] >= - HalfCenterWidth + localPoint[2]*(HalfCenterWidth - adsg.HalfWidth2())/(0.5 * adsg.Length()) &&
	  localPoint[0] <=   HalfCenterWidth - localPoint[2]*(HalfCenterWidth - adsg.HalfWidth2())/(0.5 * adsg.Length())
	  ) return a;
    }// for loop over AuxDetSensitive a

    // throw an exception because we couldn't find the sensitive volume
    throw cet::exception("Geometry") << "Can't find AuxDetSensitive for position ("
				     << point[0] << ","
				     << point[1] << ","
				     << point[2] << ")\n";

    return UINT_MAX;
  }

}
