# ======================================================================
#
# Testing
#
# ======================================================================

cet_test(ServiceUtil_test
  LIBRARIES
    art_Utilities canvas
    ${MF_MESSAGELOGGER}
    
    ${FHICLCPP}
    cetlib cetlib_except
  USE_BOOST_UNIT
  )

