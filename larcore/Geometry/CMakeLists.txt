art_make(SERVICE_LIBRARIES larcorealg_Geometry
                           art_Framework_Principal
                           art_Persistency_Provenance
                           ${MF_MESSAGELOGGER}
                           ROOT::Core
         MODULE_LIBRARIES larcorealg_Geometry
                          art_Framework_Services_Registry
                          ${MF_MESSAGELOGGER}
                          ROOT::Geom
                          ROOT::GenVector )

install_headers()
install_fhicl()

add_subdirectory(gdml)
