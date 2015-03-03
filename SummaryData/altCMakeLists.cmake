
set(HEADERS
	POTSummary.h
	RunData.h
	)

add_library(SummaryData SHARED
	${HEADERS}
	POTSummary.cxx
	RunData.cxx
	)

target_link_libraries(SummaryData
     art::art_Persistency_Common
     )


art_add_dictionary(DICTIONARY_LIBRARIES art::art_Framework_Core)

install(TARGETS
     SummaryData
     SummaryData_map
     SummaryData_dict
     EXPORT larcoreLibraries
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
     COMPONENT Runtime
     )

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SummaryData COMPONENT Development)

