cmake_minimum_required(VERSION 2.8.12)

if(POLICY CMP0025)
    cmake_policy(SET CMP0025 OLD)
endif()

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()


project(larcore)


set(larcore_VERSION "04.00.01")
set(larcore_VERSION_MAJOR 04)
set(larcore_VERSION_MINOR 00)
set(larcore_VERSION_PATCH 01)

set(larcore_SOVERSION "1.0.0")


set(larcore_DEBUG_POSTFIX "d")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules ${CMAKE_MODULE_PATH})

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

set(BASE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/BuildProducts")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIRECTORY}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIRECTORY}/${CMAKE_INSTALL_LIBDIR}")

find_package(art 1.11.3 REQUIRED)

find_package(FNALCore 0.1.0 REQUIRED)

set(art_MIN_BOOST_VERSION "1.53.0")

if(FNALCore_BOOST_VERSION VERSION_LESS art_MIN_BOOST_VERSION)
     message(FATAL_ERROR "Located version of FNALCore compiled against Boost ${FNALCore_BOOST_VERSION}\nart requires Boost >= ${art_MIN_BOOST_VERSION}")
endif()

find_package(Boost ${art_MIN_BOOST_VERSION}
     REQUIRED
       date_time
       unit_test_framework
       program_options
     )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FNALCore_CXX_FLAGS} -O3 -g -DNDEBUG -fno-omit-frame-pointer")

find_package(GCCXML 0.9.0 REQUIRED)

find_package(CLHEP 2.2.0.3 REQUIRED)

find_package(SQLite3 3.8.5 REQUIRED)

find_package(ROOT 5.34.20 REQUIRED
     Core
     Cint
     Cintex
     Hist
     Matrix
     Reflex
     RIO
     Thread
     Tree
    )

if(NOT ROOT_python_FOUND)
     message(FATAL_ERROR "art requires ROOT with Python support")
endif()

find_package(TBB 4.1.0 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})
include_directories(${art_INCLUDE_DIRS})
include_directories(${ROOT_INCLUDE_DIRS})
include_directories(${CLHEP_INCLUDE_DIRS})
include_directories(${FNALCore_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})


# source
add_subdirectory(Geometry)
add_subdirectory(SimpleTypesAndConstants)
add_subdirectory(SummaryData)

# tests 
add_subdirectory(test)

configure_package_config_file(
  Modules/larcoreConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/larcoreConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/larcore-${larcore_VERSION}
  PATH_VARS
    CMAKE_INSTALL_INCLUDEDIR
    CMAKE_INSTALL_LIBDIR
  )

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/larcoreConfigVersion.cmake
  VERSION ${larcore_VERSION}
  COMPATIBILITY AnyNewerVersion
  )

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/larcoreConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/larcoreConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/larcore-${larcore_VERSION}
  COMPONENT Development
  )

install(EXPORT larcoreLibraries
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/larcore-${larcore_VERSION}
  NAMESPACE larsoft::
  COMPONENT Development
  )

# packaging utility
include(ArtCPack)
