# CMake script for Bio++ Remote Acnuc Access
# Author: Sylvain Gaillard and Julien Dutheil
# Created: 11/09/2009

# File list
set(C_FILES
 md5.c
 parser.c
 zsockr.c
 misc_acnuc.c
 RAA_acnuc.c
)

set(CPP_FILES
 RAA.cpp
 RaaList.cpp
 RaaSpeciesTree.cpp
)

set(H_FILES
 parser.h
 RAA.h
 RaaSeqAttributes.h
 RAA_acnuc.h
 RaaList.h
 RaaSpeciesTree.h
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
install(TARGETS bppraa-static bppraa-shared DESTINATION lib)

# Install headers
install(FILES ${H_FILES} DESTINATION include/Bpp/Raa)
