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
   * @brief The geometry of one entire detector, as served by art
   *
   * This class extends the interface of the geometry service provider,
   * GeometryCore, to the one of an art service.
   * It handles the correct initialization of the provider using information
   *
   * It relies on geo::ExptGeoHelperInterface service to obtain the
   * channel mapping algorithm proper for the selected geometry.
   *
   * The geometry initialization happens immediately on construction.
   * Optionally, the geometry is automatically reinitialized on each run based
   * on the information contained in the art::Run object.
   *
   * Configuration
   * ==============
   *
   * In addition to the parameters documented in geo::GeometryCore, the
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
   *   geo::ChannelMapAlg); its content is dependent on the chosen
   *   implementation of `geo::ChannelMapAlg`
   * - *Builder* (a parameter set: default: empty): configuration for the
   *   geometry builder; if omitted, the standard builder
   *   (`geo::GeometryBuilderStandard`) with standard configuration will be
   *   used; if specified, currently the standard builder is nevertheless used;
   *   this interface can be "toolized", in which case this parameter set will
   *   select and configure the chosen tool.
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

    /// Returns a pointer to the geometry service provider
    provider_type const* provider() const
      { return static_cast<provider_type const*>(this); }

  private:

    /// Updates the geometry if needed at the beginning of each new run
    void preBeginRun(art::Run const& run);

    /// Expands the provided paths and loads the geometry description(s)
    void LoadNewGeometry(
      std::string gdmlfile, std::string rootfile,
      bool bForceReload = false
      );

    void InitializeChannelMap();

    std::string               fRelPath;          ///< Relative path added to FW_SEARCH_PATH to search for
                                                 ///< geometry file
    bool                      fDisableWiresInG4; ///< If set true, supply G4 with GDMLfileNoWires
                                                 ///< rather than GDMLfile
    bool                      fForceUseFCLOnly;  ///< Force Geometry to only use the geometry
                                                 ///< files specified in the fcl file
    fhicl::ParameterSet       fSortingParameters;///< Parameter set to define the channel map sorting
    fhicl::ParameterSet       fBuilderParameters;///< Parameter set for geometry builder.
  };

} // namespace geo

DECLARE_ART_SERVICE(geo::Geometry, SHARED)

#endif // GEO_GEOMETRY_H
