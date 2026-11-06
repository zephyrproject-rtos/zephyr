.. _toolchain_iar_rh850:

IAR RH850 Toolchain
#################

#. Download and install a release v3.20 or newer of `IAR RH850 Toolchain`_ on your host (IAR Embedded Workbench or IAR Build Tools for Renesas RH850, perpetual or subscription licensing)

#. Make sure you have :ref:`Zephyr SDK <toolchain_zephyr_sdk>` installed on your host.

#. :ref:`Set these environment variables <env_vars>`:

    - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``iar``.
    - Set :envvar:`IAR_TOOLCHAIN_PATH` to the toolchain installation directory.

For example:

.. code-block:: bash

    # Linux (default installation path):
    export IAR_TOOLCHAIN_PATH=/opt/iar/ewrh850-<version>/rh850
    export ZEPHYR_TOOLCHAIN_VARIANT=iar

.. code-block:: batch

    # Windows:
    set IAR_TOOLCHAIN_PATH=c:\<path>\ewrh850-<version>\rh850
    set ZEPHYR_TOOLCHAIN_VARIANT=iar

.. note::

    Known limitations:

    - The IAR Toolchain uses ``ilink`` for linking and depends on Zephyr’s CMAKE_LINKER_GENERATOR. ``ilink`` is incompatible with Zephyr’s linker script template, which works with GNU ld.

    - C library support for ``Minimal libc`` only. C++ is not supported.

    - Some Zephyr subsystems or modules may contain C or assembly code that relies on GNU intrinsics and have not yet been updated to work fully with ``iar``.

    - TrustedFirmware is not supported

    - The IAR RH850 toolchain is not freely available. A valid commercial license from IAR is required in order to use this compiler.

.. _IAR RH850 Toolchain: https://www.iar.com/embedded-development-tools/free-trials
