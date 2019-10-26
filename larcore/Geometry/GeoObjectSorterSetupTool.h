/**
 * @file   larcore/Geometry/GeoObjectSorterSetupTool.h
 * @brief  Interface for a tool to configure a geometry object sorter.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 25, 2019
 * 
 * This library is header-only.
 */

#ifndef LARCORE_GEOMETRY_GEOOBJECTSORTERSETUPTOOL_H
#define LARCORE_GEOMETRY_GEOOBJECTSORTERSETUPTOOL_H

// LArSoft libraries
#include "larcorealg/Geometry/GeoObjectSorter.h"

// C/C++ standard libraries
#include <memory> // std::unique_ptr<>


// -----------------------------------------------------------------------------
namespace geo { class GeoObjectSorterSetupTool; }

/**
 * @brief Interface for a tool creating a geometry sorting object.
 * 
 * This class creates a `geo::GeoObjectSorter` instance.
 * Ownership of the sorter is yielded to the caller.
 * 
 * The implementation is expected to extract from the tool configuration all the
 * information it needs to create the sorter.
 */
class geo::GeoObjectSorterSetupTool {
    
    public:
  
  virtual ~GeoObjectSorterSetupTool() noexcept = default;
  
  
  /**
   * @brief Returns a new instance of the geometry object sorter.
   * 
   * If the call fails, a null pointer is returned. This may happen on calls
   * following the first one, if the implementation does not support multiple
   * calls. 
   * For all other errors, the implementations are expected to throw
   * the proper exception.
   */
  std::unique_ptr<geo::GeoObjectSorter> setupSorter()
    { return doSorter(); }
  
  
    protected:
  
  // --- BEGIN -- Virtual interface --------------------------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the geometry sorter.
  virtual std::unique_ptr<geo::GeoObjectSorter> doSorter() = 0;
  
  /// @}
  // --- END -- Virtual interface ----------------------------------------------
  
  
}; // class geo::GeoObjectSorterSetupTool


// -----------------------------------------------------------------------------

#endif // LARCORE_GEOMETRY_GEOOBJECTSORTERSETUPTOOL_H
