////////////////////////////////////////////////////////////////////////////////
/// \file StandardGeometryHelper_service.cc
///
/// \author  rs@fnal.gov
////////////////////////////////////////////////////////////////////////////////

// class header
#include "larcore/Geometry/StandardGeometryHelper.h"

// LArSoft libraries
#include "larcorealg/Geometry/ChannelMapStandardAlg.h"
#include "larcorealg/Geometry/GeometryCore.h"

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"


namespace geo
{

  //----------------------------------------------------------------------------
  StandardGeometryHelper::StandardGeometryHelper(fhicl::ParameterSet const& pset)
    : fPset( pset )
    , fChannelMap()
    {}


  //----------------------------------------------------------------------------
  void StandardGeometryHelper::doConfigureChannelMapAlg
    (fhicl::ParameterSet const& sortingParameters, geo::GeometryCore* geom)
  {
    mf::LogInfo("StandardGeometryHelper")
      << "Loading channel mapping: ChannelMapStandardAlg";
    fChannelMap
      = std::make_shared<geo::ChannelMapStandardAlg>(sortingParameters);
    geom->ApplyChannelMap(fChannelMap);

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
