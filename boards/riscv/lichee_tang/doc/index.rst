.. _lichee_tang:

Sipeed Lichee Tang
##################

Overview
********

The Sipeed Lichee Tang is Anlogic EG4S20 FPGA development board.
It shipped with Nuclei E203 HummingBird RISC-V core.
More information can be found on
`Sipeed's website <https://tang.sipeed.com/en/>`_.

`E203 HummingBird RISC-V core port on Lichee Tang <https://github.com/Lichee-Pi/Tang_E203_Mini>`_.

Programming and debugging
*************************

Building
========

Applications for the ``lichee_tang`` board configuration can be built as usual
(see :ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: lichee_tang
   :goals: build

Flashing
========

In order to upload the application to the device, you'll need OpenOCD with
E203 support. Download the source codes from the
`Lichee_Tang_openocd repository
<https://github.com/Lichee-Pi/LicheeTang_openocd>` and build it.

The Zephyr SDK uses a bundled version of OpenOCD by default. You can
overwrite that behavior by adding the
``-DOPENOCD=<path/to/riscv-openocd/bin/openocd>`` and
``-DOPENOCD_DEFAULT_PATH=<path/to/openocd/scripts>``
parameter when building:

.. zephyr-app-commands::
   :board: lichee_tang
   :goals: build
   :gen-args: -DOPENOCD=<path/to/riscv-openocd/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/scripts>

When using a custom toolchain it should be enough to have the downloaded
version of the binary in your ``PATH``.

Now you can flash the application as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details):

.. code-block:: console

   ninja flash

Depending on your OS you might have to run the flash command as superuser.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
