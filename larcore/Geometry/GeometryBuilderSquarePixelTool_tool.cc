/**
 * @file   larcore/Geometry/GeometryBuilderSquarePixelTool_tool.cc
 * @brief  Tool to create a `geo::GeometryBuilderSquarePixel` geometry builder.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 23, 2019
 * @see    `larcorealg/Geometry/GeometryBuilderSquarePixel.h`
 */

// LArSoft libraries
#include "larcore/Geometry/GeometryBuilderTool.h"
#include "larcorealg/Geometry/PixelPlane/GeometryBuilderSquarePixel.h"

// framework libraries
#include "art/Utilities/ToolConfigTable.h"
#include "art/Utilities/ToolMacros.h"


// -----------------------------------------------------------------------------
namespace geo {
  class GeometryBuilderSquarePixelTool;
} // namespace geo


/**
 * @brief Interface for a tool creating the standard ICARUS channel mapper.
 * 
 * This tool creates a `icarus::ICARUSChannelMapAlg` mapper.
 * 
 */
class geo::GeometryBuilderSquarePixelTool: public geo::GeometryBuilderTool {
  
  /// The type of builder being created.
  using Builder_t = geo::GeometryBuilderSquarePixel;
  
    public:
  
  using Parameters = art::ToolConfigTable<Builder_t::Config>;
  
  
  /// Constructor: passes all parameters to the channel mapping algorithm.
  GeometryBuilderSquarePixelTool(Parameters const& config);
  
  
    protected:
  
  std::unique_ptr<Builder_t> fBuilder;
  
  
  // --- BEGIN -- Virtual interface implementation ---------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the channel mapping.
  virtual std::unique_ptr<geo::GeometryBuilder> doMakeBuilder() override
    { return std::move(fBuilder); }
  
  /// @}
  // --- END -- Virtual interface implementation -----------------------------
  
  
  /// Creates and returns the channel mapping algorithm.
  std::unique_ptr<Builder_t> createBuilder
    (Builder_t::Config const& config) const;
  
}; // class geo::GeometryBuilderSquarePixelTool


// -----------------------------------------------------------------------------
// ---  Implementation
// -----------------------------------------------------------------------------
geo::GeometryBuilderSquarePixelTool::GeometryBuilderSquarePixelTool
  (Parameters const& config)
  : fBuilder(createBuilder(config()))
{}


// -----------------------------------------------------------------------------
auto geo::GeometryBuilderSquarePixelTool::createBuilder
  (Builder_t::Config const& config) const -> std::unique_ptr<Builder_t>
{
  return std::make_unique<Builder_t>(config);
} // geo::GeometryBuilderSquarePixelTool::createBuilder()



// -----------------------------------------------------------------------------
DEFINE_ART_CLASS_TOOL(geo::GeometryBuilderSquarePixelTool)


// -----------------------------------------------------------------------------
