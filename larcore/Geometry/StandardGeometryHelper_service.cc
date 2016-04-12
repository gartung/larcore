////////////////////////////////////////////////////////////////////////////////
/// \file StandardGeometryHelper_service.cc
///
/// \author  rs@fnal.gov
////////////////////////////////////////////////////////////////////////////////

// class header
#include "larcore/Geometry/StandardGeometryHelper.h"

// LArSoft libraries
#include "larcore/Geometry/ChannelMapStandardAlg.h"
#include "larcore/Geometry/GeometryCore.h"

// C/C++ libraries
#include <string>

namespace geo
{

  //----------------------------------------------------------------------------
  StandardGeometryHelper::StandardGeometryHelper
    (fhicl::ParameterSet const& pset, art::ActivityRegistry&)
    : fPset( pset )
    , fChannelMap()
    {}


  //----------------------------------------------------------------------------
  void StandardGeometryHelper::doConfigureChannelMapAlg
    (fhicl::ParameterSet const& sortingParameters, geo::GeometryCore* geom)
  {
    fChannelMap.reset();
    
    std::string const detectorName = geom->DetectorName();
    
    // Migration note:
    // Should just create ChannelMapStandardAlg with no decision-making after transition
    // detector names in this code must be all lower case
    if ( (detectorName.find("argoneut") != std::string::npos)
        || (detectorName.find("microboone") != std::string::npos)
        || (detectorName.find("bo") != std::string::npos)
        || (detectorName.find("jp250l") != std::string::npos)
        || (detectorName.find("csu40l") != std::string::npos)
        || (detectorName.find("lariat") != std::string::npos)
        || (detectorName.find("icarus") != std::string::npos)
        || (detectorName.find("lartpcdetector") != std::string::npos)
       )
    {
      fChannelMap
        = std::make_shared<geo::ChannelMapStandardAlg>(sortingParameters);
    }
    
    if ( fChannelMap )
    {
      geom->ApplyChannelMap(fChannelMap);
    }
  } // StandardGeometryHelper::doConfigureChannelMapAlg()
  
  
  //----------------------------------------------------------------------------
  StandardGeometryHelper::ChannelMapAlgPtr_t
  StandardGeometryHelper::doGetChannelMapAlg() const
  {
    return fChannelMap;
  }

  //----------------------------------------------------------------------------
  
} // namespace geo

DEFINE_ART_SERVICE_INTERFACE_IMPL(
  geo::StandardGeometryHelper, geo::ExptGeoHelperInterface
  )
