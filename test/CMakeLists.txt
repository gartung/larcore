IF(ALT_CMAKE)
INCLUDE(altCMakeLists.cmake)
ELSE()

include(CetTest)
add_subdirectory(SimpleTypesAndConstants)
add_subdirectory(Geometry)
ENDIF()
