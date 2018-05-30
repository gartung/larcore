/**
 * @file    DumpGeometry_module.cc
 * @brief   Prints on screen the current geometry.
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    May 30, 2018
 * 
 */

// framework libraries
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Comment.h"
#include "fhiclcpp/types/Name.h"

// C/C++ standard libraries
#include <string>

// ... more follow

namespace geo {
  class DumpGeometry;
}

/** ****************************************************************************
 * @brief Describes on screen the current geometry.
 * 
 * One print is performed at the beginning of each run.
 * 
 * 
 * Configuration parameters
 * =========================
 * 
 * - *OutputCategory* (string, default: DumpGeometry): output category used
 *   by the message facility to output information (INFO level)
 * 
 */
class geo::DumpGeometry: public art::EDAnalyzer {
  
    public:
  struct Config {
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    
    fhicl::Atom<std::string> outputCategory {
      Name("outputCategory"),
      Comment
        ("name of message facility output category to stream the information into (INFO level)"),
      "DumpGeometry"
      };
  
  }; // struct Config
  
  using Parameters = art::EDAnalyzer::Table<Config>;
  
  explicit DumpGeometry(Parameters const& config);

  // Plugins should not be copied or assigned.
  DumpGeometry(DumpGeometry const&) = delete;
  DumpGeometry(DumpGeometry &&) = delete;
  DumpGeometry& operator = (DumpGeometry const&) = delete;
  DumpGeometry& operator = (DumpGeometry &&) = delete;

  // Required functions
  virtual void analyze(art::Event const&) override {}
  
  /// Drives the dumping.
  virtual void beginRun(art::Run const&) override;
  
    private:
  
  std::string fOutputCategory; ///< Name of the category for output.
  
}; // class geo::DumpGeometry


//==============================================================================
//=== Module implementation
//===

// LArSoft libraries
#include "larcore/Geometry/Geometry.h"

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"


//------------------------------------------------------------------------------
geo::DumpGeometry::DumpGeometry(Parameters const& config)
  : EDAnalyzer(config)
  , fOutputCategory(config().outputCategory())
  {}


//------------------------------------------------------------------------------
void geo::DumpGeometry::beginRun(art::Run const&) {
  
  auto const& geom = *(lar::providerFrom<geo::Geometry>());
  geom.Print(mf::LogInfo(fOutputCategory));
  
} // geo::DumpGeometry::beginRun()

//------------------------------------------------------------------------------
DEFINE_ART_MODULE(geo::DumpGeometry)

//==============================================================================

