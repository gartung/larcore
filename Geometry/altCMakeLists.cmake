set (HEADERS
     AuxDetGeo.h
     ChannelMapAlg.h
     ChannelMapStandardAlg.h
     CryostatGeo.h
     ExptGeoHelperInterface.h
     GeoObjectSorter.h
     GeoObjectSorterStandard.h
     Geometry.h
     OpDetGeo.h
     PlaneGeo.h
     StandardGeometryHelper.h
     TPCGeo.h
     WireGeo.h
     geo.h
     )	

add_library(Geometry SHARED
     ${HEADERS}
     AuxDetGeo.cxx
     ChannelMapAlg.cxx
     ChannelMapStandardAlg.cxx
     CryostatGeo.cxx
     GeoObjectSorter.cxx
     GeoObjectSorterStandard.cxx
     OpDetGeo.cxx
     PlaneGeo.cxx
     TPCGeo.cxx
     WireGeo.cxx
     )

target_link_libraries(Geometry 
     art::art_Framework_Core
     art::art_Framework_IO_Sources
     art::art_Framework_Principal
     art::art_Persistency_Provenance
     art::art_Utilities
     FNALCore::FNALCore
     ${CLHEP_LIBRARIES}
     ${ROOT_BASIC_LIB_LIST}
     ${ROOT_GEOM}
     ${ROOT_XMLIO}
     ${ROOT_GDML}
     ${ROOT_EG}
     )

target_include_directories(Geometry
     PUBLIC
     ${ROOT_INCLUDE_DIRS}
     ${CLHEP_INCLUDE_DIRS}
     )

art_add_service(Geometry_service
     Geometry_service.cc
     )


art_add_service(StandardGeometryHelper_service
     StandardGeometryHelper_service.cc
     )

install(TARGETS
     Geometry
     Geometry_service
     StandardGeometryHelper_service
     EXPORT larcoreLibraries
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
     COMPONENT Runtime
     )

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Geometry COMPONENT Development)

file(GLOB FHICL_FILES 
     [^.]*.fcl
)

install(FILES ${FHICL_FILES} DESTINATION job COMPONENT Runtime)

add_subdirectory(gdml)

