/**
 * @file   GeometryIteratorLoopTestAlg.h
 * @brief  Tests the correct iteration of the geo::Geometry iterators
 * @author Gianluca Petrillo (petrillo@fnal.gov)
 * @date   May 7th, 2015
 */

#ifndef GEO_GEOMETRYITERATORLOOPTESTALG_H
#define GEO_GEOMETRYITERATORLOOPTESTALG_H


namespace fhicl {
  class ParameterSet;
}

namespace geo {
  
  class GeometryCore; // forward declaration
  
  //----------------------------------------------------------------------------
  
  class GeometryIteratorLoopTestAlg {
      public:
    
    /// Constructor: reads configuration, does nothing
    GeometryIteratorLoopTestAlg(fhicl::ParameterSet const& /* pset */) {}
    
    /// Virtual destructor
    virtual ~GeometryIteratorLoopTestAlg() = default;
    
    /// Algorithm set up
    virtual void Setup(geo::GeometryCore const& new_geo) { geom = &new_geo; }
    
    /// Executes the test
    virtual unsigned int Run();
    
      protected:
    GeometryCore const* geom = nullptr; ///< pointer to the geometry description
    
  }; // class GeometryIteratorLoopTestAlg
  

} // namespace geo


#endif // GEO_GEOMETRYITERATORLOOPTESTALG_H
