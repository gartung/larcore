IF(ALT_CMAKE)
INCLUDE(altCMakeLists.cmake)
ELSE()

set( gdml_install_dir "${product}/${version}/gdml" )

# special note: 
# Preferred solution is to not install GDMLSchema files 
# and instead, usd the version found in geant4 at GDML_SCHEMA_DIR,
# but this is not working. So revert to installing the
# schema files:
install_gdml( SUBDIRS GDMLSchema )

# install gdml executables
file(GLOB gdml_bin *.pl genmake )
install( PROGRAMS ${gdml_bin} DESTINATION ${gdml_install_dir} )

ENDIF()
