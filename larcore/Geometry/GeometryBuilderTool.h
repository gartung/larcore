/**
 * @file   larcore/Geometry/GeometryBuilderTool.h
 * @brief  Interface for a tool to create a geometry builder.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 23, 2019
 * @see    `larcorealg/Geometry/GeometryBuilder.h`
 * 
 * This library is header-only.
 */

#ifndef LARCORE_GEOMETRY_GEOMETRYBUILDERTOOL_H
#define LARCORE_GEOMETRY_GEOMETRYBUILDERTOOL_H

// LArSoft libraries
#include "larcorealg/Geometry/GeometryBuilder.h"

// C/C++ standard libraries
#include <memory> // std::unique_ptr<>


// -----------------------------------------------------------------------------
namespace geo {
  class GeometryBuilderTool;
} // namespace geo
  
/**
 * @brief Interface for a tool creating a geometry builder object.
 * 
 * This class creates a `geo::GeometryBuilder` instance.
 * 
 */
class geo::GeometryBuilderTool {
  
    public:
  
  virtual ~GeometryBuilderTool() noexcept = default;
  
  
  /**
   * @brief Returns a new instance of the geometry builder.
   * 
   * The implementations are expected to throw the proper exceptions on error.
   */
  std::unique_ptr<geo::GeometryBuilder> makeBuilder()
    { return doMakeBuilder(); }
  
  
    protected:
  
  // --- BEGIN -- Virtual interface ------------------------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the channel mapping.
  virtual std::unique_ptr<geo::GeometryBuilder> doMakeBuilder() = 0;
  
  /// @}
  // --- END -- Virtual interface --------------------------------------------
  
  
}; // class geo::GeometryBuilderTool


// -----------------------------------------------------------------------------


#endif // LARCORE_GEOMETRY_GEOMETRYBUILDERTOOL_H

