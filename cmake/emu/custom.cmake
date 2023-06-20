# SPDX-License-Identifier: Apache-2.0
#
# ${ZEPHYR_BASE}/CMakeLists.txt looks for ${ZEPHYR_BASE}/cmake/emu/${EMU_PLATFORM}.cmake
# when building the `run` target. Create this placeholder file so it doesn't complain.
# The real 'run' custom_target should be defined in `board.cmake` instead.
#
# See https://docs.zephyrproject.org/latest/develop/test/twister.html#running-tests-on-custom-emulator
#
