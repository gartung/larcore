
art_add_module(GeometryTest_module
     GeometryTest_module.cc
     )

target_include_directories(GeometryTest_module
     PUBLIC
     ${ROOT_INCLUDE_DIRS}
     ${CLHEP_INCLUDE_DIRS}
     )

art_add_module(GeometryIteratorTest_module
     GeometryIteratorTest_module.cc
     )

target_include_directories(GeometryIteratorTest_module
     PUBLIC
     ${ROOT_INCLUDE_DIRS}
     ${CLHEP_INCLUDE_DIRS}
     )

install(TARGETS
     GeometryTest_module
     GeometryIteratorTest_module
     EXPORT larcoreLibraries
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
     COMPONENT Runtime
     )


