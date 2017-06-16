/**
 * @file   GeometryIteratorTestAlg.h
 * @brief  Tests the correct iteration of the geo::Geometry iterators
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   May 7th, 2015
 */

#ifndef GEO_GEOMETRYITERATORTESTALG_H
#define GEO_GEOMETRYITERATORTESTALG_H


namespace fhicl {
  class ParameterSet;
}

namespace geo {
  
  class GeometryCore; // forward declaration
  
  //----------------------------------------------------------------------------
  
  class GeometryIteratorTestAlg {
      public:
    
    /// Constructor: reads configuration, does nothing
    GeometryIteratorTestAlg(fhicl::ParameterSet const& /* pset */) {}
    
    /// Virtual destructor
    virtual ~GeometryIteratorTestAlg() = default;
    
    /// Algorithm set up
    virtual void Setup(geo::GeometryCore const& new_geo) { geom = &new_geo; }
    
    /// Executes the test
    virtual unsigned int Run() const;
   
    /// @{
    /// @name ID iterator tests
    void CryostatIDIteratorsTest() const;
    void TPCIDIteratorsTest() const;
    void PlaneIDIteratorsTest() const;
    void WireIDIteratorsTest() const;
    void TPCsetIDIteratorsTest() const;
    void ROPIDIteratorsTest() const;
    /// @}
    
    /// @{
    /// @name Element iterator tests
    void CryostatIteratorsTest() const;
    void TPCIteratorsTest() const;
    void PlaneIteratorsTest() const;
    void WireIteratorsTest() const;
    /// @}
    
      protected:
    GeometryCore const* geom = nullptr; ///< pointer to the geometry description
    
  }; // class GeometryIteratorTestAlg
  

} // namespace geo


#endif // GEO_GEOMETRYITERATORTESTALG_H
