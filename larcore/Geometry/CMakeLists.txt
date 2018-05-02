
art_make( 
          SERVICE_LIBRARIES larcorealg_Geometry
                            art_Framework_Principal
                            art_Framework_Services_Registry
                            cetlib cetlib_except
                            ${MF_MESSAGELOGGER}
                            
                            ${FHICLCPP}
                            ${ROOT_BASIC_LIB_LIST}
          MODULE_LIBRARIES larcorealg_Geometry
                           art_Framework_Core
                           art_Framework_Services_Registry
                           canvas
                           ${MF_MESSAGELOGGER}
                           
                           ${FHICLCPP}
                           cetlib cetlib_except
          )

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
