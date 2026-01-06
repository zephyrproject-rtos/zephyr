# SPDX-License-Identifier: Apache-2.0

#set(SUPPORTED_EMU_PLATFORMS qemu)
set(SUPPORTED_EMU_PLATFORMS custom)

set(QEMU_binary_suffix riscv64)
set(QEMU_CPU_TYPE_${ARCH} riscv64)

list(APPEND CUSTOM_QEMU_FLAGS_APPEND "")
list(APPEND CUSTOM_QEMU_STDERR_REDIRECT "")

if(CONFIG_SMP)
    list(APPEND CUSTOM_QEMU_FLAGS_APPEND
            -smp 4)
endif()

if(CONFIG_SEMIHOST)
    # -semihosting-config enable=on,target=auto,chardev=con
    # versin not support
    #
    list(APPEND CUSTOM_QEMU_FLAGS_APPEND
            -semihosting)
    list(APPEND CUSTOM_QEMU_STDERR_REDIRECT
            2>&1)
endif()

set(QEMU_FLAGS_${ARCH}
        -nographic
        -machine xiaohui
        -cpu c908v
        -m 256
)

list(APPEND QEMU_FLAGS_${ARCH} ${CUSTOM_QEMU_FLAGS_APPEND})

add_custom_target(run_custom
        COMMAND
        /rvhome/o_ld02443552/workspace/zephyrproject/xuantie_qemu/bin/qemu-system-riscv64
        ${QEMU_FLAGS_${ARCH}}
        -kernel ${APPLICATION_BINARY_DIR}/zephyr/zephyr.elf
        ${CUSTOM_QEMU_STDERR_REDIRECT}
        WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
        DEPENDS ${logical_target_for_zephyr_elf}
        USES_TERMINAL
)

board_set_debugger_ifnset(qemu)
