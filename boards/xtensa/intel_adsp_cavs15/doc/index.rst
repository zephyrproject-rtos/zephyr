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

`SOF Diagnostic Driver`_ provide interface for firmware loading. Python tools
in the board support directory use the interface to load firmware to ``ADSP``.

Assume that the up_squared board's host name is ``cavs15`` (It also can be an
ip address), and the user account is ``user``. Then copy the python tool to the
``up_squared`` board from your build environment::

   $ scp boards/xtensa/intel_adsp/tools/cavstool.py user@cavs15:
   $ scp boards/xtensa/intel_adsp/tools/remote-fw-service.py user@cavs15:


Note that the ``/dev/hda`` device file created by the diagnostic driver must
be readable and writable by the process.  So we simply by running the
loader script as root:

.. code-block:: console

   cavs15$ sudo ./remote-fw-service.py

Cavstool_server.py is a daemon which accepts a firmware image from a remote host
and loads it into the ADSP. After successful firmware download, the daemon also
sends any log messages or output back to the client.

Running and Debugging
=====================

While the python script is running on ``up_squared`` board, you can start load
image and run the application by:

.. code-block:: console

   west flash --remote-host cavs15

or

.. code-block:: console

   west flash --remote-host 192.168.x.x --pty

Then you can see the log message immediately:

.. code-block:: console

   Hello World! intel_adsp_cavs15


Integration Testing With Twister
================================

The ADSP hardware also has integration for testing using the twister
tool.  The ``cavstool_client.py`` script can be used as the
``--device-serial-pty`` handler, and the west flash script should take
a path to the same key file used above.

.. code-block:: console

    ./scripts/twister --device-testing -p intel_adsp_cavs15 \
      --device-serial-pty $ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,myboard.local,-l \
      --west-flash "--remote-host=myboard.local"

And if you install the SOF software stack in rather than the default path,
you also can specify the location of the rimage tool, signing key and the
toml config, for example:

.. code-block:: console

    ./scripts/twister --device-testing -p intel_adsp_cavs15 \
      --device-serial-pty $ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool_client.py,myboard.local,-l \
      --west-flash "--remote-host=myboard.local,\
      --rimage-tool=/path/to/rimage_tool,\
      --key=/path/to/otc_private_key.pem,\
      --config-dir=/path/to/config_dir"


.. target-notes::

.. _Getting Started with Ubuntu Core on an UP Squared Board: https://software.intel.com/en-us/articles/getting-started-with-ubuntu-core-on-an-up-squared-board

.. _SOF Diagnostic Driver: https://github.com/thesofproject/sof-diagnostic-driver

.. _Sound Open Firmware: https://github.com/thesofproject/sof

.. _rimage Build Instructions: https://github.com/thesofproject/rimage#building

.. _rimage keys: https://github.com/thesofproject/sof/tree/master/rimage/keys
