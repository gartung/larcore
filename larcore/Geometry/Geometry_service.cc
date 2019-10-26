/**
 * @file   Geometry_service.cc
 * @brief  art framework interface to geometry description - implementation file
 * @author brebel@fnal.gov
 * @see    Geometry.h
 */

// class header
#include "larcore/Geometry/Geometry.h"

// lar includes
#include "larcore/Geometry/GeometryBuilderTool.h"
#include "larcore/Geometry/ChannelMapSetupTool.h"
#include "larcore/Geometry/GeoObjectSorterSetupTool.h"
#include "larcore/Geometry/ExptGeoHelperInterface.h"
#include "larcoreobj/SummaryData/RunData.h"

// Framework includes
#include "art/Utilities/make_tool.h"
#include "fhiclcpp/types/Table.h"
#include "cetlib_except/exception.h"
#include "cetlib/search_path.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// C/C++ standard libraries
#include <string>


namespace geo {


  //......................................................................
  // Constructor.
  Geometry::Geometry(fhicl::ParameterSet const& pset, art::ActivityRegistry &reg)
    : GeometryCore(pset)
    , fRelPath             (pset.get< std::string       >("RelativePath",     ""   ))
    , fDisableWiresInG4    (pset.get< bool              >("DisableWiresInG4", false))
    , fForceUseFCLOnly     (pset.get< bool              >("ForceUseFCLOnly" , false))
    , fSortingParameters   (pset.get<fhicl::ParameterSet>("SortingParameters", {}  ))
    , fBuilderParameters   (pset.get<fhicl::ParameterSet>("Builder",           {}  ))
    , fChannelMapParameters(pset.get<fhicl::ParameterSet>("ChannelMapping",    {}  ))
    , fSorterParameters    (pset.get<fhicl::ParameterSet>("Sorter",            {}  ))
  {
    // add a final directory separator ("/") to fRelPath if not already there
    if (!fRelPath.empty() && (fRelPath.back() != '/')) fRelPath += '/';

    // register a callback to be executed when a new run starts
    reg.sPreBeginRun.watch(this, &Geometry::preBeginRun);

    //......................................................................
    // 5.15.12 BJR: use the gdml file for both the fGDMLFile and fROOTFile
    // variables as ROOT v5.30.06 is once again able to read in gdml files
    // during batch operation, in this case think of fROOTFile meaning the
    // file used to make the ROOT TGeoManager.  I don't want to remove
    // the separate variables in case ROOT breaks again
    std::string GDMLFileName = pset.get<std::string>("GDML");
    std::string ROOTFileName = pset.get<std::string>("GDML");

    // load the geometry
    LoadNewGeometry(GDMLFileName, ROOTFileName);

  } // Geometry::Geometry()


  void Geometry::preBeginRun(art::Run const& run)
  {
    // FIXME this seems utterly wrong: constructor loads geometry based on an
    // explicit parameter, whereas here we load it by detector name

    // if we are requested to stick to the configured geometry, do nothing
    if (fForceUseFCLOnly) return;

    // check here to see if we need to load a new geometry.
    // get the detector id from the run object
    std::vector< art::Handle<sumdata::RunData> > rdcol;
    run.getManyByType(rdcol);
    if (rdcol.empty()) {
      mf::LogWarning("Geometry") << "cannot find sumdata::RunData object to grab detector name\n"
                                 << "this is expected if generating MC files\n"
                                 << "using default geometry from configuration file\n";
      return;
    }

    // if the detector name is still the same, everything is fine
    std::string newDetectorName = rdcol.front()->DetName();
    if (DetectorName() == newDetectorName) return;

    // check to see if the detector name in the RunData
    // object has not been set.
    std::string const nodetname("nodetectorname");
    if (newDetectorName == nodetname) {
      MF_LOG_WARNING("Geometry") << "Detector name not set: " << newDetectorName;
    } // if no detector name stored
    else {
      // the detector name is specified in the RunData object
      SetDetectorName(newDetectorName);
    }

    LoadNewGeometry(DetectorName() + ".gdml", DetectorName() + ".gdml", true);
  } // Geometry::preBeginRun()


  //......................................................................
  void Geometry::InitializeChannelMap()
  {
    
    // channel mapping can be loaded from a tool,
    // in which case the configuration must at least specify the tool_type:
    if (!fChannelMapParameters.is_empty()) {
      std::shared_ptr<geo::ChannelMapAlg> channelMap
        = art::make_tool<geo::ChannelMapSetupTool>(fChannelMapParameters)
          ->setupChannelMap()
        ;
      
      std::unique_ptr<geo::GeoObjectSorter> ownedSorter;
      geo::GeoObjectSorter const* sorter
        = getSorter(ownedSorter, channelMap.get());
      
      ApplyChannelMap(channelMap, sorter); // will take ownership of the object
    }
    else {
      // the service is responsible of calling the channel map configuration
      // of the geometry
      geo::ExptGeoHelperInterface* helper = nullptr;
      try {
        helper = art::ServiceHandle<geo::ExptGeoHelperInterface>().get();
      }
      catch (art::Exception const& e) {
        if (e.categoryCode() != art::errors::ServiceNotFound) throw;
        throw cet::exception("Geometry")
          << "Can't create any channel mapping! Please either:"
          "\n1) configure a `ChannelMapSetupTool` in `Geometry` service"
          "\n2) configure a `ExptGeoHelperInterface` service"
          "\n";
      }
      if (helper)
        helper->ConfigureChannelMapAlg(fSortingParameters, this);
    } // if ... else
    
    if ( ! ChannelMap() ) {
      throw cet::exception("ChannelMapLoadFail")
        << " failed to load new channel map";
    }

  } // Geometry::InitializeChannelMap()
  
  
  //......................................................................
  geo::GeoObjectSorter const* Geometry::getSorter(
    std::unique_ptr<geo::GeoObjectSorter>& owned,
    geo::ChannelMapAlg const* channelMap /* = nullptr */
  ) const {
    
    /*
     * This method returns a object sorter:
     * 
     * 1) if a sorter configuration is specified, it loads a tool with that
     *    configuration
     * 2) if no sorter configuration is present, obtains a sorter from
     *    channel mapping
     * 3) if channel mapping is not available or throws a
     *    `geo::ChannelMapAlg::NoSorter` exception, no sorter is returned
     * 
     */
    
    // if no channel mapping is specified, use the one the class owns
    // (but it might not own any yet)
    if (!channelMap) channelMap = ChannelMap();
    
    geo::GeoObjectSorter const* sorter = nullptr;
    
    if (!fSorterParameters.is_empty()) {
      owned = art::make_tool<geo::GeoObjectSorterSetupTool>(fSorterParameters)
        ->setupSorter();
      sorter = owned.get();
    }
    else if (channelMap) {
      try {
        sorter = &(channelMap->Sorter());
      }
      catch (geo::ChannelMapAlg::NoSorter const& e) {
        MF_LOG_DEBUG("Geometry") << "Geometry::getSorter(): "
          "Channel mapping declined the use of a geometry sorter:\n"
          << e.what();
      }
    }
    else {
      MF_LOG_WARNING("Geometry")
        << "No channel mapping available: no sorting algorithm will be used.";
    }
    
    return sorter;
  } // Geometry::getSorter()
  
  
  //......................................................................
  void Geometry::LoadNewGeometry(
    std::string gdmlfile, std::string /* rootfile */,
    bool bForceReload /* = false */
  ) {
    // start with the relative path
    std::string GDMLFileName(fRelPath), ROOTFileName(fRelPath);

    // add the base file names
    ROOTFileName.append(gdmlfile); // not rootfile (why?)
    GDMLFileName.append(gdmlfile);

    // special for GDML if geometry with no wires is used for Geant4 simulation
    if(fDisableWiresInG4)
      GDMLFileName.insert(GDMLFileName.find(".gdml"), "_nowires");

    // Search all reasonable locations for the GDML file that contains
    // the detector geometry.
    // cet::search_path constructor decides if initialized value is a path
    // or an environment variable
    cet::search_path sp("FW_SEARCH_PATH");

    std::string GDMLfile;
    if( !sp.find_file(GDMLFileName, GDMLfile) ) {
      throw cet::exception("Geometry")
        << "cannot find the gdml geometry file:"
        << "\n" << GDMLFileName
        << "\nbail ungracefully.\n";
    }

    std::string ROOTfile;
    if( !sp.find_file(ROOTFileName, ROOTfile) ) {
      throw cet::exception("Geometry")
        << "cannot find the root geometry file:\n"
        << "\n" << ROOTFileName
        << "\nbail ungracefully.\n";
    }

    std::unique_ptr<geo::GeometryBuilder> builder
      = makeBuilder(fBuilderParameters);

    // initialize the geometry with the files we have found
    LoadGeometryFile(GDMLfile, ROOTfile, *builder, bForceReload);

    builder.reset(); // done with it: release immediately

    // now update the channel map
    InitializeChannelMap();

  } // Geometry::LoadNewGeometry()
  
  
  //......................................................................
  std::unique_ptr<geo::GeometryBuilder> Geometry::makeBuilder
    (fhicl::ParameterSet config)
  {
    if (!config.has_key("tool_type"))
      config.put("tool_type", "GeometryBuilderStandardTool");
    return art::make_tool<geo::GeometryBuilderTool>(config)->makeBuilder();
  } // Geometry::makeBuilder()
  
  
  //......................................................................
  
  DEFINE_ART_SERVICE(Geometry)
} // namespace geo
