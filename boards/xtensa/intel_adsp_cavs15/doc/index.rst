.. _Up_Squared_Audio_DSP:

Up Squared Audio DSP
####################

System Requirements
*******************

Prerequisites
=============

The Zephyr SDK 0.11 or higher is required.

Since firmware binary signing for Audio DSP is mandatory on Intel products
form Skylake onwards the signing tool and key are needed.

``up_squared`` board is running Linux with `SOF Diagnostic Driver`_ built and
loaded.

Signing tool
------------

rimage is Audio DSP firmware image creation and signing tool. The tool is used
by `Sound Open Firmware`_ to generate binary firmware signed images.

For the building instructions refer to `rimage Build Instructions`_.

Signing keys
------------

The key used is Intel Open Source Technology Center (OTC) community key.
It can be freely used by anyone and intended for firmware developers.
Please download and store private key from the location:
https://github.com/thesofproject/sof/blob/master/keys/otc_private_key.pem

For more information about keys refer to `rimage keys`_.

Setup up_squared board
----------------------

To setup Linux on ``up_squared`` board refer to
`Getting Started with Ubuntu Core on an UP Squared Board`_.

After installing Linux build and install `SOF Diagnostic Driver`_.

Programming and Debugging
*************************

Build Zephyr application
========================

Applications can be build in the usual way (see :ref:`build_an_application`
for more details). The only additional step required is signing. For example,
for building ``hello_world`` application following steps are needed.

#. Building Zephyr application ``hello_world``

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_adsp_cavs15
      :goals: build

#. Sign and create firmware image

   .. code-block:: console

      west sign -t rimage -- -k <path to otc_private_key.pem>

Loading image to Audio DSP
==========================

Use the ``cavs-fw.py`` script to load a signed image onto the DSP on
the device.  This script needs write access so the PCI device BAR in
sysfs (see below) and the CAP_SYS_ADMIN capability to change the
vm.nr_hugepages setting in the kernel.  In practice it's easiest to
run it as root.

.. code-block:: console

   $ sudo boards/xtensa/intel_adsp_cavs15/tools/cavs-fw.py <path to zephyr.ri>

Debugging
=========

The only way to debug application is using logging. Logging and ADSP logging
backend needs to be enabled in the application configuration.

ADSP logging backend writes logs to the ring buffer in the shared memory.

As above, the ``adsplog`` tool requires appropriate permissions, in
this case to the sysfs "resource4" device on the appropriate PCI
device.  This can likewise be managed via any filesystem, setuid or
udev trick the operator prefers.

.. code-block:: console

   $ boards/xtensa/intel_adsp_cavs15/tools/adsplog.py
   ERROR: Cannot open /sys/bus/pci/devices/0000:00:0e.0/resource4 for reading

   $ sudo chmod 666 /sys/bus/pci/devices/0000:00:0e.0/resource4
   $ boards/xtensa/intel_adsp_cavs15/tools/adsplog.py
   Hello World! intel_adsp_cavs15

Integration Testing With Twister
================================

The ADSP hardware also has integration for testing using the twister
tool from a remote build system with ssh access to the DSP device
under test.  A single "cavsload.sh" script is provided that can act as
both the device-serial-pty handler and the west-flash loader.

.. code-block:: console

    $ CAVSLOAD=$ZEPHYR_BASE/boards/xtensa/intel_adsp_cavs15/tools/cavsload.sh
    $ $ZEPHYR_BASE/scripts/twister -p intel_adsp_cavs15 --device-testing \
      --device-serial-pty=$CAVSLOAD --west-flash=$CAVSLOAD

See configuration details at the top of the script.  Note the script
may also be passed a built zephyr.elf file as a single argument and it
will sign, load and run it, emitting the console to standard output:

.. code-block:: console

    $ZEPHYR_BASE/boards/xtensa/intel_adsp_cavs15/tools/cavsload.sh build/zephyr/zephyr.elf

.. target-notes::

.. _Getting Started with Ubuntu Core on an UP Squared Board: https://software.intel.com/en-us/articles/getting-started-with-ubuntu-core-on-an-up-squared-board

.. _Sound Open Firmware: https://github.com/thesofproject/sof

.. _rimage Build Instructions: https://github.com/thesofproject/rimage#building

.. _rimage keys: https://github.com/thesofproject/sof/tree/master/rimage/keys
