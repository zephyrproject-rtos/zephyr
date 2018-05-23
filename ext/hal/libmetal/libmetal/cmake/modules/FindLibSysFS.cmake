# FindLibSysFS
# --------
#
# Find LibSysFS
#
# Find the native LibSysFS includes and library This module defines
#
# ::
#
#   LIBSYSFS_INCLUDE_DIR, where to find libsysfs.h, etc.
#   LIBSYSFS_LIBRARIES, the libraries needed to use LibSysFS.
#   LIBSYSFS_FOUND, If false, do not try to use LibSysFS.
#
# also defined, but not for general use are
#
# ::
#
#   LIBSYSFS_LIBRARY, where to find the LibSysFS library.

find_path (LIBSYSFS_INCLUDE_DIR sysfs/libsysfs.h)

set (LIBSYSFS_NAMES ${LIBSYSFS_NAMES} sysfs)
find_library (LIBSYSFS_LIBRARY NAMES ${LIBSYSFS_NAMES})

# handle the QUIETLY and REQUIRED arguments and set LIBSYSFS_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (LIBSYSFS DEFAULT_MSG LIBSYSFS_LIBRARY LIBSYSFS_INCLUDE_DIR)

if (LIBSYSFS_FOUND)
  set (LIBSYSFS_LIBRARIES ${LIBSYSFS_LIBRARY})
endif (LIBSYSFS_FOUND)

mark_as_advanced (LIBSYSFS_LIBRARY LIBSYSFS_INCLUDE_DIR)
