/**
 * @file   larcore/Geometry/GeoObjectSorterStandardSetupTool_tool.cc
 * @brief  A tool to configure the standard geometry object sorter.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   October 25, 2019
 * @see    `larcore/Geometry/GeoObjectSorterSetupTool.h`
 */

// LArSoft libraries
#include "larcore/Geometry/GeoObjectSorterSetupTool.h"
#include "larcorealg/Geometry/GeoObjectSorterStandard.h"

// support libraries
#include "art/Utilities/ToolMacros.h"
#include "fhiclcpp/ParameterSet.h"


// -----------------------------------------------------------------------------
namespace geo { class GeoObjectSorterStandardSetupTool; }

/**
 * @brief A tool creating a `geo::GeoObjectSorterStandard` object.
 * 
 * This class creates a `geo::GeoObjectSorterStandard` instance.
 * Ownership of the sorter is yielded to the caller.
 * 
 * The configuration is passed directly to the sorter.
 */
class geo::GeoObjectSorterStandardSetupTool
  : public geo::GeoObjectSorterSetupTool
{
    public:
  
  using Sorter_t = geo::GeoObjectSorterStandard;
  
  /// Constructor: passes the configuration to the sorter object.
  GeoObjectSorterStandardSetupTool(fhicl::ParameterSet const& config);
  
  
    protected:
  
  std::unique_ptr<Sorter_t> fSorter; ///< Sorter object to be delivered.
  
  // --- BEGIN -- Virtual interface --------------------------------------------
  /// @name Virtual interface
  /// @{
  
  /// Returns a pointer to the geometry sorter. It works only once.
  virtual std::unique_ptr<geo::GeoObjectSorter> doSorter() override
    { return std::move(fSorter); }
  
  /// @}
  // --- END -- Virtual interface ----------------------------------------------
  
  /// Creates a new sorter with the specified configuration.
  std::unique_ptr<Sorter_t> createSorter
    (fhicl::ParameterSet const& config) const;
  
  
}; // class geo::GeoObjectSorterStandardSetupTool


// -----------------------------------------------------------------------------
geo::GeoObjectSorterStandardSetupTool::GeoObjectSorterStandardSetupTool
  (fhicl::ParameterSet const& config)
  : fSorter(createSorter(config))
  {}

// -----------------------------------------------------------------------------
auto geo::GeoObjectSorterStandardSetupTool::createSorter
  (fhicl::ParameterSet const& config) const -> std::unique_ptr<Sorter_t>
{
  return std::make_unique<Sorter_t>(config);
} // geo::GeoObjectSorterStandardSetupTool::createSorter()


// -----------------------------------------------------------------------------
DEFINE_ART_CLASS_TOOL(geo::GeoObjectSorterStandardSetupTool)


// -----------------------------------------------------------------------------
