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

install_headers()
install_fhicl()
install_source()
