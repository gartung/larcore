IF(ALT_CMAKE)
INCLUDE(altCMakeLists.cmake)
ELSE()

art_make( LIBRARY_NAME SummaryData
          LIB_LIBRARIES ${ART_PERSISTENCY_COMMON}
         )

install_headers()
install_source()
ENDIF()
