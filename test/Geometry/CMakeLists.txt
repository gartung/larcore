cet_enable_asserts()

# test libraries
art_make_library( LIBRARY_NAME GeometryTestLib
                  SOURCE
                    GeometryTestAlg.cxx
                    GeometryIteratorTestAlg.cxx
                    GeometryIteratorLoopTestAlg.cxx
                    ChannelMapStandardTestAlg.cxx
                  LIBRARIES larcore_Geometry
                        larcore_TestUtils
                        ${ART_UTILITIES}
                        ${MF_MESSAGELOGGER}
                        ${MF_UTILITIES}
                        ${FHICLCPP}
                        ${CETLIB}
                        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
          )


# test modules
simple_plugin ( GeometryIteratorLoopTest "module"
                    larcore_Geometry
                    GeometryTestLib
                    larcore_Geometry_Geometry_service
                    ${MF_MESSAGELOGGER}
                    ${MF_UTILITIES}
                    ${FHICLCPP}
                    ${CETLIB}
             )

simple_plugin ( GeometryTest "module"
                    larcore_Geometry
                    GeometryTestLib
                    larcore_Geometry_Geometry_service
                    ${MF_MESSAGELOGGER}
                    ${MF_UTILITIES}
                    ${FHICLCPP}
                    ${CETLIB}
                    ${ROOT_BASIC_LIB_LIST}
              )

# geometry test on "standard" geometry

# unit test (use geometry configuration from config file, the rest is ignored)
# this uses BOOST for the test
# This test is temporary marked ONLY_SLF since on OSX it fails
# for reasons unrelated to the geometry test itself
# (untimely initialization of the Boost test framework)
cet_test(geometry_iterator_test
  SOURCES geometry_iterator_test.cxx
  LIBRARIES larcore_Geometry
            GeometryTestLib
            ${MF_MESSAGELOGGER}
            ${MF_UTILITIES}
            ${FHICLCPP}
            ${CETLIB}
  USE_BOOST_UNIT
  DATAFILES test_geometry.fcl
  TEST_ARGS test_geometry.fcl
#  OPTIONAL_GROUPS ONLY_SLF
)

# unit test (use the hard-coded channel mapping for "standard" LArTPCdetector)
cet_test(geometry_test
  SOURCES geometry_test.cxx
  DATAFILES test_geometry.fcl
  TEST_ARGS ./test_geometry.fcl
  LIBRARIES larcore_Geometry
            GeometryTestLib
            ${MF_MESSAGELOGGER}
            ${MF_UTILITIES}
            ${FHICLCPP}
            ${CETLIB}
  USE_BOOST_UNIT
)

# unit test (use the hard-coded channel mapping for "standard" LArTPCdetector)
cet_test(geometry_iterator_loop_test
  SOURCES geometry_iterator_loop_test.cxx
  DATAFILES test_geometry_iterator_loop.fcl
  TEST_ARGS ./test_geometry_iterator_loop.fcl
  LIBRARIES larcore_Geometry
            GeometryTestLib
            ${MF_MESSAGELOGGER}
            ${MF_UTILITIES}
            ${FHICLCPP}
            ${CETLIB}
)


# test for standard channel mapping (BOOST unit test)
cet_test(geometry_standardchannelmapping_test
  SOURCES geometry_standardchannelmapping_test.cxx
  LIBRARIES larcore_Geometry
            GeometryTestLib
            ${MF_MESSAGELOGGER}
            ${MF_UTILITIES}
            ${FHICLCPP}
            ${CETLIB}
  USE_BOOST_UNIT
  DATAFILES test_geometry.fcl
  TEST_ARGS ./test_geometry.fcl
)


# simple unit test
# this uses BOOST for the test
cet_test(geometry_thirdplaneslope_test
  SOURCES geometry_thirdplaneslope_test.cxx
  LIBRARIES larcore_Geometry
            ${MF_MESSAGELOGGER}
            ${MF_UTILITIES}
            ${FHICLCPP}
            ${CETLIB}
  USE_BOOST_UNIT
  DATAFILES test_geometry.fcl
  TEST_ARGS test_geometry.fcl
)


# these tests run a FCL file and fail only if lar exits with a bad exit code;
# they are fast and always run.
# FCL files need to be copied to the test area (DATAFILES directive) since they
# are not installed.
# 
# This test is equivalent to geometry_test, but run in art environment
cet_test(geometry HANDBUILT
  DATAFILES test_geometry.fcl
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./test_geometry.fcl
)

# FCL files need to be copied to the test area (DATAFILES directive) since they
# are not installed.
# 
# This test is equivalent to geometry_iterator_loop_test, but run in art environment
cet_test(geometry_iterator HANDBUILT
  DATAFILES test_geometry_iterator_loop.fcl
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./test_geometry_iterator_loop.fcl
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


# FCL files need to be copied to the test area (DATAFILES directive) since they
# are not installed.
cet_test(dump_channel_map_test HANDBUILT
  DATAFILES dump_lartpcdetector_channelmap.fcl
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./dump_lartpcdetector_channelmap.fcl
)


install_headers()
install_fhicl()
install_source()
