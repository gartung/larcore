/**
 * @file    DumpChannelMap_module.cc
 * @brief   Prints on screen the current channel-wire map
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * @date    October 27th, 2015
 * 
 */

// LArSoft libraries
#include "SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t

// framework libraries
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

// C/C++ standard libraries
#include <string>

// ... more follow

namespace geo {
  class DumpChannelMap;
}

/** ****************************************************************************
 * @brief Prints on screen the current channel-wire map
 * 
 * One print is performed at the beginning of each run.
 * 
 * 
 * Configuration parameters
 * =========================
 * 
 * - *ChannelToWires* (boolean, default: true): prints all the wires
 *   corresponding to each channel
 * - *WireToChannel* (boolean, default: false): prints which channel covers
 *   each wire
 * - *FirstChannel* (integer, default: no limit): ID of the lowest channel to be
 *   printed
 * - *LastChannel* (integer, default: no limit): ID of the highest channel to be
 *   printed
 * - *OutputCategory* (string, default: DumpChannelMap): output category used
 *   by the message facility to output information (INFO level)
 * 
 */


class geo::DumpChannelMap: public art::EDAnalyzer {
public:
  explicit DumpChannelMap(fhicl::ParameterSet const & p);

  // Plugins should not be copied or assigned.
  DumpChannelMap(DumpChannelMap const &) = delete;
  DumpChannelMap(DumpChannelMap &&) = delete;
  DumpChannelMap & operator = (DumpChannelMap const &) = delete;
  DumpChannelMap & operator = (DumpChannelMap &&) = delete;

  // Required functions
  virtual void analyze(art::Event const&) override {}
  
  /// Drives the dumping
  virtual void beginRun(art::Run const&) override;
  
    private:
  
  std::string OutputCategory; ///< name of the category for output
  bool DoChannelToWires; ///< dump channel -> wires mapping
  bool DoWireToChannel; ///< dump wire -> channel mapping
  
  raw::ChannelID_t FirstChannel; ///< first channel to be printed
  raw::ChannelID_t LastChannel; ///< last channel to be printed
  
}; // geo::DumpChannelMap


//==============================================================================
//=== Algorithms declaration
//===

// LArSoft libraries
#include "SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t

// C/C++ standard libraries
#include <string>

// ... and more below...

namespace geo {
  class GeometryCore;
} // namespace geo

namespace {
  
  /// Dumps channel-to-wires mapping
  class DumpChannelToWires {
      public:
    
    /// Constructor; includes a working default configuration
    DumpChannelToWires()
      : FirstChannel(raw::InvalidChannelID)
      , LastChannel(raw::InvalidChannelID)
      {}
    
    /// Sets up the required environment
    void Setup(geo::GeometryCore const& geometry)
      { pGeom = &geometry; }
    
    /// Sets the lowest and highest channel ID to be printed (inclusive)
    void SetLimits
      (raw::ChannelID_t first_channel, raw::ChannelID_t last_channel)
      { FirstChannel = first_channel; LastChannel = last_channel; }
    
    /// Dumps to the specified output category
    void Dump(std::string OutputCategory) const;
    
    
      protected:
    geo::GeometryCore const* pGeom = nullptr; ///< pointer to geometry
    
    raw::ChannelID_t FirstChannel; ///< lowest channel to be printed
    raw::ChannelID_t LastChannel; ///< highest channel to be printed
    
    /// Throws an exception if the object is not ready to dump
    void CheckConfig() const;
    
  }; // class DumpChannelToWires
  
  
  /// Dumps wire-to-channel mapping
  class DumpWireToChannel {
      public:
    
    /// Constructor; includes a working default configuration
    DumpWireToChannel() {}
    
    /// Sets up the required environment
    void Setup(geo::GeometryCore const& geometry)
      { pGeom = &geometry; }
    
    /// Dumps to the specified output category
    void Dump(std::string OutputCategory) const;
    
    
      protected:
    geo::GeometryCore const* pGeom = nullptr; ///< pointer to geometry
    
    /// Throws an exception if the object is not ready to dump
    void CheckConfig() const;
    
  }; // class DumpWireToChannel
  
  
} // local namespace


//==============================================================================
//=== Module implementation
//===

// LArSoft libraries
#include "SimpleTypesAndConstants/RawTypes.h" // raw::ChannelID_t
#include "Geometry/Geometry.h"
#include "Geometry/GeometryCore.h"

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

// C/C++ standard libraries
#include <string>

// ... and more below ...

//------------------------------------------------------------------------------
geo::DumpChannelMap::DumpChannelMap(fhicl::ParameterSet const& p)
  : EDAnalyzer(p)
  , OutputCategory(p.get<std::string>   ("OutputCategory", "DumpChannelMap"))
  , DoChannelToWires(p.get<bool>        ("ChannelToWires", true))
  , DoWireToChannel(p.get<bool>         ("WireToChannel",  false))
  , FirstChannel(p.get<raw::ChannelID_t>("FirstChannel",   raw::InvalidChannelID))
  , LastChannel(p.get<raw::ChannelID_t> ("LastChannel",    raw::InvalidChannelID))
{
  
} // geo::DumpChannelMap::DumpChannelMap()

//------------------------------------------------------------------------------
void geo::DumpChannelMap::beginRun(art::Run const&) {
  
  geo::GeometryCore const& geom = *(art::ServiceHandle<geo::Geometry>());
  
  if (DoChannelToWires) {
    DumpChannelToWires dumper;
    dumper.Setup(geom);
    dumper.SetLimits(FirstChannel, LastChannel);
    dumper.Dump(OutputCategory);
  }
  
  if (DoWireToChannel) {
    DumpWireToChannel dumper;
    dumper.Setup(geom);
  //  dumper.SetLimits(FirstChannel, LastChannel);
    dumper.Dump(OutputCategory);
  }
  
} // geo::DumpChannelMap::beginRun()

//------------------------------------------------------------------------------
DEFINE_ART_MODULE(geo::DumpChannelMap)

//==============================================================================
//===  Algorithm implementation
//===

// LArSoft libraries
#include "SimpleTypesAndConstants/geo_types.h" // geo::WireID
#include "Geometry/GeometryCore.h"

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Utilities/Exception.h"

// C/C++ standard libraries
#include <vector>

//------------------------------------------------------------------------------
//--- DumpChannelToWires
//------------------------------------------------------------------------------
void DumpChannelToWires::CheckConfig() const {
  
  /// check that the configuration is complete
  if (!pGeom) {
    throw art::Exception(art::errors::LogicError)
      << "DumpChannelToWires: no valid geometry available!";
  }
} // DumpChannelToWires::CheckConfig()

//------------------------------------------------------------------------------
void DumpChannelToWires::Dump(std::string OutputCategory) const {
  
  /// check that the configuration is complete
  CheckConfig();
  
  /// extract general channel range information
  unsigned int const NChannels = pGeom->Nchannels();
  
  if (NChannels == 0) {
    mf::LogError(OutputCategory)
      << "Nice detector we have here, with no channels.";
    return;
  }
  
  raw::ChannelID_t const PrintFirst
    = raw::isValidChannelID(FirstChannel)? FirstChannel: raw::ChannelID_t(0);
  raw::ChannelID_t const PrintLast
    = raw::isValidChannelID(LastChannel)? LastChannel: raw::ChannelID_t(NChannels-1);
  
  // print intro
  unsigned int const NPrintedChannels = (PrintLast - PrintFirst) + 1;
  if (NPrintedChannels == NChannels) {
    mf::LogInfo(OutputCategory) << "Printing all " << NChannels << " channels";
  }
  else {
    mf::LogInfo(OutputCategory) << "Printing channels from " << PrintFirst
      << " to " << LastChannel << " (" << NPrintedChannels
      << " channels out of " << NChannels << ")";
  }
  
  // print map
  mf::LogVerbatim log(OutputCategory);
  for (raw::ChannelID_t channel = PrintFirst; channel <= PrintLast; ++channel) {
    std::vector<geo::WireID> const Wires = pGeom->ChannelToWire(channel);
    
    log << "\n " << ((int) channel) << " ->";
    switch (Wires.size()) {
      case 0:  log << " no wires";                       break;
      case 1:                                            break;
      default: log << " [" << Wires.size() << " wires]"; break;
    } // switch
    
    for (geo::WireID const& wireID: Wires)
      log << " { " << std::string(wireID) << " };";
    
  } // for (channels)
  
} // DumpChannelToWires::Dump()

//------------------------------------------------------------------------------
//--- DumpWireToChannel
//------------------------------------------------------------------------------
void DumpWireToChannel::CheckConfig() const {
  
  /// check that the configuration is complete
  if (!pGeom) {
    throw art::Exception(art::errors::LogicError)
      << "DumpWireToChannel: no valid geometry available!";
  }
} // DumpWireToChannel::CheckConfig()

//------------------------------------------------------------------------------
void DumpWireToChannel::Dump(std::string OutputCategory) const {
  
  /// check that the configuration is complete
  CheckConfig();
  
  /// extract general channel range information
  unsigned int const NChannels = pGeom->Nchannels();
  
  if (NChannels == 0) {
    mf::LogError(OutputCategory)
      << "Nice detector we have here, with no channels.";
    return;
  }
  
  // print intro
  mf::LogInfo(OutputCategory)
    << "Printing wire channels for up to " << NChannels << " channels";
  
  // print map
  mf::LogVerbatim log(OutputCategory);
  for (geo::WireID const& wireID: pGeom->IterateWireIDs()) {
    raw::ChannelID_t channel = pGeom->PlaneWireToChannel(wireID);
    log << "\n { " << std::string(wireID) << " } => ";
    if (raw::isValidChannelID(channel)) log << channel;
    else                                log << "invalid!";
  } // for
  
} // DumpWireToChannel::Dump()


//==============================================================================
