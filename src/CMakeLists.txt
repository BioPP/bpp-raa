# CMake script for Bio++ Remote Acnuc Access
# Authors:
#   Sylvain Gaillard
#   Julien Dutheil
#   Francois Gindraud (2017)
# Created: 11/09/2009

set (C_FILES
  Bpp/Raa/RAA_acnuc.c
  Bpp/Raa/md5.c
  Bpp/Raa/misc_acnuc.c
  Bpp/Raa/parser.c
  Bpp/Raa/zsockr.c
  )

set (CPP_FILES
  Bpp/Raa/RAA.cpp
  Bpp/Raa/RaaList.cpp
  Bpp/Raa/RaaSpeciesTree.cpp
  )

# Here we have both C and CPP files, so -std=c++11 must not be applied to C_FILES.
# Cpp specific options (-std=c++11) are placed in *-cxx-compile-options.
# *-cxx-compile-options are applied to source files (and not whole target), and also to interface.
# -Wall in private-compile-options is valid in C and CPP, so it is placed at target level.
set_source_files_properties (${CPP_FILES} PROPERTIES
  COMPILE_FLAGS ${private-cxx-compile-options}
  )

# Build the static lib
add_library (${PROJECT_NAME}-static STATIC ${CPP_FILES} ${C_FILES})
target_include_directories (${PROJECT_NAME}-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )
set_target_properties (${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries (${PROJECT_NAME}-static ${BPP_LIBS_STATIC} zlib)
target_compile_options (${PROJECT_NAME}-static PRIVATE ${private-compile-options})

# Build the shared lib
add_library (${PROJECT_NAME}-shared SHARED ${CPP_FILES} ${C_FILES})
target_include_directories (${PROJECT_NAME}-shared PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )
set_target_properties (${PROJECT_NAME}-shared
  PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
  MACOSX_RPATH 1
  VERSION ${${PROJECT_NAME}_VERSION}
  SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR}
  )
target_link_libraries (${PROJECT_NAME}-shared ${BPP_LIBS_SHARED} zlib)
target_compile_options (${PROJECT_NAME}-shared PRIVATE ${private-compile-options})

# Install libs and headers
install (
  TARGETS ${PROJECT_NAME}-static ${PROJECT_NAME}-shared
  EXPORT ${PROJECT_NAME}-targets
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  ARCHIVE DESTINATION lib${LIB_SUFFIX}
  )
install (
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bpp
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
  )
# Install cmake file representing targets
install (EXPORT ${PROJECT_NAME}-targets DESTINATION ${cmake-package-location})
