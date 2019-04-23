# ======================================================================
#
# Testing
#
# ======================================================================

cet_test(ServiceUtil_test
  LIBRARIES
    ${ART_FRAMEWORK_SERVICES_REGISTRY}
    ${ART_UTILITIES}
    ${CANVAS}
    ${MF_MESSAGELOGGER}
    ${FHICLCPP}
    ${CETLIB_EXCEPT}
  USE_BOOST_UNIT
  )

