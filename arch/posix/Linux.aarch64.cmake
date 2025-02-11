# SPDX-License-Identifier: Apache-2.0

# For Aarch64, multilib is not an actively pursued solution for most Linux
# distributions. Userspace is (generally) either 32-bit or 64-bit but not
# both.

# @Intent: Call a script to get userspace wordsize for comparison with CONFIG_64BIT
execute_process(
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/user_wordsize.py
  OUTPUT_VARIABLE
  WORDSIZE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (CONFIG_64BIT)
  if (${WORDSIZE} STREQUAL "32")
    message(FATAL_ERROR
      "CONFIG_64BIT=y but this Aarch64 machine has a 32-bit userspace.\n"
      "If you were targeting native_sim/native/64, target native_sim instead.\n"
      "Otherwise, be sure to define CONFIG_64BIT appropriately.\n"
    )
  endif()
  zephyr_compile_options(-fPIC)
else ()
  if (${WORDSIZE} STREQUAL "64")
    message(FATAL_ERROR
      "CONFIG_64BIT=n but this Aarch64 machine has a 64-bit userspace.\n"
      "If you were targeting native_sim, target native_sim/native/64 instead.\n"
      "Otherwise, be sure to define CONFIG_64BIT appropriately.\n"
    )
  endif()
endif ()
