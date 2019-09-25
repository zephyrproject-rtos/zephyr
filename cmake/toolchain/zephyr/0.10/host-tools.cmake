# SPDX-License-Identifier: Apache-2.0

set(HOST_TOOLS_HOME ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/${TOOLCHAIN_ARCH}-pokysdk-linux)

# Path used for searching by the find_*() functions, with appropriate
# suffixes added. Ensures that the SDK's host tools will be found when
# we call, e.g. find_program(QEMU qemu-system-x86)
list(APPEND CMAKE_PREFIX_PATH ${HOST_TOOLS_HOME}/usr)

# TODO: Use find_* somehow for these as well?
set_ifndef(QEMU_BIOS            ${HOST_TOOLS_HOME}/usr/share/qemu)
set_ifndef(OPENOCD_DEFAULT_PATH ${HOST_TOOLS_HOME}/usr/share/openocd/scripts)
