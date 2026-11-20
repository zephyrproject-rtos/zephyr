.. _snippet-qemu-uefi:

QEMU UEFI Snippet (qemu-uefi)
#############################

.. code-block:: console

   west build -b qemu_cortex_a53 -S qemu-uefi [...]

Overview
********

This snippet builds the application as an EFI application and runs it
under the EDK2 UEFI firmware in QEMU. It sets
:kconfig:option:`CONFIG_BUILD_OUTPUT_EFI` and
:kconfig:option:`CONFIG_QEMU_UEFI_BOOT`, along with what EDK2 needs:

- QEMU without a secure world (:kconfig:option:`CONFIG_ARMV8_A_NS`).
- 512 MiB of RAM, so EDK2 has room above Zephyr's RAM.
- Zephyr's RAM at 0x48000000, away from the devicetree QEMU places at
  0x40000000 and from what EDK2 keeps below 0x48000000.

Requirements
************

The ``qemu_cortex_a53`` board, and the EDK2 firmware for the QEMU ``virt``
machine, as described in the board's documentation.
