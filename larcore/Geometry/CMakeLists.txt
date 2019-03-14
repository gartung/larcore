art_make(SERVICE_LIBRARIES larcorealg_Geometry
                            art_Framework_Principal
                            art_Persistency_Provenance
                            ${MF_MESSAGELOGGER}
          MODULE_LIBRARIES larcorealg_Geometry
                          art_Framework_Services_Registry)

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
