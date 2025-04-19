.. _toolchain_atfe:

Arm Toolchain for Embedded (ATfE)
#################################

#. Download and install a `Arm toolchain for embedded`_ build for your operating system
   and extract it on your file system.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``atfe``.
   - Set :envvar:`ATFE_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`ATFE_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      atfe
      $ echo $ATFE_TOOLCHAIN_PATH
      /home/you/Downloads/ATfE

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      atfe
      > echo %ATFE_TOOLCHAIN_PATH%
      C:\ATfE

.. _Arm Toolchain for Embedded: https://developer.arm.com/Tools%20and%20Software/Arm%20Toolchain%20for%20Embedded
