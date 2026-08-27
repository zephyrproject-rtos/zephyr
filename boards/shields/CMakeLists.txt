# SPDX-License-Identifier: Apache-2.0

# Shield directories may contain multiple shields. Filter out duplicate
# directories to avoid including the same CMakeLists.txt file multiple times
set(unique_shield_dirs ${SHIELD_DIRS})
list(REMOVE_DUPLICATES unique_shield_dirs)

foreach(shield_dir ${unique_shield_dirs})
  # To avoid a lot of empty CMakeLists.txt files we assume it is not an
  # error if it is missing
  if(EXISTS ${shield_dir}/CMakeLists.txt)
    # Out-of-tree shield directories will not be a subdirectory,
    # use the filename portion of the shield directory path as a
    # relative cmake binary_dir
    cmake_path(GET shield_dir FILENAME binary_dir)
    add_subdirectory(${shield_dir} ${binary_dir})
  endif()
endforeach()
