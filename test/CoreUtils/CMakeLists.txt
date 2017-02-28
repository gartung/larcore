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
cet_test( RealComparisons_test USE_BOOST_UNIT )
cet_test( DereferenceIterator_test USE_BOOST_UNIT )
cet_test( DumpUtils_test
  LIBRARIES
    ${ROOT_CORE}
    ${ROOT_PHYSICS}
  USE_BOOST_UNIT
  )

# test quiet_Math_Functor.h
get_property(IncludeDirectories DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

cet_test(MathFunctor_workaround HANDBUILT
  TEST_EXEC sh
  TEST_ARGS quiet_Math_Functor_test.sh ${IncludeDirectories}
  DATAFILES quiet_Math_Functor_test.sh
  )
