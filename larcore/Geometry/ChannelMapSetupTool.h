/**
 * @file   larcore/Geometry/ChannelMapSetupTool.h
 * @brief  Interface for a tool to configure a geometry channel mapper.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 7, 2019
 * 
 * This library is header-only.
 */

#ifndef LARCORE_GEOMETRY_CHANNELMAPSETUPTOOL_H
#define LARCORE_GEOMETRY_CHANNELMAPSETUPTOOL_H

// LArSoft libraries
#include "larcorealg/Geometry/ChannelMapAlg.h"

// C/C++ standard libraries
#include <memory> // std::shared_ptr<>


// -----------------------------------------------------------------------------
namespace geo { class ChannelMapSetupTool; }
  
/**
 * @brief Interface for a tool creating a channel mapping object.
 * 
 * This class creates a `geo::ChannelMapAlg` instance.
 * The usage pattern is to call `setupChannelMap()`, after which this tool
 * can be discarded since the returned pointer shares the ownership of the
 * mapping object.
 * 
 * The implementation of this tool is expected to figure out the configuration
 * to pass to the channel mapping algorithm based on the tool configuration
 * only. The tool implementation can choose to create the channel mapping
 * object immediately on construction, or on the fly when it's requested via
 * `setupChannelMap()`.
 * The delivered channel mapping object is fully constructed and ready to
 * `geo::ChannelMapAlg::Initialize()` the geometry.
 * 
 */
class geo::ChannelMapSetupTool {
    public:
  
  virtual ~ChannelMapSetupTool() noexcept = default;
  
  
  /**
   * @brief Returns a new instance of the channel mapping.
   * 
   * If the call fails, a null pointer is returned. This may happen on calls
   * following the first one, if the implementation does not support multiple
   * calls. 
   * For all other errors, the implementations are expected to throw
   * the proper exception.
   */
  std::shared_ptr<geo::ChannelMapAlg> setupChannelMap()
    { return doChannelMap(); }
  
  
    protected:
  
  // --- BEGIN -- Virtual interface --------------------------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the channel mapping.
  virtual std::shared_ptr<geo::ChannelMapAlg> doChannelMap() = 0;
  
  /// @}
  // --- END -- Virtual interface ----------------------------------------------
  
  
}; // class geo::ChannelMapSetupTool


// -----------------------------------------------------------------------------

#endif // LARCORE_GEOMETRY_CHANNELMAPSETUPTOOL_H
