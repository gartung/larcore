////////////////////////////////////////////////////////////////////////////////
/// \file StandardGeometryHelper.h
/// \brief Geometry helper service for detectors that use strictly standard functionality
/// 
/// Handles detector-specific information for the generic Geometry service
/// within LArSoft. Derived from the ExptGeoHelperInterface class. This version
/// provides strictly standard functionality
///
/// \verion $Id
/// \author rs@fnal.gov
////////////////////////////////////////////////////////////////////////////////

#ifndef GEO_StandardGeometryHelper_h
#define GEO_StandardGeometryHelper_h

#include "Geometry/ExptGeoHelperInterface.h"

#include <memory>
#include <vector>

// Forward declarations
//
class TString;

namespace art
{
  class ActivityRegistry;
}

namespace fhicl
{
  class ParameterSet;
}

namespace geo
{
  class ChannelMapAlg;
  class CryostaGeo;
}

namespace geo
{
  class ChannelMapAlg;
}

// Declaration
//
namespace geo
{
  class StandardGeometryHelper : public ExptGeoHelperInterface
  {
  public:
  
    StandardGeometryHelper( fhicl::ParameterSet const & pset, art::ActivityRegistry &reg );
    ~StandardGeometryHelper() throw();

    // Public interface for ExptGeoHelperInterface (for reference purposes)
    //
    // Configure and initialize the channel map.
    //
    // void  ConfigureChannelMapAlg( const TString & detectorName, 
    //                               fhicl::ParameterSet const & sortingParam,
    //                               std::vector<geo::CryostatGeo*> & c );
    //
    // Returns null pointer if the initialization failed
    // NOTE:  the sub-class owns the ChannelMapAlg object
    //
    // std::shared_ptr<const geo::ChannelMapAlg> & GetChannelMapAlg() const;
  
  private:
    
    void  doConfigureChannelMapAlg( const TString & detectorName,
                                    fhicl::ParameterSet const & sortingParam,
                                    std::vector<geo::CryostatGeo*> & c ) override;
    std::shared_ptr<const geo::ChannelMapAlg> doGetChannelMapAlg() const override;
    
    fhicl::ParameterSet const & fPset;
    art::ActivityRegistry & fReg;
    std::shared_ptr<geo::ChannelMapAlg> fChannelMap;
  
  };

}
DECLARE_ART_SERVICE_INTERFACE_IMPL(geo::StandardGeometryHelper, geo::ExptGeoHelperInterface, LEGACY)

#endif // GEO_StandardGeometryHelper_h
