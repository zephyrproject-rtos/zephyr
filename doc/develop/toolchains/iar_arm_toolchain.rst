.. _toolchain_iar_arm:

IAR Arm Toolchain
#################

#. Download and install a release v9.70 or newer of `IAR Arm Toolchain`_ on your host (IAR Embedded Workbench or IAR Build Tools, perpetual or subscription licensing)

#. Make sure you have :ref:`Zephyr SDK <toolchain_zephyr_sdk>` installed on your host.

#. :ref:`Set these environment variables <env_vars>`:

    - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``iar``.
    - Set :envvar:`IAR_TOOLCHAIN_PATH` to the toolchain installation directory.

#. The cloud licensed variant of the IAR Toolchain needs the :envvar:`IAR_LMS_BEARER_TOKEN` environment
   variable to be set to a valid ``license bearer token`` (subscription licensing).

For example:

.. code-block:: bash

    # Linux (default installation path):
    export IAR_TOOLCHAIN_PATH=/opt/iar/cxarm-<version>/arm
    export ZEPHYR_TOOLCHAIN_VARIANT=iar
    export IAR_LMS_BEARER_TOKEN="<BEARER-TOKEN>"

.. code-block:: batch

    # Windows:
    set IAR_TOOLCHAIN_PATH=c:\<path>\cxarm-<version>\arm
    set ZEPHYR_TOOLCHAIN_VARIANT=iar
    set IAR_LMS_BEARER_TOKEN="<BEARER-TOKEN>"

.. note::

    Known limitations:

    - The IAR Toolchain uses ``ilink`` for linking and depends on Zephyr’s CMAKE_LINKER_GENERATOR. ``ilink`` is incompatible with Zephyr’s linker script template, which works with GNU ld.

    - The GNU Assembler distributed with the Zephyr SDK is used for ``.S-files``.

    - C library support for ``Minimal libc`` only. C++ is not supported.

    - Some Zephyr subsystems or modules may contain C or assembly code that relies on GNU intrinsics and have not yet been updated to work fully with ``iar``.

    - TrustedFirmware is not supported

.. _IAR Arm Toolchain: https://www.iar.com/products/architectures/arm/
