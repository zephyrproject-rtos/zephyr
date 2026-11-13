# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_QEMU_X86_64_KVM)
  function(custom_qemu_validator result_var prog)
    execute_process(
      COMMAND ${prog} -display none --enable-kvm -kernel /dev/null
      RESULT_VARIABLE qemu_validator_result
      OUTPUT_VARIABLE qemu_validator_output
      ERROR_VARIABLE qemu_validator_error
      TIMEOUT 1
    )
    if("${qemu_validator_error}" MATCHES "kvm")
      set(${result_var} FALSE PARENT_SCOPE)
      message(STATUS "QEMU binary ${prog} does not support KVM.")
    else()
      set(${result_var} TRUE PARENT_SCOPE)
    endif()
  endfunction()
endif()
