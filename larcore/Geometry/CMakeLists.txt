art_make(
         EXCLUDE
           "GeometryBuilderStandardTool_tool.cc"
           "GeometryBuilderWirelessTool_tool.cc"
         SERVICE_LIBRARIES larcorealg_Geometry
                           art_Framework_Principal
                           art_Persistency_Provenance
                           ${MF_MESSAGELOGGER}
                           ${ROOT_CORE}
         MODULE_LIBRARIES larcorealg_Geometry
                          art_Framework_Services_Registry
                          ${MF_MESSAGELOGGER}
                          ${ROOT_CORE}
                          ${ROOT_GEOM}
                          ${ROOT_GENVECTOR})

simple_plugin(GeometryBuilderStandardTool "tool"
  larcorealg_Geometry
  )
simple_plugin(GeometryBuilderWirelessTool "tool"
  larcorealg_Geometry
  )

install_headers()
install_fhicl()
install_source()

add_subdirectory(gdml)
