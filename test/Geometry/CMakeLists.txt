IF(ALT_CMAKE)
INCLUDE(altCMakeLists.cmake)
ELSE()
cet_enable_asserts()

simple_plugin ( GeometryIteratorTest "module"
                    Geometry
                    Geometry_service
                    ${MF_MESSAGELOGGER}
                    ${MF_UTILITIES}
                    ${FHICLCPP}
                    ${CETLIB}
               BASENAME_ONLY
             )

simple_plugin ( GeometryTest "module"
                    Geometry
                    Geometry_service
                    ${MF_MESSAGELOGGER}
                    ${MF_UTILITIES}
                    ${FHICLCPP}
                    ${CETLIB}
                    ${ROOT_BASIC_LIB_LIST}
                BASENAME_ONLY
              )

# geometry test on "standard" geometry

# these tests run a FCL file and fail only if lar exits with a bad exit code;
# they are fast and always run.
# FCL files need to be copied to the test area (DATAFILES directive) since they
# are not installed.
cet_test(geometry HANDBUILT
  DATAFILES test_geometry.fcl
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./test_geometry.fcl
)

# FCL files need to be copied to the test area (DATAFILES directive) since they
# are not installed.
cet_test(geometry_iterator HANDBUILT
  DATAFILES test_geometry_iterators.fcl
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./test_geometry_iterators.fcl
)

# set FHICL_FILE_PATH and FW_SEARCH_PATH for the case where building without mrb
# this is not generic, we take advantage of knowing the subdirectories for the fcl and gdml files
set( mrb_build_dir $ENV{MRB_BUILDDIR} )
if( NOT mrb_build_dir )
  #message(STATUS "MRB_BUILDDIR is undefined, set FHICL_FILE_PATH and FW_SEARCH_PATH")
  set_tests_properties( geometry geometry_iterator 
                        PROPERTIES ENVIRONMENT 
                        "FHICL_FILE_PATH=.:${CMAKE_BINARY_DIR}/job;FW_SEARCH_PATH=${CMAKE_BINARY_DIR}/gdml" )
endif()

install_headers()
install_fhicl()
install_source()
ENDIF()
