
art_make( 
          
          LIB_LIBRARIES ${MF_MESSAGELOGGER}
                        ${MF_UTILITIES}
                        ${FHICLCPP}
                        ${CETLIB}
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
                            ${CETLIB}
                            ${MF_MESSAGELOGGER}
                            ${MF_UTILITIES}
                            ${FHICLCPP}
          MODULE_LIBRARIES larcore_Geometry
                           art_Framework_Core
                           art_Framework_Services_Registry
                           canvas_Utilities
                           ${MF_MESSAGELOGGER}
                           ${MF_UTILITIES}
                           ${FHICLCPP}
                           ${CETLIB}
          )

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
