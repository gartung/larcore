IF(ALT_CMAKE)
INCLUDE(altCMakeLists.cmake)
ELSE()
# ======================================================================
#
# Testing
#
# ======================================================================

cet_test( testPhysicalConstants )

ENDIF()
