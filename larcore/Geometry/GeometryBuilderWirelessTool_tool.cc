/**
 * @file   larcore/Geometry/GeometryBuilderWirelessTool_tool.cc
 * @brief  Tool to create a `geo::GeometryBuilderWireless` geometry builder.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   November 5, 2019
 * @see    `larcorealg/Geometry/GeometryBuilderWireless.h`
 */

// LArSoft libraries
#include "larcore/Geometry/GeometryBuilderTool.h"
#include "larcorealg/Geometry/GeometryBuilderWireless.h"

// framework libraries
#include "art/Utilities/ToolConfigTable.h"
#include "art/Utilities/ToolMacros.h"


// -----------------------------------------------------------------------------
namespace geo {
  class GeometryBuilderWirelessTool;
} // namespace geo


/**
 * @brief Tool creating a geometry builder ignoring wires.
 * 
 * This tool creates a `geo::GeometryBuilderWireless` geometry builder.
 * 
 */
class geo::GeometryBuilderWirelessTool: public geo::GeometryBuilderTool {
  
  /// The type of builder being created.
  using Builder_t = geo::GeometryBuilderWireless;
  
    public:
  
  using Parameters = art::ToolConfigTable<Builder_t::Config>;
  
  
  /// Constructor: passes all parameters to the geometry builder algorithm.
  GeometryBuilderWirelessTool(Parameters const& config);
  
  
    protected:
  
  std::unique_ptr<Builder_t> fBuilder;
  
  
  // --- BEGIN -- Virtual interface implementation ---------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the geometry builder.
  virtual std::unique_ptr<geo::GeometryBuilder> doMakeBuilder() override
    { return std::move(fBuilder); }
  
  /// @}
  // --- END -- Virtual interface implementation -----------------------------
  
  
  /// Creates and returns the geometry builder.
  std::unique_ptr<Builder_t> createBuilder
    (Builder_t::Config const& config) const;
  
}; // class geo::GeometryBuilderWirelessTool


// -----------------------------------------------------------------------------
// ---  Implementation
// -----------------------------------------------------------------------------
geo::GeometryBuilderWirelessTool::GeometryBuilderWirelessTool
  (Parameters const& config)
  : fBuilder(createBuilder(config()))
{}


// -----------------------------------------------------------------------------
auto geo::GeometryBuilderWirelessTool::createBuilder
  (Builder_t::Config const& config) const -> std::unique_ptr<Builder_t>
{
  return std::make_unique<Builder_t>(config);
} // geo::GeometryBuilderWirelessTool::createBuilder()



// -----------------------------------------------------------------------------
DEFINE_ART_CLASS_TOOL(geo::GeometryBuilderWirelessTool)


// -----------------------------------------------------------------------------
