
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
          SERVICE_LIBRARIES larcore_Geometry
                            ${ART_FRAMEWORK_CORE}
                            ${ART_FRAMEWORK_IO_SOURCES}
                            ${ART_FRAMEWORK_PRINCIPAL}
                            art_Persistency_Common canvas_Persistency_Common
                            art_Persistency_Provenance canvas_Persistency_Provenance
                            art_Utilities canvas_Utilities
                            ${ART_FRAMEWORK_SERVICES_REGISTRY}
                            ${MF_MESSAGELOGGER}
                            ${MF_UTILITIES}
                            ${FHICLCPP}
                            ${CETLIB}
          MODULE_LIBRARIES larcore_Geometry
#                           ${ART_FRAMEWORK_CORE}
#                           ${ART_FRAMEWORK_IO_SOURCES}
                           ${ART_FRAMEWORK_PRINCIPAL}
#                           art_Persistency_Common canvas_Persistency_Common
#                           art_Persistency_Provenance canvas_Persistency_Provenance
                           art_Utilities canvas_Utilities
                           ${ART_FRAMEWORK_SERVICES_REGISTRY}
                           ${MF_MESSAGELOGGER}
                           ${MF_UTILITIES}
                           ${FHICLCPP}
                           ${CETLIB}
          )

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
