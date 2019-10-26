/**
 * @file   larcore/Geometry/ChannelMapStandardSetupTool_tool.cc
 * @brief  Tool to create a `geo::ChannelMapStandardAlg` channel mapping object.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 25, 2019
 * @see    `larcorealg/Geometry/ChannelMapStandardAlg.h`
 */

// LArSoft libraries
#include "larcore/Geometry/ChannelMapSetupTool.h"
#include "larcorealg/Geometry/ChannelMapStandardAlg.h"

// framework libraries
#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"


// -----------------------------------------------------------------------------
namespace geo { class ChannelMapStandardSetupTool; }

/**
 * @brief Interface for a tool creating the standard LArSoft channel mapping.
 * 
 * This tool creates a `geo::ChannelMapStandardAlg` channel mapping object.
 * Ownership of the object is shared.
 * 
 */
class geo::ChannelMapStandardSetupTool: public geo::ChannelMapSetupTool {
  
  /// The type of builder being created.
  using Mapper_t = geo::ChannelMapStandardAlg;
  
    public:
  
  /// Constructor: passes all parameters to the channel mapping algorithm.
  ChannelMapStandardSetupTool(fhicl::ParameterSet const& config);
  
  
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
    (fhicl::ParameterSet const& config) const;
  
}; // class geo::ChannelMapStandardSetupTool


// -----------------------------------------------------------------------------
// ---  Implementation
// -----------------------------------------------------------------------------
geo::ChannelMapStandardSetupTool::ChannelMapStandardSetupTool
  (fhicl::ParameterSet const& config)
  : fChannelMap(createMapper(config))
{}


// -----------------------------------------------------------------------------
auto geo::ChannelMapStandardSetupTool::createMapper
  (fhicl::ParameterSet const& config) const -> std::shared_ptr<Mapper_t> 
{
  return std::make_shared<Mapper_t>(config);
} // geo::ChannelMapStandardSetupTool::createMapper()


// -----------------------------------------------------------------------------
DEFINE_ART_CLASS_TOOL(geo::ChannelMapStandardSetupTool)


// -----------------------------------------------------------------------------
