/**
 * @file   ChannelMapStandardTestAlg.h
 * @brief  Tests the standard channel mapping algorithm.
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   June 26th, 2015
 */

#ifndef GEO_CHANNELMAPSTANDARDTESTALG_H
#define GEO_CHANNELMAPSTANDARDTESTALG_H


namespace fhicl {
  class ParameterSet;
}

namespace geo {
  
  class GeometryCore; // forward declaration
  
  //----------------------------------------------------------------------------
  
  class ChannelMapStandardTestAlg {
      public:
    
    /// Constructor: reads configuration, does nothing
    ChannelMapStandardTestAlg(fhicl::ParameterSet const& /* pset */) {}
    
    /// Virtual destructor
    virtual ~ChannelMapStandardTestAlg() = default;
    
    /// Algorithm set up
    virtual void Setup(geo::GeometryCore const& new_geo) { geom = &new_geo; }
    
    /// Executes the test
    virtual unsigned int Run();
   
    /// Tests TPCset mappings
    void TPCsetMappingTest() const;
    
    /// Tests ROP mappings
    void ROPMappingTest() const;
    
    /// Tests channel mappings (very, very partial)
    void ChannelMappingTest() const;
    
      protected:
    GeometryCore const* geom = nullptr; ///< pointer to the geometry description
    
  }; // class ChannelMapStandardTestAlg
  

} // namespace geo


#endif // GEO_CHANNELMAPSTANDARDTESTALG_H
