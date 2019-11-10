/**
 * @file   larcore/Geometry/ChannelMapPixelSetupTool_tool.cc
 * @brief  Tool to create a `geo::ChannelMapPixelAlg` channel mapping object.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 25, 2019
 * @see    `larcorealg/Geometry/ChannelMapPixelAlg.h`
 */

// LArSoft libraries
#include "larcore/Geometry/ChannelMapSetupTool.h"
#include "larcorealg/Geometry/PixelPlane/ChannelMapPixelAlg.h"

// framework libraries
#include "art/Utilities/ToolConfigTable.h"
#include "art/Utilities/ToolMacros.h"


// -----------------------------------------------------------------------------
namespace geo { class ChannelMapPixelSetupTool; }

/**
 * @brief Interface for a tool creating the LArSoft "pixel" channel mapping.
 * 
 * This tool creates a `geo::ChannelMapPixelAlg` channel mapping object.
 * Ownership of the object is shared.
 * 
 */
class geo::ChannelMapPixelSetupTool: public geo::ChannelMapSetupTool {
  
  /// The type of builder being created.
  using Mapper_t = geo::ChannelMapPixelAlg;
  
    public:
  
  using Parameters = art::ToolConfigTable<Mapper_t::Config>;
  
  /// Constructor: passes all parameters to the channel mapping algorithm.
  ChannelMapPixelSetupTool(Parameters const& config);
  
  
    protected:
  
  std::shared_ptr<Mapper_t> fChannelMap; ///< Pointer to the channel mapping.
  
  
  // --- BEGIN -- Virtual interface ------------------------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the channel mapping. Ownership is shared.
  virtual std::shared_ptr<geo::ChannelMapAlg> doChannelMap() override
    { return fChannelMap; }
  
  /// @}
  // --- END -- Virtual interface --------------------------------------------

  /// Creates and returns the channel mapping algorithm.
  std::shared_ptr<Mapper_t> createMapper
    (Mapper_t::Config const& config) const;
  
}; // class geo::ChannelMapPixelSetupTool


// -----------------------------------------------------------------------------
// ---  Implementation
// -----------------------------------------------------------------------------
geo::ChannelMapPixelSetupTool::ChannelMapPixelSetupTool
  (Parameters const& config)
  : fChannelMap(createMapper(config()))
{}


// -----------------------------------------------------------------------------
auto geo::ChannelMapPixelSetupTool::createMapper
  (Mapper_t::Config const& config) const -> std::shared_ptr<Mapper_t> 
{
  return std::make_shared<Mapper_t>(config);
} // geo::ChannelMapPixelSetupTool::createMapper()


// -----------------------------------------------------------------------------
DEFINE_ART_CLASS_TOOL(geo::ChannelMapPixelSetupTool)


// -----------------------------------------------------------------------------
