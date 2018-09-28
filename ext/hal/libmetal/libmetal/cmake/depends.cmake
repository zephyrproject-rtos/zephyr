if (WITH_DOC)
  find_package (Doxygen)
endif (WITH_DOC)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

  check_include_files (stdatomic.h HAVE_STDATOMIC_H)
  check_include_files (linux/futex.h HAVE_FUTEX_H)

  find_package (HugeTLBFS)
  if (HUGETLBFS_FOUND)
    collect (PROJECT_INC_DIRS "${HUGETLBFS_INCLUDE_DIR}")
    collect (PROJECT_LIB_DEPS "${HUGETLBFS_LIBRARIES}")
    add_definitions(-DHAVE_HUGETLBFS_H)
  endif(HUGETLBFS_FOUND)

  find_package (LibSysFS REQUIRED)
  collect (PROJECT_INC_DIRS "${LIBSYSFS_INCLUDE_DIR}")
  collect (PROJECT_LIB_DEPS "${LIBSYSFS_LIBRARIES}")

  find_package(Threads REQUIRED)
  collect (PROJECT_LIB_DEPS "${CMAKE_THREAD_LIBS_INIT}")

  find_package(LibRt REQUIRED)
  collect (PROJECT_LIB_DEPS "${LIBRT_LIBRARIES}")

else ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

  # TODO: fix for find_path() to detect stdatomic.h
  # find_path (HAVE_STDATOMIC_H stdatomic.h)
  set (_saved_cmake_required_flags ${CMAKE_REQUIRED_FLAGS})
  set (CMAKE_REQUIRED_FLAGS "-c" CACHE STRING "")
  check_include_files (stdatomic.h HAVE_STDATOMIC_H)
  set (CMAKE_REQUIRED_FLAGS ${_saved_cmake_required_flags})

endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

# vim: expandtab:ts=2:sw=2:smartindent
