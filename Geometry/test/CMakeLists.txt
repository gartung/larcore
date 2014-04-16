cet_enable_asserts()

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


install_headers()
install_fhicl()
install_scripts( LIST debuggeo.csh
                      testgeo.csh
		      testgeo.pl
               )
install_source()
