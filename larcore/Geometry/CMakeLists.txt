
art_make( 
          LIBRARY_NAME  Geometry
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
          SERVICE_LIBRARIES Geometry
                            ${ART_FRAMEWORK_CORE}
                            ${ART_FRAMEWORK_IO_SOURCES}
                            ${ART_FRAMEWORK_PRINCIPAL}
                            ${ART_PERSISTENCY_COMMON}
                            ${ART_PERSISTENCY_PROVENANCE}
                            ${ART_UTILITIES}
                            ${ART_FRAMEWORK_SERVICES_REGISTRY}
                            ${MF_MESSAGELOGGER}
                            ${MF_UTILITIES}
                            ${FHICLCPP}
                            ${CETLIB}
          MODULE_LIBRARIES Geometry
#                           ${ART_FRAMEWORK_CORE}
#                           ${ART_FRAMEWORK_IO_SOURCES}
                           ${ART_FRAMEWORK_PRINCIPAL}
#                           ${ART_PERSISTENCY_COMMON}
#                           ${ART_PERSISTENCY_PROVENANCE}
                           ${ART_UTILITIES}
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
