.. zephyr:code-sample:: efi_gop
   :name: LVGL on EFI GOP display
   :relevant-api: display_interface

   Run LVGL on the EFI Graphics Output Protocol (GOP) framebuffer when
   booting x86_64 via UEFI (e.g. QEMU with OVMF).

Overview
********

This sample demonstrates:

* **EFI GOP display**: The Zephyr EFI boot stub (zefi) obtains the GOP
  from the UEFI environment and saves framebuffer info for the
  :ref:`display_efi_gop` driver. No external display hardware is needed
  beyond what the firmware provides.
* **LVGL**: A simple UI with a status bar (title, resolution, uptime) and
  three progress bars, built with LVGL widgets (label, bar, flex layout).
* **Multi-threaded**: Worker threads push progress updates via a message
  queue; the LVGL thread updates the bars and runs the timer handler.
  Layout adapts to any GOP resolution via flex.

The UI shows "Zephyr + LVGL on GOP (WxH)" in the top bar and three
progress bars labeled thread_a, thread_b, thread_c, driven by worker
threads. Uptime is shown on the right of the status bar.

Requirements
************

* **Board**: :zephyr:board:`qemu_x86_64` (or other x86_64 board with
  EFI boot and GOP).
* **Boot**: UEFI (e.g. QEMU with OVMF). The sample builds an EFI
  application (``CONFIG_BUILD_OUTPUT_EFI=y``).
* **Display**: The EFI GOP display driver must be enabled
  (``CONFIG_DISPLAY_EFI_GOP``) and the devicetree overlay must set
  ``chosen zephyr,display`` to the GOP display node (see
  ``boards/qemu_x86_64.overlay``).

Building and Running
********************

**Prerequisites**: Install ``uefi-run`` and OVMF
(e.g. ``sudo apt install ovmf``):

.. code-block:: console

   cargo install uefi-run

For pflash mode support (required with newer split OVMF images), use the
fork that adds ``--pflash``:

.. code-block:: console

   cargo install --git https://github.com/rede97/uefi-run uefi-run

Set either ``OVMF_FD_PATH`` (legacy single FD) or both
``OVMF_CODE_FD_PATH`` and ``OVMF_VARS_FD_PATH`` for pflash mode::

   export OVMF_CODE_FD_PATH=/usr/share/OVMF/OVMF_CODE_4M.fd
   export OVMF_VARS_FD_PATH=/usr/share/OVMF/OVMF_VARS_4M.fd

When using GOP, the run target adds ``-vga std`` only; ``-display`` is left
at QEMU default. For a graphical window, use system or custom QEMU: set
``QEMU_BIN_PATH`` (e.g. ``export QEMU_BIN_PATH=/usr/bin``) or enable
``CONFIG_QEMU_USE_SYSTEM`` in menuconfig.

Build with the board overlay (enables EFI GOP and UEFI boot for run)::

   west build -b qemu_x86_64 -d build/efi_gop \\
       samples/drivers/efi_gop -- \\
       -DOVERLAY_FILE=boards/qemu_x86_64.overlay

Run with graphics (QEMU uses OVMF and ``-vga std`` so the GOP display works)::

   west build -t run -d build/efi_gop

Without ``CONFIG_QEMU_UEFI_BOOT`` and OVMF/uefi-run, ``west build -t run``
would use SeaBIOS and no display; the sample enables UEFI boot so that
the run target shows the LVGL UI in a QEMU window.

References
**********

* Display API: :ref:`display_api`
* EFI GOP display driver (``CONFIG_DISPLAY_EFI_GOP``)
* LVGL: https://lvgl.io/
