# CMake script for Bio++ Remote Acnuc Access
# Author: Sylvain Gaillard and Julien Dutheil
# Created: 11/09/2009

# Global parameters
cmake_minimum_required(VERSION 2.6)
project(bpp-raa C CXX)
if(NOT DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif(NOT DEFINED CMAKE_BUILD_TYPE)
set(CMAKE_CXX_FLAGS "-Wall")

# Libtool-like version number
# CURRENT:REVISION:AGE => file.so.(C-A).A.R
# current:  The most recent interface number that this library implements.
# revision: The implementation number of the current interface.
# age:      The difference between the newest and oldest interfaces that this
#           library implements.
# In other words, the library implements all the interface numbers in the
# range from number current - age to current.
set(BPPRAA_VERSION_CURRENT "1")
set(BPPRAA_VERSION_REVISION "0")
set(BPPRAA_VERSION_AGE "0")

# Effective version number computation
math(EXPR BPPRAA_VERSION_MAJOR "${BPPRAA_VERSION_CURRENT} -
${BPPRAA_VERSION_AGE}")
set(BPPRAA_VERSION_MINOR ${BPPRAA_VERSION_AGE})
set(BPPRAA_VERSION_PATCH ${BPPRAA_VERSION_REVISION})
set(BPPRAA_VERSION "${BPPRAA_VERSION_MAJOR}.${BPPRAA_VERSION_MINOR}.${BPPRAA_VERSION_PATCH}")

# Set the CMAKE_PREFIX_PATH for the find_library fonction when using non
# standard install location
IF(CMAKE_INSTALL_PREFIX)
  SET(CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}" ${CMAKE_PREFIX_PATH})
ENDIF(CMAKE_INSTALL_PREFIX)

#here is a useful function:
MACRO(IMPROVED_FIND_LIBRARY OUTPUT_LIBS lib_name include_to_find)
  #start:
  FIND_PATH(${lib_name}_INCLUDE_DIR ${include_to_find})
  SET(${lib_name}_NAMES ${lib_name} ${lib_name}lib ${lib_name}dll)
  FIND_LIBRARY(${lib_name}_LIBRARY NAMES ${${lib_name}_NAMES})

  IF(${lib_name}_LIBRARY)
    MESSAGE("-- Library ${lib_name} found here:")
    MESSAGE("   includes : ${${lib_name}_INCLUDE_DIR}")
    MESSAGE("   libraries: ${${lib_name}_LIBRARY}")
  ELSE(${lib_name}_LIBRARY)
    MESSAGE(FATAL_ERROR "${lib_name} required but not found.")
  ENDIF(${lib_name}_LIBRARY)
  
  #add the dependency:
  INCLUDE_DIRECTORIES(${${lib_name}_INCLUDE_DIR})
  SET(${OUTPUT_LIBS} ${${OUTPUT_LIBS}} ${${lib_name}_LIBRARY})
ENDMACRO(IMPROVED_FIND_LIBRARY)

#Find the Bio++ libraries:
IMPROVED_FIND_LIBRARY(LIBS bpp-seq Bpp/Seq/Alphabet/Alphabet.h)
IMPROVED_FIND_LIBRARY(LIBS bpp-core Bpp/Clonable.h)

# Find the zlib installation
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

# Subdirectories
add_subdirectory(src)

# Doxygen
find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_custom_target (apidoc ${DOXYGEN_EXECUTABLE} Doxyfile)
  ADD_CUSTOM_TARGET (apidoc-stable cp Doxyfile Doxyfile-stable
    COMMAND echo "HTML_HEADER=header.html" >> Doxyfile-stable
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile-stable )
endif (DOXYGEN_FOUND)

# Packager
set(CPACK_PACKAGE_NAME "libbpp-raa")
set(CPACK_PACKAGE_VENDOR "Bio++ Development Team")
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The Bio++ Remote Acnuc Access library")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING.txt")
set(CPACK_RESOURCE_FILE_AUTHORS "${CMAKE_SOURCE_DIR}/AUTHORS.txt")
set(CPACK_RESOURCE_FILE_INSTALL "${CMAKE_SOURCE_DIR}/INSTALL.txt")
set(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES
 "CMakeFiles"
 "Makefile"
 "_CPack_Packages"
 "CMakeCache.txt"
 ".*\\\\.cmake"
 ".*\\\\.git"
 ".*\\\\.gz"
 ".*\\\\.deb"
 ".*\\\\.rpm"
 "\\\\..*\\\\.swp"
 "src/\\\\..*"
 "src/libbpp*"
 "debian/libbpp.*/"
 "debian/libbpp.*\\\\.so.*"
 "debian/libbpp.*\\\\.a"
 "debian/libbpp.*\\\\.substvars"
 "debian/libbpp.*\\\\.debhelper"
 "debian/debhelper\\\\.log"
 "debian/tmp"
 "html"
 "Raa.tag"
 "Testing"
 "build-stamp"
 "genIncludes.py"
 ${CPACK_SOURCE_IGNORE_FILES}
)
IF (MACOS)
  SET(CPACK_GENERATOR "Bundle")
ENDIF()
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
include(CPack)

#This adds the 'dist' target
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
IF (UNIX)
#This creates deb packages:
ADD_CUSTOM_TARGET(deb dpkg-buildpackage)
ADD_DEPENDENCIES(deb dist)

#This creates rpm packages:
ADD_CUSTOM_TARGET(rpm rpmbuild -ta ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz)
ADD_DEPENDENCIES(rpm dist)

ENDIF()

