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


install_headers()
install_fhicl()
install_source()
