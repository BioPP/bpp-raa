# SPDX-FileCopyrightText: The Bio++ Development Group
#
# SPDX-License-Identifier: CECILL-2.1

cmake_minimum_required (VERSION 3.5)
project (bpp-raa3 C CXX)

# Compile options
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
add_compile_options(-Wall)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Libtool-like version number
# CURRENT:REVISION:AGE => file.so.(C-A).A.R
# current:  The most recent interface number that this library implements.
# revision: The implementation number of the current interface.
# age:      The difference between the newest and oldest interfaces that this
#           library implements.
# In other words, the library implements all the interface numbers in the
# range from number current - age to current.
SET(${PROJECT_NAME}_VERSION_CURRENT "1")
SET(${PROJECT_NAME}_VERSION_REVISION "0")
SET(${PROJECT_NAME}_VERSION_AGE "0")

#static linkage?
IF(NOT BUILD_STATIC)
	SET(BUILD_STATIC FALSE CACHE BOOL
	  "Compile and install static libraries."
      FORCE)
ENDIF()
IF(BUILD_STATIC)
  MESSAGE(STATUS "Static libraries requested.")
ENDIF()

# Effective version number computation
MATH(EXPR ${PROJECT_NAME}_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_CURRENT} - ${${PROJECT_NAME}_VERSION_AGE}")
SET(${PROJECT_NAME}_VERSION_MINOR ${${PROJECT_NAME}_VERSION_AGE})
SET(${PROJECT_NAME}_VERSION_PATCH ${${PROJECT_NAME}_VERSION_REVISION})
SET(${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

set (PROJECT_VERSION ${${PROJECT_NAME}_VERSION})
include (GNUInstallDirs)
find_package (bpp-seq3 1.0.0 REQUIRED)

# Find the zlib installation (define zlib imported target)
find_package(ZLIB REQUIRED)
add_library (zlib IMPORTED UNKNOWN)
set_target_properties (zlib PROPERTIES
  IMPORTED_LOCATION ${ZLIB_LIBRARIES}
  INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIR}
  )

# CMake package
set (cmake-package-location ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
include (CMakePackageConfigHelpers)
configure_package_config_file (
  package.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/package.cmake
  INSTALL_DESTINATION ${cmake-package-location}
  )
write_basic_package_version_file (
  ${CMAKE_CURRENT_BINARY_DIR}/package-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
  )
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/package.cmake DESTINATION ${cmake-package-location}
  RENAME ${PROJECT_NAME}-config.cmake)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/package-version.cmake DESTINATION ${cmake-package-location}
  RENAME ${PROJECT_NAME}-config-version.cmake)

# Define the libraries
add_subdirectory (src)

# Doxygen
FIND_PACKAGE(Doxygen)
IF (DOXYGEN_FOUND)
  ADD_CUSTOM_TARGET (apidoc cp Doxyfile ${CMAKE_BINARY_DIR}/Doxyfile-build
    COMMAND echo "OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}" >> ${CMAKE_BINARY_DIR}/Doxyfile-build
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile-build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  ADD_CUSTOM_TARGET (apidoc-stable cp Doxyfile ${CMAKE_BINARY_DIR}/Doxyfile-stable
    COMMAND echo "OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}" >> ${CMAKE_BINARY_DIR}/Doxyfile-stable
    COMMAND echo "HTML_HEADER=header.html" >> ${CMAKE_BINARY_DIR}/Doxyfile-stable
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile-stable
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
ENDIF (DOXYGEN_FOUND)

# Packager
SET(CPACK_PACKAGE_NAME "libbpp-raa3")
SET(CPACK_PACKAGE_VENDOR "Bio++ Development Team")
SET(CPACK_PACKAGE_VERSION "3.0.0")
SET(CPACK_PACKAGE_VERSION_MAJOR "3")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Bio++ Remote Acnuc Access library")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSES/CECILL-2.1.txt")
SET(CPACK_RESOURCE_FILE_AUTHORS "${CMAKE_SOURCE_DIR}/AUTHORS.txt")
SET(CPACK_RESOURCE_FILE_INSTALL "${CMAKE_SOURCE_DIR}/INSTALL.txt")
SET(CPACK_SOURCE_GENERATOR "TGZ")
# /!\ This assumes that an external build is used
SET(CPACK_SOURCE_IGNORE_FILES 
       "/build/" 
       "/html/" 
       "/BppRaa.tag" 
       "/\\\\.git/" 
       "/\\\\.gitignore" 
       ${CPACK_SOURCE_IGNORE_FILES}
       )

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_DEBSOURCE_PACKAGE_FILE_NAME "lib${CMAKE_PROJECT_NAME}_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}.orig")
INCLUDE(CPack)

#This adds the 'dist' target
ADD_CUSTOM_TARGET(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

IF (UNIX)

#This creates rpm packages:
ADD_CUSTOM_TARGET(rpm rpmbuild -ta ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz)
ADD_DEPENDENCIES(rpm dist)

ENDIF()

