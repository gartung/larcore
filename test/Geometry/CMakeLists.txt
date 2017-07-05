cet_enable_asserts()

# test modules
simple_plugin ( GeometryIteratorLoopTest "module"
                    larcorealg_Geometry
                    GeometryTestLib
                    larcore_Geometry_Geometry_service
                    ${MF_MESSAGELOGGER}
                    ${MF_UTILITIES}
                    ${FHICLCPP}
                    cetlib cetlib_except
             )

simple_plugin ( GeometryTest "module"
                    larcorealg_Geometry
                    GeometryTestLib
                    larcore_Geometry_Geometry_service
                    ${MF_MESSAGELOGGER}
                    ${MF_UTILITIES}
                    ${FHICLCPP}
                    cetlib cetlib_except
                    ${ROOT_BASIC_LIB_LIST}
              )

# geometry test on "standard" geometry

# This test is equivalent to geometry_test, but run in art environment
cet_test(geometry HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./test_geometry.fcl
)

# FCL files need to be copied to the test area (DATAFILES directive) since they
# are not installed.
# 
# This test is equivalent to geometry_iterator_loop_test, but run in art environment
cet_test(geometry_iterator_loop HANDBUILT
  TEST_EXEC lar
  TEST_ARGS --rethrow-all --config ./test_geometry_iterator_loop.fcl
)

# set FHICL_FILE_PATH and FW_SEARCH_PATH for the case where building without mrb
# this is not generic, we take advantage of knowing the subdirectories for the fcl and gdml files
set( mrb_build_dir $ENV{MRB_BUILDDIR} )
if( NOT mrb_build_dir )
  #message(STATUS "MRB_BUILDDIR is undefined, set FHICL_FILE_PATH and FW_SEARCH_PATH")
  set_tests_properties( geometry geometry_iterator_loop
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
