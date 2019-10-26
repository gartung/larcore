/**
 * @file   Geometry.h
 * @brief  art framework interface to geometry description
 * @author brebel@fnal.gov
 * @see    Geometry_service.cc
 *
 * Revised <seligman@nevis.columbia.edu> 29-Jan-2009
 *         Revise the class to make it into more of a general detector interface
 * Revised <petrillo@fnal.gov> 27-Apr-2015
 *         Factorization into a framework-independent GeometryCore.h and a
 *         art framework interface
 * Revised <petrillo@fnal.gov> 10-Nov-2015
 *         Complying with the provider requirements described in ServiceUtil.h
 */

#ifndef GEO_GEOMETRY_H
#define GEO_GEOMETRY_H

// LArSoft libraries
#include "larcorealg/Geometry/GeometryCore.h"
#include "larcore/CoreUtils/ServiceUtil.h" // not used; for user's convenience

// the following are included for convenience only
#include "larcorealg/Geometry/ChannelMapAlg.h"
#include "larcorealg/Geometry/CryostatGeo.h"
#include "larcorealg/Geometry/TPCGeo.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "larcorealg/Geometry/OpDetGeo.h"
#include "larcorealg/Geometry/AuxDetGeo.h"

// framework libraries
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h" // for the convenience of includers

// C/C++ standard libraries
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <memory>
#include <iterator> // std::forward_iterator_tag


namespace geo {

  /**
   * @brief The geometry of one entire detector, as served by _art_.
   *
   * This class extends the interface of the geometry service provider,
   * `geo::GeometryCore`, to the one of an _art_ service.
   *
   * The geometry initialization happens immediately on construction.
   * Optionally, the geometry is automatically reinitialized on each run based
   * on the information contained in the `art::Run` object.
   * This feature is inherently fragile and should not be relied upon.
   * 
   * 
   * Architecture
   * =============
   * 
   * The geometry service is made of four main components:
   * * `geo::GeometryCore` contains the physical description of the geometry
   *     as interpreted by LArSoft, and it also provides the interface to
   *     user queries;
   * * channel mapping algorithm provides the internal logics relating the
   *     detector sensitive components with the readout channels; the algorithm
   *     is not directly accessible by users, but it is interrogated as needed
   *     bu `geo::GeometryCore`;
   * * geometry builder algorithm connects the detector elements (cryostats,
   *     TPCs, etc.) to form the detector description stored in
   *     `geo::GeometryCore`; this is an algorithm used at initialization time
   *     only, and then discarded;
   * * geometry object sorting algorithm rearranges the internal order of the
   *     detector components (TPCs within a cryostat, wires within a wire plane,
   *     etc.); this is an algorithm used at initialization time only,
   *     and then discarded.
   * 
   * The user interface is via `geo::GeometryCore`, which is the LArSoft
   * geometry service provider. The _art_ service `geo::Geometry` (the class
   * documented here) has mainly the task of managing the four components
   * listed above, invoking them in the proper order yielding a complete
   * and functional geometry description, and to serve the service provider
   * when asked to. For legacy reasons, `geo::Geometry` also exposes the full
   * interface of `geo::GeometryCore`, and could therefore be used directly
   * in place of `geo::GeometryCore`, which is discouraged.
   * 
   * 
   * Setup
   * ======
   * 
   * There are a few options on how to choose the components listed in the
   * architecture description:
   * 
   * * `geo::GeometryCore` is a static object that can be customised only via
   *     FHiCL configuration and the other three algorithm;
   * * the channel mapping algorithm can and must be explicitly selected; the
   *     two ways to do that are:
   *     * via a tool that sets up the channel mapping algorithm
   *         (`ChannelMapping` configuration parameter);
   *         to create a new channel mapping and tool, check their interfaces
   *         (`geo::ChannelMapAlg` and `geo::ChannelMapSetupTool` respectively);
   *     * via the legacy service `ExptGeoHelperInterface`, in which case the
   *         configuration of the channel mapping algorithm is specified in
   *         `SortingParameters` (!);
   * * the object sorting algorithm can be chosen in two ways:
   *     * explicitly via a tool (`Sorter` parameter table); to create a new
   *         object sorting algorithm and tool, check their interfaces
   *         (`geo::GeoObjectSorter` and `geo::GeoObjectSorterSetupTool`)
   *     * implicitly via the channel mapping, which is expected to provide
   *         a sorting algorithm; in that case, its configuration is also
   *         managed by the channel mapping algorithm (for example, the standard
   *         channel mapping algorithm, `geo::ChannelMapStandardAlg`, passes to
   *         the sorter of choice, `geo::GeoObjectSorterStandard`, its entire
   *         configuration as found in `SortingParameters` or `ChannelMapping`
   *         depending on how that channel mapping was chosen)
   * * the geometry builder algorithm can also be set in two ways:
   *     * explicitly via a tool (`Builder` parameter table); to create a new
   *         geometry builder algorithm and tool, check their interface
   *         (`geo::GeometryBuilder` and `geo::GeometryBuilderTool`
   *         respectively);
   *     * by default by not specifying any `Builder` parameter table or leaving
   *         it completely empty, in which case the standard builder is used
   *         (`geo::GeometryBuilderStandard`) with its default configuration.
   * 
   * See the "recommended configuration" below for the recommended setup
   * options.
   * 
   *
   * Configuration
   * ==============
   *
   * In addition to the parameters documented in `geo::GeometryCore`, the
   * following parameters are supported:
   *
   * - *RelativePath* (string, default: no path): this path is prepended to the
   *   geometry file names before searching from them; the path string does not
   *   affect the file name
   * - *GDML* (string, mandatory): path of the GDML file to be served to Geant4
   *   for detector simulation. The full file is composed out of the optional
   *   relative path specified by `RelativePath` path and the base name
   *   specified in `GDML` parameter; this path is searched for in the
   *   directories configured in the `FW_SEARCH_PATH` environment variable;
   * - *ROOT* (string, mandatory): currently overridden by `GDML` parameter,
   *   whose value is used instead;
   *   this path is assembled in the same way as the one for `GDML` parameter,
   *   except that no alternative (wireless) geometry is used even if
   *   `DisableWiresInG4` is specified (see below); this file is used to load
   *   the geometry used in the internal simulation and reconstruction,
   *   basically everywhere except for the Geant4 simulation
   * - *DisableWiresInG4* (boolean, default: false): if true, Geant4 is loaded
   *   with an alternative geometry from a file with the standard name as
   *   configured with the /GDML/ parameter, but with an additional "_nowires"
   *   appended before the ".gdml" suffix
   * - *ForceUseFCLOnly* (boolean, default: false): information on the current
   *   geometry is stored in each run by the event generator producers; if this
   *   information does not describe the current geometry, a new geometry is
   *   loaded according to the information in the run. If `ForceUseFCLOnly`
   *   is set to `true`, this mechanism is disabled and the geometry is just
   *   loaded at the beginning of the job from the information in the job
   *   configuration, once and for all.
   * - *SortingParameters* (a parameter set; default: empty): this configuration
   *   is directly passed to the channel mapping algorithm (see
   *   geo::ChannelMapAlg) unless the channel mapping is chosen via a tool
   *   (see `ChannelMapping` parameter); its content is dependent on the chosen
   *   implementation of `geo::ChannelMapAlg`; note that if `Sorter` is
   *   specified (and `ChannelMapping` is not), this parameter set is still
   *   passed to the channel mapping but the sorter from channel mapping is
   *   still ignored.
   * - *Builder* (a parameter set: default: empty): configuration for the
   *   geometry builder tool, to set up a tool of `geo::GeometryBuilderTool`
   *   interface; if omitted, the standard builder
   *   (`geo::GeometryBuilderStandard`) with default configuration will be
   *   used
   * - *Sorter* (a parameter set: default: empty): configuration for the
   *   geometry object sorter tool (`geo::GeoObjectSorterSetupTool`); it takes
   *   precedence over the one from the channel mapping, which is instead used
   *   if this one is omitted
   * - *ChannelMapping* (a parameter set: default: empty): configuration for the
   *   channel mapping algorithm tool (`ChannelMapSetupTool`); if not specified,
   *   the channel mapping is initialized from `ExptGeoHelperInterface` service.
   *
   * Recommended configuration
   * --------------------------
   * 
   * The `Geometry` service has undergone many changes through LArSoft history,
   * and for legacy many older configuration patterns are supported.
   * The following is the currently recommended way to set it up:
   * * set all the options via tools:
   *     * `ChannelMapping` parameter to choose the channel mapping algorithm;
   *     * `Sorter` to choose a geometry sorting algorithm;
   *     * `Builder` to choose the geometry builder algorithm, if needed
   * * no mention of `SortingParameters` (it would be ignored since channel
   *     mapping is created via its tool anyway);
   * * no `ExptGeoHelperInterface` service.
   * 
   * An example of full `Geometry` service configuration:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * services: {
   *   Geometry: {
   *     
   *     SurfaceY:         690 # in cm, vertical distance to the surface
   *     Name:             "LArTPCdetector"
   *     GDML:             "LArTPCdetector.gdml"
   *     ROOT:             "LArTPCdetector.gdml"
   *     DisableWiresInG4:  true
   *     
   *     ChannelMapping: @local::standard_channel_mapping_tool
   *     Sorter:         @local::standard_geometry_sorter_tool
   *     
   *   } # Geometry
   * } # services
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * which uses the standard geometry builder and picks the (standard) tool
   * configurations from `geometry.fcl`.
   * 
   * @note Currently, the file defined by `GDML` parameter is also served to
   * ROOT for the internal geometry representation.
   *
   */
  class Geometry: public GeometryCore
  {
  public:

    using provider_type = GeometryCore; ///< type of service provider

    Geometry(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);

    /// Updates the geometry if needed at the beginning of each new run
    void preBeginRun(art::Run const& run);

    /// Returns a pointer to the geometry service provider
    provider_type const* provider() const
      { return static_cast<provider_type const*>(this); }

  private:

    /// Expands the provided paths and loads the geometry description(s)
    void LoadNewGeometry(
      std::string gdmlfile, std::string rootfile,
      bool bForceReload = false
      );

    /**
     * @brief Retrieves and sets up the channel mapping.
     * @throw cet::exception (category `"ChannelMapLoadFail"`) if no channel
     *        mapping object is produced
     * 
     * The channel mapping can be obtained in two ways:
     * 
     * 1. from a tool of type `geo::ChannelMapSetupTool`, if a `ChannelMapping`
     *     configuration table is provided
     * 2. from the service `geo::ExptGeoHelperInterface`, otherwise
     * 
     * Failure to obtain the channel mapping with the selected method will
     * throw an exception.
     * 
     * The resulting channel mapping object will be owned by `geo::GeometryCore`
     * (ownership is shared; however, `geo::GeometryCore` will not subsequently
     * share ownership with any other object).
     */
    void InitializeChannelMap();

    /**
     * @brief Retrieve and return a geometry sorter.
     * @param owned a free unique pointer (will own the sorter if needed)
     * @param channelMap pointer to the channel mapping to ask a sorter
     * @return a bare pointer to the sorter, or `nullptr` if failed to get any
     * 
     * The current logics is as follows:
     * 
     * 1. if a sorter configuration is present (`Sorter` in the parameter set)
     *     it loads a tool (`geo::GeoObjectSorterSetupTool`) with that
     *     configuration; ownership is transfered to the `owned` pointer
     * 2. if no sorter configuration is present, obtains a sorter from
     *     channel mapping (owned by the latter; `owned` pointer is not touched)
     * 3. if channel mapping is not available or throws a
     *     `geo::ChannelMapAlg::NoSorter` exception, no sorter is returned
     * 
     * By default, the already configured channel mapping (if any) is used.
     * If non-null, `channelMap` is used instead.
     */
    geo::GeoObjectSorter const* getSorter(
      std::unique_ptr<geo::GeoObjectSorter>& owned,
      geo::ChannelMapAlg const* channelMap = nullptr
      ) const;
    
    
    std::string               fRelPath;          ///< Relative path added to FW_SEARCH_PATH to search for
                                                 ///< geometry file
    bool                      fDisableWiresInG4; ///< If set true, supply G4 with GDMLfileNoWires
                                                 ///< rather than GDMLfile
    bool                      fForceUseFCLOnly;  ///< Force Geometry to only use the geometry
                                                 ///< files specified in the fcl file
    fhicl::ParameterSet       fSortingParameters;///< Parameter set to define the channel map and sorting

    fhicl::ParameterSet       fBuilderParameters;///< Parameter set for geometry builder via tool.
    
    fhicl::ParameterSet       fChannelMapParameters;///< Parameter set for channel mapping via tool.
    
    fhicl::ParameterSet       fSorterParameters; ///< Parameter set for sorting algorithm via tool.

    
    /// Returns a `geo::GeometryBuilder` tool with the specified configuration.
    static std::unique_ptr<geo::GeometryBuilder> makeBuilder
      (fhicl::ParameterSet config);
  
  
  };

} // namespace geo

DECLARE_ART_SERVICE(geo::Geometry, LEGACY)

// check that the requirements for geo::Geometry are satisfied
template struct lar::details::ServiceRequirementsChecker<geo::Geometry>;


#endif // GEO_GEOMETRY_H
