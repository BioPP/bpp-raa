# CMake script for Bio++ Remote Acnuc Access
# Author: Sylvain Gaillard and Julien Dutheil
# Created: 11/09/2009

# File list
set(C_FILES
 Bpp/Raa/md5.c
 Bpp/Raa/parser.c
 Bpp/Raa/zsockr.c
 Bpp/Raa/misc_acnuc.c
 Bpp/Raa/RAA_acnuc.c
)

set(CPP_FILES
 Bpp/Raa/RAA.cpp
 Bpp/Raa/RaaList.cpp
 Bpp/Raa/RaaSpeciesTree.cpp
)

set(H_FILES
 Bpp/Raa/parser.h
 Bpp/Raa/RAA.h
 Bpp/Raa/RaaSeqAttributes.h
 Bpp/Raa/RAA_acnuc.h
 Bpp/Raa/RaaList.h
 Bpp/Raa/RaaSpeciesTree.h
)

# Build the static lib
add_library(bppraa-static STATIC ${CPP_FILES} ${C_FILES})
set_target_properties(bppraa-static
  PROPERTIES OUTPUT_NAME bpp-raa
  CLEAN_DIRECT_OUTPUT 1
  )
target_link_libraries(bppraa-static ${LIBS})

# Build the shared lib
add_library(bppraa-shared SHARED ${CPP_FILES} ${C_FILES})
set_target_properties(bppraa-shared
  PROPERTIES OUTPUT_NAME bpp-raa
  CLEAN_DIRECT_OUTPUT 1
  VERSION ${BPPRAA_VERSION}
  SOVERSION ${BPPRAA_VERSION_MAJOR}
  )
target_link_libraries(bppraa-shared ${LIBS})

# Install libs
install(TARGETS bppraa-static bppraa-shared DESTINATION lib${LIB_SUFFIX})

# Install headers
install(DIRECTORY Bpp/ DESTINATION include/Bpp FILES_MATCHING PATTERN "*.h" PATTERN "*.all")
