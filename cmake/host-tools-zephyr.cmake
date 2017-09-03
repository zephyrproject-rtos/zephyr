# Lots of duplications here.
# FIXME: maintain this only in one place.

# We need to separate actual toolchain from the host-tools required by Zephyr
# and currently provided by the Zephyr SDK. Those tools will need to be
# provided for different OSes and sepearately from the toolchain.


if(NOT ZEPHYR_SDK_INSTALL_DIR)
  set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
endif()
set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")
if(NOT ZEPHYR_SDK_INSTALL_DIR)
  message(FATAL_ERROR "ZEPHYR_SDK_INSTALL_DIR is not set")
endif()

set(REQUIRED_SDK_VER 0.9.1)
set(TOOLCHAIN_VENDOR zephyr)
set(TOOLCHAIN_ARCH x86_64)

file(READ ${ZEPHYR_SDK_INSTALL_DIR}/sdk_version SDK_VERSION)
if(${REQUIRED_SDK_VER} VERSION_GREATER ${SDK_VERSION})
  message(FATAL_ERROR "The SDK version you are using is old, please update your SDK.
You need at least SDK version ${REQUIRED_SDK_VER}.
The new version of the SDK can be download from:
https://github.com/zephyrproject-rtos/meta-zephyr-sdk/releases/download/${REQUIRED_SDK_VER}/zephyr-sdk-${REQUIRED_SDK_VER}-setup.run
")
endif()

if(MINGW)
  set(TOOLCHAIN_HOME ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/i686-pokysdk-mingw32)
else()
  set(TOOLCHAIN_HOME ${ZEPHYR_SDK_INSTALL_DIR}/sysroots/${TOOLCHAIN_ARCH}-pokysdk-linux)
endif()

set(DTC       		${TOOLCHAIN_HOME}/usr/bin/dtc)
if(${SDK_VERSION} VERSION_GREATER "0.9.1")
  set(KCONFIG_CONF       	${TOOLCHAIN_HOME}/usr/bin/conf)
  set(KCONFIG_MCONF      	${TOOLCHAIN_HOME}/usr/bin/mconf)
endif()
set(QEMU_BIOS 		${TOOLCHAIN_HOME}/usr/share/qemu)

if("${ARCH}" STREQUAL "x86")
  set(QEMU ${TOOLCHAIN_HOME}/usr/bin/qemu-system-i386)
else()
  set(QEMU ${TOOLCHAIN_HOME}/usr/bin/qemu-system-${ARCH})
endif()
