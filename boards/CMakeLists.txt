# To avoid a lot of empty CMakeLists.txt files we assume it is not an
# error if it is missing

if(EXISTS ${BOARD_DIR}/CMakeLists.txt)
  # Create board library for sub-directory CMakeLists.txt to populate
  add_library(board STATIC "")
  include(${BOARD_DIR}/CMakeLists.txt)

  get_property(sources TARGET board PROPERTY SOURCES)
  if(NOT sources)
    # board was not populated, exclude it from build.
    set_property(TARGET board PROPERTY EXCLUDE_FROM_ALL True)
    # To avoid CMake error on empth lib even though it is not built a single file must bne added.
    target_sources(board PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)
  else()
    target_link_libraries(board PRIVATE zephyr_interface)
    target_link_libraries(zephyr_app_linking INTERFACE board)
  endif()
endif()
