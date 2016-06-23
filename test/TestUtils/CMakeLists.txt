
cet_test(NameSelector_test
  SOURCES NameSelector_test.cxx
  LIBRARIES
    larcore_TestUtils
    ${ART_UTILITIES}
  USE_BOOST_UNIT
)

cet_test(ProviderList_test
  USE_BOOST_UNIT
)
