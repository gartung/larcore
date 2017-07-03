
art_make( 
          
          LIB_LIBRARIES ${MF_MESSAGELOGGER}
                        ${MF_UTILITIES}
                        ${FHICLCPP}
                        cetlib cetlib_except
                        ${CLHEP}
                        ${ROOT_BASIC_LIB_LIST}
                        ${ROOT_GEOM}
                        ${ROOT_XMLIO}
                        ${ROOT_GDML}
                        ${ROOT_EG}
                        ${ROOT_GENVECTOR}
          SERVICE_LIBRARIES larcore_Geometry
                            art_Framework_Principal
                            art_Framework_Services_Registry
                            cetlib cetlib_except
                            ${MF_MESSAGELOGGER}
                            ${MF_UTILITIES}
                            ${FHICLCPP}
          MODULE_LIBRARIES larcore_Geometry
                           art_Framework_Core
                           art_Framework_Services_Registry
                           canvas
                           ${MF_MESSAGELOGGER}
                           ${MF_UTILITIES}
                           ${FHICLCPP}
                           cetlib cetlib_except
          )

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
