# SPDX-License-Identifier: Apache-2.0

foreach(shield_dir ${SHIELD_DIRS})
  # To avoid a lot of empty CMakeLists.txt files we assume it is not an
  # error if it is missing
  if(EXISTS ${shield_dir}/CMakeLists.txt)
    # Out-of-tree shield directories will not be a subdirectory,
    # use the shield name as a relative cmake binary_dir
    cmake_path(GET shield_dir FILENAME shield)
    add_subdirectory(${shield_dir} ${shield})
  endif()
endforeach()
