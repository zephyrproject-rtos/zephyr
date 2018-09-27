# To avoid a lot of empty CMakeLists.txt files we assume it is not an
# error if it is missing

if(EXISTS ${BOARD_DIR}/CMakeLists.txt)
  if(BOARD_ROOT)
	set(build_dir boards/${ARCH}/${BOARD})
  else()
	unset(build_dir)
  endif()

  add_subdirectory(${BOARD_DIR} ${build_dir})
endif()
