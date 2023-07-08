.. _shakti-vajra:

SHAKTI C-Class Core
###################

Overview
********

The Arty A7 100T FPGA is used to prove the Shakti C-Class SOC named Vajra Core.
More information can be found on
`Shakti's website <https://shakti.org.in/>`_.


Get the Toolchain
*****************

The minimum version of the `Shakti SDK tools
<https://gitlab.com/shaktiproject/software/shakti-tools.git>`_
with toolchain for the RISCV64 architecture is required to be cloned and installed.
Please see the :ref:`installation instructions <install-required-tools>`
for more details.

Programming and debugging
*************************

Building
========

Applications for the ``shakti_vajra`` board configuration can be built as usual
(see :ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: shakti_vajra
   :goals: build

Flashing
========

In order to upload the application to the device, you'll need OpenOCD with
RISC-V support. Download the tarball for your OS from the `Shakti SDK tools
<https://gitlab.com/shaktiproject/software/shakti-tools.git>`_ and extract it.

The Zephyr SDK uses a bundled version of OpenOCD by default. You can
overwrite that behavior by adding the
``-DOPENOCD=<path/to/shakti-tools/bin/openocd>`` parameter when building:

.. zephyr-app-commands::
   :board: shakti_vajra
   :goals: build
   :gen-args: -DOPENOCD=<path/to/shakti-tools/bin/openocd>

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
