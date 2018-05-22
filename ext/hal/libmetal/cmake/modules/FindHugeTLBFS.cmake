# FindHugeTLBFS
# --------
#
# Find HugeTLBFS
#
# Find the native HugeTLBFS includes and library This module defines
#
# ::
#
#   HUGETLBFS_INCLUDE_DIR, where to find hugetlbfs.h, etc.
#   HUGETLBFS_LIBRARIES, the libraries needed to use HugeTLBFS.
#   HUGETLBFS_FOUND, If false, do not try to use HugeTLBFS.
#
# also defined, but not for general use are
#
# ::
#
#   HUGETLBFS_LIBRARY, where to find the HugeTLBFS library.

find_path (HUGETLBFS_INCLUDE_DIR hugetlbfs.h)

set (HUGETLBFS_NAMES ${HUGETLBFS_NAMES} hugetlbfs)
find_library (HUGETLBFS_LIBRARY NAMES ${HUGETLBFS_NAMES})

# handle the QUIETLY and REQUIRED arguments and set HUGETLBFS_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (HUGETLBFS DEFAULT_MSG HUGETLBFS_LIBRARY HUGETLBFS_INCLUDE_DIR)

if (HUGETLBFS_FOUND)
  set (HUGETLBFS_LIBRARIES ${HUGETLBFS_LIBRARY})
endif (HUGETLBFS_FOUND)

mark_as_advanced (HUGETLBFS_LIBRARY HUGETLBFS_INCLUDE_DIR)
