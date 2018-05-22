collector_create (PROJECT_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}")
collector_create (PROJECT_LIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}")

collect (PROJECT_LIB_DIRS "${CMAKE_CURRENT_BINARY_DIR}")
collect (PROJECT_INC_DIRS "${CMAKE_CURRENT_BINARY_DIR}/include")

collect (PROJECT_LIB_HEADERS alloc.h)
collect (PROJECT_LIB_HEADERS assert.h)
collect (PROJECT_LIB_HEADERS atomic.h)
collect (PROJECT_LIB_HEADERS cache.h)
collect (PROJECT_LIB_HEADERS compiler.h)
collect (PROJECT_LIB_HEADERS condition.h)
collect (PROJECT_LIB_HEADERS config.h)
collect (PROJECT_LIB_HEADERS cpu.h)
collect (PROJECT_LIB_HEADERS device.h)
collect (PROJECT_LIB_HEADERS dma.h)
collect (PROJECT_LIB_HEADERS io.h)
collect (PROJECT_LIB_HEADERS irq.h)
collect (PROJECT_LIB_HEADERS list.h)
collect (PROJECT_LIB_HEADERS log.h)
collect (PROJECT_LIB_HEADERS mutex.h)
collect (PROJECT_LIB_HEADERS shmem.h)
collect (PROJECT_LIB_HEADERS sleep.h)
collect (PROJECT_LIB_HEADERS spinlock.h)
collect (PROJECT_LIB_HEADERS sys.h)
collect (PROJECT_LIB_HEADERS time.h)
collect (PROJECT_LIB_HEADERS utilities.h)
collect (PROJECT_LIB_HEADERS version.h)

collect (PROJECT_LIB_SOURCES dma.c)
collect (PROJECT_LIB_SOURCES device.c)
collect (PROJECT_LIB_SOURCES init.c)
collect (PROJECT_LIB_SOURCES io.c)
collect (PROJECT_LIB_SOURCES log.c)
collect (PROJECT_LIB_SOURCES shmem.c)
collect (PROJECT_LIB_SOURCES version.c)

add_subdirectory (compiler)
add_subdirectory (processor)
add_subdirectory (system)

collector_list (_inc_dirs PROJECT_INC_DIRS)
collector_list (_sources PROJECT_LIB_SOURCES)
collector_list (_headers PROJECT_LIB_HEADERS)
collector_list (_deps PROJECT_LIB_DEPS)

foreach (f ${_headers})
  configure_file (${f} include/${PROJECT_NAME}/${f} @ONLY)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${f}
           DESTINATION include RENAME ${PROJECT_NAME}/${f})
  if (${f} MATCHES "^[^/]*\\.h")
    collect (PROJECT_HDR_TESTS "metal/${f}")
  endif (${f} MATCHES "^[^/]*\\.h")
endforeach (f)

include_directories (${_inc_dirs})
add_definitions (-DMETAL_INTERNAL)

if (WITH_DEFAULT_LOGGER)
  add_definitions (-DDEFAULT_LOGGER_ON)
endif (WITH_DEFAULT_LOGGER)

if (WITH_ZEPHYR)
  zephyr_library_named(metal)
  add_dependencies(metal offsets_h)
  target_sources (metal PRIVATE ${_sources})
  zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
else (WITH_ZEPHYR)
  # Build a shared library if so configured.
  if (WITH_SHARED_LIB)
    set (_lib ${PROJECT_NAME}-shared)
    add_library (${_lib} SHARED ${_sources})
    target_link_libraries (${_lib} ${_deps})
    install (TARGETS ${_lib} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if (PROJECT_EC_FLAGS)
      string(REPLACE " " ";" _ec_flgs ${PROJECT_EC_FLAGS})
      target_compile_options (${_lib} PUBLIC ${_ec_flgs})
    endif (PROJECT_EC_FLAGS)
    set_target_properties (${_lib} PROPERTIES
      OUTPUT_NAME       "${PROJECT_NAME}"
      VERSION           "${PROJECT_VER}"
      SOVERSION         "${PROJECT_VER_MAJOR}"
    )
  endif (WITH_SHARED_LIB)

  # Build a static library if so configured.
  if (WITH_STATIC_LIB)
    set (_lib ${PROJECT_NAME}-static)
    add_library (${_lib} STATIC ${_sources})
    install (TARGETS ${_lib} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    if (PROJECT_EC_FLAGS)
      string(REPLACE " " ";" _ec_flgs ${PROJECT_EC_FLAGS})
      target_compile_options (${_lib} PUBLIC ${_ec_flgs})
    endif (PROJECT_EC_FLAGS)
    set_target_properties (${_lib} PROPERTIES
      OUTPUT_NAME       "${PROJECT_NAME}"
    )
  endif (WITH_STATIC_LIB)
endif (WITH_ZEPHYR)

# vim: expandtab:ts=2:sw=2:smartindent
