////////////////////////////////////////////////////////////////////////////////
/// \file  ExptGeoHelperInterface.h
/// \brief Interface to a service that handles any experiment-specific knowledge
///        that is needed by the Geometry service.
/// 
///  This is an interface to a service that virtualizes detector or experiment-specific
///  knowledge that is required by the Geometry service. Experiments implement the 
///  private virtual functions within a concrete service provider class to perform
///  the specified actions as appropriate for the particular experiment. It is 
///  expected that such requests will occur infrequently within a job. Calculations
///  that occur frequently should be handled via interfaces that are passed
///  back to the Geometry service.
///
///  Note that the public interface for this service cannot be overriden. The
///  experiment-specific sub-classes should implement only the private methods
///  without promoting their visibility.
///
/// \version $Id
/// \author  rs@fnal.gov
////////////////////////////////////////////////////////////////////////////////


#ifndef GEO_ExptGeoHelperInterface_h
#define GEO_ExptGeoHelperInterface_h

#include <memory>
#include <vector>

#include "art/Framework/Services/Registry/ServiceMacros.h"

class TString;
namespace geo
{
  class ChannelMapAlg;
  class AuxDetGeo;
  class CryostatGeo;
}

namespace geo 
{

  class ExptGeoHelperInterface
  {
  public:
    virtual ~ExptGeoHelperInterface() = default;
    
    /// Configure and initialize the channel map.
    ///
    void  ConfigureChannelMapAlg( const TString & detectorName, 
                                  fhicl::ParameterSet const & sortingParameters,
                                  std::vector<geo::CryostatGeo*> & c,
                                  std::vector<geo::AuxDetGeo*>   & ad );
    
    /// Returns null pointer if the initialization failed
    /// NOTE:  the sub-class owns the ChannelMapAlg object
    ///
    std::shared_ptr<const ChannelMapAlg> GetChannelMapAlg() const;
  
  private:
    
    /// Does the work of initializing the appropriate ChannelMapAlg sub-class
    ///
    virtual 
    void doConfigureChannelMapAlg( const TString & detectorName,
                                   fhicl::ParameterSet const & sortingParameters,
                                   std::vector<geo::CryostatGeo*> & c,
                                   std::vector<geo::AuxDetGeo*>   & ad ) = 0;
    
    /// Returns the ChannelMapAlg
    virtual 
    std::shared_ptr<const ChannelMapAlg> doGetChannelMapAlg() const    = 0;
  
  }; // end ExptGeoHelperInterface class declaration
  


  //-------------------------------------------------------------------------------------------

  inline 
  void ExptGeoHelperInterface::ConfigureChannelMapAlg( const TString & detName,
                                                       fhicl::ParameterSet const & sortingParam,
                                                       std::vector<geo::CryostatGeo*> & c,
                                                       std::vector<geo::AuxDetGeo*>   & ad )
  {
    doConfigureChannelMapAlg( detName, sortingParam, c, ad ); 
  }

  inline 
  std::shared_ptr<const ChannelMapAlg>
        ExptGeoHelperInterface::GetChannelMapAlg() const
  {
    return doGetChannelMapAlg();
  }
}

DECLARE_ART_SERVICE_INTERFACE(geo::ExptGeoHelperInterface, LEGACY)

#endif // GEO_ExptGeoHelperInterface_h

