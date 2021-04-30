.. _longan_nano:

Sipeed Longan Nano
##################

Overview
********

The Sipeed Longan Nano and Longan Nano Lite is an simple and tiny development board with
an GigaDevice GD32VF103 SoC that based on N200 RISC-V IP core by Nuclie system.
More information can be found on
`Sipeed's website <https://longan.sipeed.com/en/>`_ and
`Nuclei's website <https://www.nucleisys.com/download.php>`_.

Programming and debugging
*************************

Building
========

Applications for the ``logan_nano`` board configuration can be built as usual
(see :ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: logan_nano
   :goals: build

Flashing
========

In order to upload the application to the device, you'll need OpenOCD with
GD32V support. Download the tarball for your OS from the
`SiPEED longan nano download site
<http://dl.sipeed.com/LONGAN/platformio/dl-packages/>` and extract it.

The Zephyr SDK uses a bundled version of OpenOCD by default. You can
overwrite that behavior by adding the
``-DOPENOCD=<path/to/riscv-openocd/bin/openocd>`` and
``-DOPENOCD_DEFAULT_PATH=<path/to/openocd/scripts>``
parameter when building:

.. zephyr-app-commands::
   :board: longan_nano
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
