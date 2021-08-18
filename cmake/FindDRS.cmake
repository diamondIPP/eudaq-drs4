# - Try to find DRS used in native reader library for exporting RAW to DRS format needed for reconstruction
# Once done this will define
#  DRS_FOUND - System has DRS
#  DRS_INCLUDE_DIRS - The DRS include directories
#  DRS_LIBRARIES - The libraries needed to use DRS
#  DRS_DEFINITIONS - Compiler switches required for using DRS

IF (NOT DEFINED ${DRS_PATH})
   SET(DRS_PATH $ENV{DRS_PATH})
ENDIF()
MESSAGE(STATUS "Looking for DRS Board dependencies: DRS... DRS_PATH: " ${DRS_PATH})

find_path(DRS_INCLUDE_DIR DRS.h HINTS "${DRS_PATH}/include")
find_path(DRS_SRC_DIR DRS.cpp HINTS "${DRS_PATH}/src")
IF (NOT DRS_INCLUDE_DIR OR NOT DRS_SRC_DIR)
    MESSAGE(SEND_ERROR "Cannot find DRS_SRC_DIR...")
ENDIF()

find_library(DRS_LIBRARY NAMES DRS HINTS "${DRS_PATH}")
IF (NOT DRS_LIBRARY)
    MESSAGE(STATUS "\tYou need to build a shared object: "
    "	"
    "$(SHARED_OBJECTS): %.so:  DRS.o mxml.o averager.o musbstd.o"
	" $(CXX) $(CFLAGS) $(WXFLAGS) $(WXLIBS) $(LIBS)"
	" -Wall -shared -fPIC -o $@ -I include/ src/DRS.cpp "
	"src/averager.cpp src/mxml.c  src/musbstd.c -I include/")
    MESSAGE(SEND_ERROR "\tCannot find DRS_LIBRARY Dir: " ${DRS_LIBRARY})
ENDIF ()

set(DRS_LIBRARIES ${DRS_LIBRARY} )
set(DRS_INCLUDE_DIRS ${DRS_INCLUDE_DIR})

#include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set DRS_FOUND to TRUE
# if all listed variables are TRUE
#find_package_handle_standard_args(DRS
#  REQUIRED_VARS DRS_LIBRARY DRS_API_INCLUDE_DIR DRS_UTILS_INCLUDE_DIR PXAR_UTIL_INCLUDE_DIR)

IF(DRS_LIBRARY AND DRS_INCLUDE_DIR)
   SET(DRS_FOUND TRUE)
   MESSAGE(STATUS "Found DRS library and headers.")
ENDIF () 
#(DRS_LIBRARY AND DRS_INCLUDE_DIR)

mark_as_advanced(DRS_LIBRARY DRS_INCLUDE_DIR)