
set_property (GLOBAL PROPERTY "PROJECT_LIB_EXTRA_CFLAGS")

collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collect (PROJECT_LIB_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
collect (PROJECT_INC_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")


add_subdirectory (common)
add_subdirectory (virtio)
add_subdirectory (rpmsg)
add_subdirectory (remoteproc)

if (WITH_PROXY)
  add_subdirectory (proxy)
endif (WITH_PROXY)

set (OPENAMP_LIB open_amp)

if (NOT CMAKE_INSTALL_LIBDIR)
	set (CMAKE_INSTALL_LIBDIR "lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

collector_list (_include PROJECT_INC_DIRS)
include_directories (${_include})

collector_list (_deps PROJECT_LIB_DEPS)

get_property (_ecflags GLOBAL PROPERTY "PROJECT_LIB_EXTRA_CFLAGS")

collector_list (_sources PROJECT_LIB_SOURCES)
set_property (SOURCE ${_sources}
  APPEND_STRING PROPERTY COMPILE_FLAGS " ${_ecflags}")

# Build a shared library if so configured.
if (WITH_ZEPHYR)
  zephyr_library_named(${OPENAMP_LIB})
  add_dependencies(${OPENAMP_LIB} offsets_h)
  target_sources (${OPENAMP_LIB} PRIVATE ${_sources})
  zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
else (WITH_ZEPHYR)
  if (WITH_SHARED_LIB)
    set (_lib ${OPENAMP_LIB}-shared)
    add_library (${_lib} SHARED ${_sources})
    target_link_libraries (${_lib} ${_deps})
    install (TARGETS ${_lib} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    set_target_properties (${_lib} PROPERTIES
      OUTPUT_NAME       "${OPENAMP_LIB}"
      VERSION           "${PROJECT_VER}"
      SOVERSION         "${PROJECT_VER_MAJOR}"
    )
  endif (WITH_SHARED_LIB)

  if (WITH_STATIC_LIB)
    set (_lib ${OPENAMP_LIB}-static)
    add_library (${_lib} STATIC ${_sources})
    install (TARGETS ${_lib} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    set_target_properties (${_lib} PROPERTIES
      OUTPUT_NAME       "${OPENAMP_LIB}"
    )
  endif (WITH_STATIC_LIB)
endif (WITH_ZEPHYR)

install (DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/openamp" DESTINATION include)

# vim: expandtab:ts=2:sw=2:smartindent
