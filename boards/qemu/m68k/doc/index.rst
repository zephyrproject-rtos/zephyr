.. zephyr:board:: qemu_m68k

Overview
********

The QEMU M68K board runs Zephyr on QEMU's ``virt`` machine. It supports two
CPU choices:

* Motorola 68000
* Motorola 68010

Use one of these board targets:

* ``qemu_m68k/qemu_virt_m68k_m68000``
* ``qemu_m68k/qemu_virt_m68k_m68010``

The board provides 16 MiB of RAM and these emulated devices:

* six Goldfish interrupt controllers, connected to CPU interrupt levels 1 to 6
* a Goldfish RTC used as the system timer
* a Goldfish TTY used for the console and shell

Hardware
********

Supported Features
^^^^^^^^^^^^^^^^^^

.. zephyr:board-supported-hw::

Devices
^^^^^^^

Interrupt controller
""""""""""""""""""""

QEMU provides six Goldfish interrupt controllers. They are connected to the
68000-family autovector levels 1 through 6.

System timer
""""""""""""

The Goldfish RTC provides the system timer. QEMU runs the RTC from the virtual
machine clock.

Console
"""""""

The Goldfish TTY is the default console and shell device.

Reset
"""""

A Zephyr reboot writes the reset command to QEMU's ``virt-ctrl`` device.

Known limitations
^^^^^^^^^^^^^^^^^

* Only the Motorola 68000 and 68010 CPU choices are supported.
* The board does not provide networking or Bluetooth devices.
* The image is loaded into RAM. The board does not provide flash or persistent
  storage.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Motorola 68000
^^^^^^^^^^^^^^

Build and run the :zephyr:code-sample:`hello_world` sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_m68k/qemu_virt_m68k_m68000
   :goals: run

Motorola 68010
^^^^^^^^^^^^^^

Build and run the :zephyr:code-sample:`hello_world` sample with:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_m68k/qemu_virt_m68k_m68010
   :goals: run

Exit QEMU by pressing :kbd:`CTRL+A` and then :kbd:`x`.
