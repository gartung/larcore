////////////////////////////////////////////////////////////////////////////////
/// \file StandardGeometryHelper_service.cc
///
/// \version $Id
/// \author  rs@fnal.gov
////////////////////////////////////////////////////////////////////////////////

#include "Geometry/StandardGeometryHelper.h"

#include "Geometry/ChannelMapAlg.h"

// Migration note:
// This class will needs to create only the ChannelMapStandardAlg after migration
#include "Geometry/ChannelMapStandardAlg.h"
// #include "Geometry/ChannelMap35Alg.h"
// #include "Geometry/ChannelMapAPAAlg.h"

#include "TString.h"


namespace geo
{

  StandardGeometryHelper::StandardGeometryHelper( fhicl::ParameterSet const & pset, art::ActivityRegistry & reg )
  :  fPset( pset ),
     fReg( reg ),
     fChannelMap()
  {}

  StandardGeometryHelper::~StandardGeometryHelper() throw()
  {}  
  
  void StandardGeometryHelper::doConfigureChannelMapAlg( const TString & detectorName,
                                                     fhicl::ParameterSet const & sortingParam,
                                                     std::vector<geo::CryostatGeo*> & c )
  {
    fChannelMap = nullptr;
    
    fChannelMap = std::shared_ptr<geo::ChannelMapStandardAlg>( new geo::ChannelMapStandardAlg( sortingParam ) );
    
    // Migration note:
    // Should just create ChannelMapStandardAlg with no decision-making after transition
    if ( detectorName.Contains("argoneut")
        || detectorName.Contains("microboone")
        || detectorName.Contains("bo")
        || detectorName.Contains("jp250l")
        || detectorName.Contains("csu40l")
        || detectorName.Contains("lariat")
        || detectorName.Contains("icarus")
        || detectorName.Contains("lartpcdetector")
       )
    {
      fChannelMap = std::shared_ptr<geo::ChannelMapAlg>( new geo::ChannelMapStandardAlg( sortingParam ) );
    }
//     if ( detectorName.Contains("lbne35t") ) 
//     {
//       fChannelMap = std::shared_ptr<geo::ChannelMapAlg>( new geo::ChannelMap35Alg( sortingParam ) );
//     }
//     else if ( detectorName.Contains("lbne10kt") ) 
//     {
//       fChannelMap = std::shared_ptr<geo::ChannelMapAlg>( new geo::ChannelMapAPAAlg( sortingParam ) );
//     }
//     else if ( detectorName.Contains("lbne34kt") )
//     {
//       fChannelMap = std::shared_ptr<geo::ChannelMapAlg>( new geo::ChannelMapAPAAlg( sortingParam ) );
//     }
    else
    {
      fChannelMap = nullptr;
    }
    if ( fChannelMap )
    {
      fChannelMap->Initialize( c );
    }
  }
  
  std::shared_ptr<const geo::ChannelMapAlg> StandardGeometryHelper::doGetChannelMapAlg() const
  {
    return fChannelMap;
  }

}

DEFINE_ART_SERVICE_INTERFACE_IMPL(geo::StandardGeometryHelper, geo::ExptGeoHelperInterface)
