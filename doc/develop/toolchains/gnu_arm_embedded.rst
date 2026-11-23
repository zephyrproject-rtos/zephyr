.. _toolchain_gnuarmemb:

GNU Arm Embedded
################

#. Download and install a `GNU Arm Embedded`_ build for your operating system
   and extract it on your file system.

   .. note::

      On Windows, we'll assume for this guide that you install into the directory
      :file:`C:\\gnu_arm_embedded`. You can also choose the default installation
      path used by the ARM GCC installer, in which case you will need to adjust the path
      accordingly in the guide below.

   .. warning::

      On macOS Catalina or later you might need to :ref:`change a security
      policy <mac-gatekeeper>` for the toolchain to be able to run from the
      terminal.

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

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      gnuarmemb
      > echo %GNUARMEMB_TOOLCHAIN_PATH%
      C:\gnu_arm_embedded

   .. warning::

      On macOS, if you are having trouble with the suggested procedure, there is an unofficial package on brew that might help you.
      Run ``brew install gcc-arm-embedded`` and configure the variables

      - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``gnuarmemb``.
      - Set :envvar:`GNUARMEMB_TOOLCHAIN_PATH` to the brew installation directory (something like ``/usr/local``)

.. _GNU Arm Embedded: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm
