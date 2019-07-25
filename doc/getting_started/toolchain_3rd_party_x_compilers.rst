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
.. _crosstool-ng site: http://crosstool-ng.org
