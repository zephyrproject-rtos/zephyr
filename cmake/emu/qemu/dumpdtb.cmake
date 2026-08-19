# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Dump the devicetree of the emulated machine, for boards whose QEMU machine
# generates one, such as the arm and riscv "virt" machines:
#
#   west build -b qemu_riscv64 -t qemu_dumpdtb
#
# Zephyr describes these machines with a hand written devicetree, so being able
# to see what QEMU actually models makes it possible to diff the two. QEMU exits
# as soon as it has written the file, without running the image, which is why
# this target does not depend on the Zephyr binary.
#
# QEMU rejects -machine dumpdtb on a machine that has no devicetree, saying so
# on stderr. The board's own -machine argument is merged with this one, since
# QEMU folds repeated -machine options into a single option group.

add_custom_target(qemu_dumpdtb
  COMMAND
  ${QEMU}
  ${QEMU_BOARD_FLAGS}
  -machine dumpdtb=${APPLICATION_BINARY_DIR}/qemu.dtb
  -display none
  -serial none
  -monitor none
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "[QEMU] Dumping the emulated machine devicetree to qemu.dtb"
  USES_TERMINAL
)
