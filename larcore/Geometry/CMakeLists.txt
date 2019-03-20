art_make(SERVICE_LIBRARIES larcorealg_Geometry
                           art_Framework_Principal
                           art_Persistency_Provenance
                           ${MF_MESSAGELOGGER}
                           ${ROOT_CORE}
         MODULE_LIBRARIES larcorealg_Geometry
                          art_Framework_Services_Registry
                          ${MF_MESSAGELOGGER}
                          ${ROOT_GEOM}
                          ${ROOT_GENVECTOR})

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
