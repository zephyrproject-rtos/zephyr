# FindLibmetal
# --------
#
# Find Libmetal
#
# Find the native Libmetal includes and library this module defines
#
# ::
#
#   LIBMETAL_INCLUDE_DIR, where to find metal/sysfs.h, etc.
#   LIBSYSFS_LIB_DIR, where to find libmetal library.

# FIX ME, CMAKE_FIND_ROOT_PATH doesn't work
# even use the following
# set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
# set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
# set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
find_path(LIBMETAL_INCLUDE_DIR NAMES metal/sys.h PATHS ${CMAKE_FIND_ROOT_PATH})
find_library(LIBMETAL_LIB NAMES metal PATHS ${CMAKE_FIND_ROOT_PATH})
get_filename_component(LIBMETAL_LIB_DIR ${LIBMETAL_LIB} DIRECTORY)

# handle the QUIETLY and REQUIRED arguments and set HUGETLBFS_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (LIBMETAL DEFAULT_MSG LIBMETAL_LIB LIBMETAL_INCLUDE_DIR)

if (LIBMETAL_FOUND)
  set (LIBMETAL_LIBS ${LIBMETAL_LIB})
endif (LIBMETAL_FOUND)

mark_as_advanced (LIBMETAL_LIB LIBMETAL_INCLUDE_DIR LIBMETAL_LIB_DIR)
