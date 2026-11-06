# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include(${CMAKE_CURRENT_LIST_DIR}/x86_kvm_validator.cmake)

function(qemu_validator result_var prog)
  if(COMMAND custom_qemu_validator)
    set(result_var_local FALSE)
    custom_qemu_validator(result_var_local ${prog})
    set(${result_var} ${result_var_local} PARENT_SCOPE)
  else()
    set(${result_var} TRUE PARENT_SCOPE)
  endif()
endfunction()
