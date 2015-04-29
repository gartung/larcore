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
 */

#ifndef GEO_GEOMETRY_H
#define GEO_GEOMETRY_H

// LArSoft libraries
#include "Geometry/GeometryCore.h"

// framework libraries
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "art/Framework/Services/Registry/ServiceHandle.h" // for the convenience of includers

// ROOT libraries
// #include <TString.h>
// #include <TVector3.h>
// #include <Rtypes.h>

// C/C++ standard libraries
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <memory>
#include <iterator> // std::forward_iterator_tag


class TGeoManager;
class TGeoVolume;
class TGeoNode;
class TGeoMaterial;
class TGeoHMatrix;

namespace geo {

  // Foward declarations within namespace.
  class CryostatGeo;
  class TPCGeo;
  class PlaneGeo;
  class WireGeo;
  class AuxDetGeo;
  
  /**
   * @brief The geometry of one entire detector
   *
   * It relies on geo::ExptGeoHelperInterface service.
   */
  class Geometry: public GeometryCore
  {
  public:
    
    Geometry(fhicl::ParameterSet const& pset, art::ActivityRegistry& reg);
    
    /// Updates the geometry if needed at the beginning of each new run
    void preBeginRun(art::Run const& run);
    
  private:
    
    /// Expands the provided paths and loads the geometry description(s)
    void LoadNewGeometry(std::string gdmlfile, std::string rootfile);
    
    void InitializeChannelMap();

    std::string               fRelPath;          ///< Relative path added to FW_SEARCH_PATH to search for 
                                                 ///< geometry file
    bool                      fDisableWiresInG4; ///< If set true, supply G4 with GDMLfileNoWires
                                                 ///< rather than GDMLfile
    bool                      fForceUseFCLOnly;  ///< Force Geometry to only use the geometry
                                                 ///< files specified in the fcl file
    fhicl::ParameterSet       fSortingParameters;///< Parameter set to define the channel map sorting
  };

} // namespace geo

DECLARE_ART_SERVICE(geo::Geometry, LEGACY)

#endif // GEO_GEOMETRY_H
