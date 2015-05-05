/**
 * @file   Geometry_test.cxx
 * @brief  Unit test for geometry functionalities on a standard detector
 * @date   May 5th, 2015
 * @author petrillo@fnal.gov
 * 
 * Usage:
 *   Geometry_test  ConfigurationFile [GeometryTestParameterSet]
 * 
 * By default, GeometryTestParameterSet is "physics.analysers.geotest".
 * 
 */


// LArSoft libraries
#include "test/Geometry/GeometryTestAlg.h"
#include "Geometry/GeometryCore.h"
#include "Geometry/ChannelMapStandardAlg.h"

// utility libraries
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/intermediate_table.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "fhiclcpp/parse.h"
// #include "fhiclcpp/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// CET libraries
#include "cetlib/filepath_maker.h"

// C/C++ standard libraries
#include <iostream> // for output before message facility is set up
#include <string>
#include <memory> // std::unique_ptr<>



/** ****************************************************************************
 * @brief Creates a full configuration for the test
 * @return a parameters set with the complete configuration
 */
fhicl::ParameterSet ConfigureGeometryTest() {
  
  const std::string GeometryConfigurationString = R"(
    services: {
      Geometry: {
        SurfaceY:        200.  # in cm, vertical distance to the surface
        Name:            "lartpcdetector"
        GDML:            "LArTPCdetector.gdml"
        ROOT:            "LArTPCdetector.gdml"
        SortingParameters: {}  # empty parameter set for default
      } # Geometry
    } # services
    
    physics: {
      analyzers: {
        geotest: {
          module_type: "GeometryTest"
          PrintWires:  true
        }
      }
    } # physics
  )";
  
  fhicl::ParameterSet global_pset;
  fhicl::make_ParameterSet(GeometryConfigurationString, global_pset);
  
  return global_pset;
} // ConfigureGeometryTest()


/** ****************************************************************************
 * @brief Returns the configuration from a FHiCL file
 * @param config_path full path of the FHiCL configuration file
 * @return a parameters set with the complete configuration from the file
 */
fhicl::ParameterSet ParseConfiguration(std::string config_path) {
  // simple file lookup policy: assume the file name specification is complete
  cet::filepath_maker policy;
  
  // parse a configuration file; obtain intermediate form
  fhicl::intermediate_table table;
  fhicl::parse_document(config_path, policy, table);
  
  // translate into a parameter set
  fhicl::ParameterSet global_pset;
  fhicl::make_ParameterSet(table, global_pset);
  
  return global_pset;
} // ParseConfiguration()


/** ****************************************************************************
 * @brief Returns the geometry configuration
 * @param config_path full path of the FHiCL configuration file
 * @return a parameters set with the complete configuration from the file
 * 
 * If config_path is empty, a hard-coded configuration is used.
 */
fhicl::ParameterSet Configure(std::string config_path = std::string()) {
  
  return config_path.empty()?
    ConfigureGeometryTest(): ParseConfiguration(config_path);
  
} // Configure()


/** ****************************************************************************
 * @brief Sets the message facility up
 * @param Configuration the complete FHiCL configuration
 * @param ApplicationName name of the application running (shows in the output)
 * 
 * Message facility configuration is expected in "services.message" parameter
 * set. If not there, the default configuration is used.
 */
void SetupMessageFacility(
  fhicl::ParameterSet const& Configuration,
  std::string ApplicationName = std::string()
) {
  // initialize the message facility
  fhicl::ParameterSet mf_pset;
  if (!Configuration.get_if_present("services.message", mf_pset)) {
    // a destination which will react to all messages from DEBUG up
    std::string MessageFacilityConfiguration = R"(
    destinations : {
      stdout: {
        type:      cout
        threshold: DEBUG
        categories: {
          default: {
            limit: -1
          }
        } // categories
      } // stdout
    } // destinations
    statistics: cout
    )";
    fhicl::make_ParameterSet(MessageFacilityConfiguration, mf_pset);
    std::cout << "Using default message facility configuration:\n"
      << mf_pset.to_indented_string(1) << std::endl;
  } // if no configuration is available
  
  mf::StartMessageFacility(mf::MessageFacilityService::SingleThread, mf_pset);
  if (!ApplicationName.empty()) mf::SetApplicationName(ApplicationName);
  mf::SetContext("Initialization");
  mf::LogInfo("MessageFacility") << "MessageFacility started.";
  mf::SetModuleName("main");
} // SetupMessageFacility()


/** ****************************************************************************
 * @brief Sets the geometry of the standard detector up
 * @tparam ChannelMapClass the class of ChannelMapAlg to set up
 * @param Configuration the complete configuration
 * @param GeoParameterPath where to find geometry configuration
 * @param HelperParameterPath where to find geometry helper configuration
 * @return a new GeometryCore object ready to be used
 * 
 * This function sets up the geometry according to the provided information:
 * - the configuration must contain enough information to locate the geometry
 *   description file
 * - we trust that that geometry works well with the specified ChannelMapClass
 * 
 */
template <class ChannelMapClass>
std::unique_ptr<geo::GeometryCore> SetupGeometry(
  fhicl::ParameterSet const& Configuration,
  std::string GeoParameterPath = "services.Geometry"
) {
  
  //
  // create the new geometry service provider
  //
  fhicl::ParameterSet GeoConfig
    = Configuration.get<fhicl::ParameterSet>(GeoParameterPath);
  std::unique_ptr<geo::GeometryCore> geom(new geo::GeometryCore(GeoConfig));
  
  std::string RelativePath = GeoConfig.get< std::string>("RelativePath", "");
  
  std::string GDMLFileName = RelativePath + GeoConfig.get<std::string>("GDML");
  std::string ROOTFileName = RelativePath + GeoConfig.get<std::string>("ROOT");
  
  // Search all reasonable locations for the geometry file;
  // we see if by any chance art's FW_SEARCH_PATH directory is set and try
  // there;
  // if not, we do expect the path to be complete enough for ROOT to figure out.
  cet::search_path sp("FW_SEARCH_PATH");
  
  std::string ROOTfile;
  if (!sp.find_file(ROOTFileName, ROOTfile)) ROOTfile = ROOTFileName;
  
  // we really don't care of GDML file, since we are not going to run Geant4
  std::string GDMLfile;
  if (!sp.find_file(GDMLFileName, GDMLfile))
    mf::LogWarning("SetupGeometry") << "GDML file not found.";
  
  // initialize the geometry with the files we have found
  geom->LoadGeometryFile(GDMLfile, ROOTfile);
  
  
  //
  // create the new channel map
  //
  fhicl::ParameterSet SortingParameters
    = GeoConfig.get<fhicl::ParameterSet>("SortingParameters");
  std::shared_ptr<geo::ChannelMapAlg> pChannelMap
    (new ChannelMapClass(SortingParameters));
  
  // connect the channel map with the geometry
  geom->ApplyChannelMap(pChannelMap);
  
  return geom;
} // SetupGeometry()



/** ****************************************************************************
 * @brief Runs the test
 * @param argc number of arguments in argv
 * @param argv arguments to the function
 * @return number of detected errors (0 on success)
 * @throw cet::exception most of error situations throw
 * 
 * The arguments in argv are:
 * 0. name of the executable ("Geometry_test")
 * 1. path to the FHiCL configuration file
 * 2. FHiCL path to the configuration of the geometry test
 *    (default: physics.analysers.geotest)
 * 3. FHiCL path to the configuration of the geometry
 *    (default: services.Geometry)
 * 
 */
//------------------------------------------------------------------------------
int main(int argc, char const** argv) {
  // here we have to figure out everything: no framework helping!
  
  //
  // parameter parsing
  //
  int iParam = 0;
  
  // first argument: configuration file (mandatory)
  std::string ConfigurationFilePath = (++iParam < argc)? argv[iParam]: "";
  
  // second argument: path of the parameter set for geometry test configuration
  // (optional; default: "physics.analysers.geotest")
  std::string GeometryTestParameterSetPath
    = (++iParam < argc)? argv[iParam]: "physics.analyzers.geotest";
  
  // third argument: path of the parameter set for geometry configuration
  // (optional; default: "services.Geometry")
  std::string GeometryParameterSetPath
    = (++iParam < argc)? argv[iParam]: "services.Geometry";
  
  
  //
  // get the configuration
  //
  fhicl::ParameterSet Configuration = Configure(ConfigurationFilePath);
  
  //
  // set up the message facility
  //
  SetupMessageFacility(Configuration, "Geometry_test");
  
  
  //
  // set up the geometry
  //
  std::unique_ptr<geo::GeometryCore> geom
    = SetupGeometry<geo::ChannelMapStandardAlg>
    (Configuration, GeometryParameterSetPath);
  
  
  //
  // Run the test
  //
  geo::GeometryTestAlg Tester
    (Configuration.get<fhicl::ParameterSet>(GeometryTestParameterSetPath));
  Tester.Configure(geom.get());
  unsigned int nErrors = Tester.Run();
  
  //
  // Let the test complain if it wants, and we don't catch exceptions neither
  //
  return (int) nErrors;
} // main()
