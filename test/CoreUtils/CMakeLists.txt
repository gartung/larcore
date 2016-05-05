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
    ${ART_UTILITIES}
    ${MF_MESSAGELOGGER}
    ${MF_UTILITIES}
    ${FHICLCPP}
    ${CETLIB}
  USE_BOOST_UNIT
  )

cet_test(UncopiableAndUnmoveableClass_test
  USE_BOOST_UNIT
  )

cet_test( ProviderPack_test USE_BOOST_UNIT )

