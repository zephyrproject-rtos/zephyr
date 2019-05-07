.. _third_party_x_compilers:

3rd Party Toolchains
####################

A "3rd party toolchain" is an officially supported toolchain provided by an
external organization. Several of these are available.

.. _toolchain_gnuarmemb:

GNU ARM Embedded
****************

#. Download and install a `GNU ARM Embedded`_ build for your operating system
   and extract it on your file system.

   .. warning::

      Do not install the toolchain into a path with spaces. On
      Windows, we'll assume you install into the directory
      :file:`C:\\gnu_arm_embedded`.

   .. warning::

	  The GNU ARM Embedded Toolchain for Windows, version **8-2018-q4-major**
	  has a `critical bug <https://github.com/zephyrproject-rtos/zephyr/issues/12257>`_
	  and should not be used. Toolchain version **7-2018-q2-update** is known to work.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``gnuarmemb``.
   - Set :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`GNUARMEMB_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      gnuarmemb
      $ echo $GNUARMEMB_TOOLCHAIN_PATH
      /home/you/Downloads/gnu_arm_embedded

      # Windows
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      gnuarmemb
      > echo %GNUARMEMB_TOOLCHAIN_PATH%
      C:\gnu_arm_embedded

Intel ISSM
**********

.. note::

   The ISSM toolset only supports development for Intel |reg| Quark |trade|
   Microcontrollers, for example, the Arduino 101 board.  (Check out the
   "Zephyr Development Environment
   Setup" in this `Getting Started on Arduino 101 with ISSM`_ document.)
   Additional setup is required to use the ISSM GUI for development.

#. Install the ISSM toolchain by downloading it from the Intel Developer Zone's
   `ISSM Toolchain`_ downloads page, then extracting the archive somewhere on
   your system.

   .. warning::

      Like the Zephyr repository, do not install the toolchain in a directory
      with spaces anywhere in the path.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``issm``.
   - Set :envvar:`ISSM_INSTALLATION_PATH` to the directory containing the
     extracted files.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`ISSM_INSTALLATION_PATH` values may be different on your system):

   .. code-block:: console

      # Linux
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      issm
      $ echo $ISSM_INSTALLATION_PATH
      /home/you/Downloads/issm0-toolchain-windows-2017-02-07

      # Windows
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      issm
      > echo %ISSM_INSTALLATION_PATH%
      c:\issm0-toolchain-windows-2017-01-25

.. _xtools_x_compilers:

Crosstool-NG
************

You can build toolchains from source code using crosstool-NG.

#. Follow the steps on the crosstool-NG website to `prepare your host
   <http://crosstool-ng.github.io/docs/os-setup/>`_.

#. Follow the `Zephyr SDK with Crosstool NG instructions
   <https://github.com/zephyrproject-rtos/sdk-ng/blob/master/README.md>`_ to
   build your toolchain. Repeat as necessary to build toolchains for multiple
   target architectures.

   You will need to clone the ``sdk-ng`` repo and run the following command:

   .. code-block:: console

      ./go.sh <arch>

   .. note::

      Currently, only i586 and Arm toolchain builds are verified.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xtools``.
   - Set :envvar:`XTOOLS_TOOLCHAIN_PATH` to the toolchain build directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`XTOOLS_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      xtools
      $ echo $XTOOLS_TOOLCHAIN_PATH
      /Volumes/CrossToolNGNew/build/output/

.. _GNU ARM Embedded: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm
.. _ISSM Toolchain: https://software.intel.com/en-us/articles/issm-toolchain-only-download
.. _Getting Started on Arduino 101 with ISSM: https://software.intel.com/en-us/articles/getting-started-arduino-101genuino-101-with-intel-system-studio-for-microcontrollers
.. _crosstool-ng site: http://crosstool-ng.org
