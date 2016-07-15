
cet_test(NameSelector_test
  SOURCES NameSelector_test.cxx
  LIBRARIES
    larcore_TestUtils
    art_Utilities canvas_Utilities
  USE_BOOST_UNIT
)

cet_test(ProviderList_test
  USE_BOOST_UNIT
)

cet_test(StopWatch_test)
