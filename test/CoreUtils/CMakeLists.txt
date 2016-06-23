# ======================================================================
#
# Testing
#
# ======================================================================

cet_test( printBacktrace_test
  LIBRARIES
    larcore_CoreUtils
  )

cet_test(ServiceUtil_test
  LIBRARIES
    art_Utilities canvas_Utilities
    ${MF_MESSAGELOGGER}
    ${MF_UTILITIES}
    ${FHICLCPP}
    ${CETLIB}
  USE_BOOST_UNIT
  )

cet_test(UncopiableAndUnmovableClass_test
  USE_BOOST_UNIT
  )

cet_test( ProviderPack_test USE_BOOST_UNIT )

