# SPDX-FileCopyrightText: The Bio++ Development Group
#
# SPDX-License-Identifier: CECILL-2.1

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

IF(BUILD_STATIC)
  # Build the static lib
  add_library (${PROJECT_NAME}-static STATIC ${CPP_FILES} ${C_FILES})
  target_include_directories (${PROJECT_NAME}-static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
  set_target_properties (${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  target_link_libraries (${PROJECT_NAME}-static ${BPP_LIBS_STATIC} zlib)
ENDIF()

# Build the shared lib
add_library (${PROJECT_NAME}-shared SHARED ${CPP_FILES} ${C_FILES})
target_include_directories (${PROJECT_NAME}-shared PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
set_target_properties (${PROJECT_NAME}-shared
  PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
  MACOSX_RPATH 1
  VERSION ${${PROJECT_NAME}_VERSION}
  SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR}
  )
target_link_libraries (${PROJECT_NAME}-shared ${BPP_LIBS_SHARED} zlib)

# Install libs and headers
IF(BUILD_STATIC)
install (
  TARGETS ${PROJECT_NAME}-static
  EXPORT ${PROJECT_NAME}-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
ENDIF()

install (
  TARGETS ${PROJECT_NAME}-shared
  EXPORT ${PROJECT_NAME}-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
install (
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
  )
# Install cmake file representing targets
install (EXPORT ${PROJECT_NAME}-targets DESTINATION ${cmake-package-location})
