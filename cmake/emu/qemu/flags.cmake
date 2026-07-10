# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Accumulators for the QEMU command line and the run targets that use it.
#
# Contributors call the functions below instead of appending to specifically
# named variables. State is kept in global properties so that it can be
# contributed from any directory or function scope, not just from a file that
# happens to be include()d into cmake/emu/qemu.cmake.
#
# Flags land in one of two ordered slots, concatenated in this order:
#
#   DEVICE  emulated devices and host plumbing      qemu_append_flags()
#   EXTRA   late additions, after every device      qemu_append_extra_flags()
#
# Within a slot, flags keep the order in which they were contributed. That
# order is significant: QEMU numbers -serial ports in the order they appear, so
# a fragment adding a second UART must be included after the one adding the
# console.

# Append <flags> to the DEVICE slot.
function(qemu_append_flags)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_FLAGS_DEVICE ${ARGV})
endfunction()

# Append <flags> to the EXTRA slot.
function(qemu_append_extra_flags)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_FLAGS_EXTRA ${ARGV})
endfunction()

# Append <flags> to the command line of a single run <target>.
function(qemu_append_target_flags target)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_TARGET_FLAGS_${target} ${ARGN})
endfunction()

# Register additional run targets, beyond run_qemu and debugserver_qemu.
function(qemu_add_run_targets)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_RUN_TARGETS ${ARGV})
endfunction()

# Read back the registered run targets.
function(qemu_get_run_targets out_var)
  get_property(targets GLOBAL PROPERTY ZEPHYR_QEMU_RUN_TARGETS)
  set(${out_var} "${targets}" PARENT_SCOPE)
endfunction()

# Run <commands> before QEMU starts, for a single run <target>. <commands> is a
# sequence of COMMAND ... groups, as accepted by add_custom_target().
function(qemu_add_pre_commands target)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_PRE_COMMANDS_${target} ${ARGN})
endfunction()

# Run <commands> after QEMU exits, for a single run <target>.
function(qemu_add_post_commands target)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_POST_COMMANDS_${target} ${ARGN})
endfunction()

# Make every run target depend on <targets> at build time, so that whatever they
# generate exists before QEMU is started.
function(qemu_add_target_depends)
  set_property(GLOBAL APPEND PROPERTY ZEPHYR_QEMU_TARGET_DEPENDS ${ARGV})
endfunction()

# Read back one of the accumulators. <slot> is FLAGS_DEVICE, FLAGS_EXTRA,
# TARGET_DEPENDS, or one of TARGET_FLAGS_<t>, PRE_COMMANDS_<t>,
# POST_COMMANDS_<t>.
function(qemu_get_property out_var slot)
  get_property(value GLOBAL PROPERTY ZEPHYR_QEMU_${slot})
  set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

qemu_add_run_targets(run_qemu debugserver_qemu)
