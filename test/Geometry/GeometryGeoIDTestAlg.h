/**
 * @file   GeometryGeoIDTestAlg.h
 * @brief  Tests the correct assignment of IDs to detector geometry objects
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   October 31, 2016
 */

#ifndef LARCORE_TEST_GEOMETRY_GEOMETRYGEOIDTESTALG_H
#define LARCORE_TEST_GEOMETRY_GEOMETRYGEOIDTESTALG_H


namespace fhicl {
  class ParameterSet;
}

namespace geo {
  
  class GeometryCore; // forward declaration
  
  //----------------------------------------------------------------------------
  
  class GeometryGeoIDTestAlg {
      public:
    
    /// Constructor: reads configuration, does nothing
    GeometryGeoIDTestAlg(fhicl::ParameterSet const& /* pset */) {}
    
    /// Algorithm set up
    void Setup(geo::GeometryCore const& new_geo) { geom = &new_geo; }
    
    /// Executes the test
    unsigned int Run() const;
   
    /// @name All the ID iterator tests
    /// @{
    void CryostatGeoIDTest() const;
    void TPCGeoIDTest() const;
    void PlaneGeoIDTest() const;
    void WireGeoIDTest() const;
    /// @}
    
    
      protected:
    GeometryCore const* geom = nullptr; ///< pointer to the geometry description
    
  }; // class GeometryGeoIDTestAlg
  

} // namespace geo


#endif // LARCORE_TEST_GEOMETRY_GEOMETRYGEOIDTESTALG_H
