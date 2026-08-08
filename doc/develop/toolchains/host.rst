.. _host_toolchains:

Host Toolchains
###############

In some configurations, you may want to use compiler toolchains installed on your host
operating system rather than Zephyr's SDK pre-packaged cross-compilers.

Using Host GCC
==============

To build using the host's native GCC compiler, set the :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`
environment variable to ``host/gnu``. This is typically used when building for native
simulator (POSIX architecture) based targets on a Linux host.

Using Host LLVM/Clang (``host/llvm``)
=====================================

Building for Native Simulator Targets
-------------------------------------

You can also use host-installed LLVM/Clang for building native simulator (POSIX
architecture) based targets on Linux. This uses the host OS's native C library and system
headers rather than the embedded Zephyr SDK sysroots.

Building for Embedded Targets
-----------------------------

Zephyr allows you to build standard embedded bare-metal firmware using a host-installed
version of LLVM/Clang (such as Apple Clang, Homebrew LLVM, or package-managed LLVM on
Linux) while dynamically binding to the bare-metal runtimes and sysroots shipped inside
the Zephyr SDK.

This is highly beneficial for utilizing the latest compiler optimizations, tools, or custom
build pipelines without losing target device library support.

.. note::
   When building for **Espressif targets** (particularly Xtensa-based chips like ESP32,
   ESP32-S2, and ESP32-S3), the upstream LLVM backend is still experimental. It is highly
   recommended to use the official
   `Espressif LLVM fork <https://github.com/espressif/llvm-project>`_
   as your ``host/llvm`` compiler to ensure full compatibility and prevent backend crashes.

Setup and Variables
~~~~~~~~~~~~~~~~~~~

To use the host LLVM toolchain, configure the following variables (either in your
environment or as CMake arguments):

1. **Set the Toolchain Variant**:
   Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``host/llvm``.

2. **Specify the Host LLVM Directory** *(Optional)*:
   Set the :envvar:`LLVM_TOOLCHAIN_PATH` (or :envvar:`CLANG_ROOT_DIR`) variable to the
   root of your host LLVM installation (the directory containing ``bin/clang``).

   If unset, the build system automatically attempts to discover your host LLVM compiler
   across standard system locations, Homebrew opt paths (supporting versioned formulas from
   ``llvm@15`` up to ``llvm@25``), and versioned Linux folders (``/usr/lib/llvm-XX``).

3. **Specify the Zephyr SDK Directory**:
   Because host Clang is a native compiler and lacks built-in target runtime libraries
   (like ``libclang_rt.builtins.a``) for embedded architectures, it needs to link against
   the runtimes packaged inside the Zephyr SDK.

   Ensure that :envvar:`ZEPHYR_SDK_INSTALL_DIR` (or :envvar:`ZEPHYR_TOOLCHAIN_PATH`) is
   set to the path of your installed Zephyr SDK. The toolchain automatically resolves the
   correct target architecture triplets and sysroots from it.

Example Shell Usage
-------------------

To configure the build using environment variables:

.. code-block:: console

   # 1. Point to host-installed LLVM (example for macOS Homebrew)
   export LLVM_TOOLCHAIN_PATH=$(brew --prefix llvm)

   # 2. Point to the Zephyr SDK installation
   export ZEPHYR_SDK_INSTALL_DIR=$HOME/zephyr-sdk-1.0.1

   # 3. Compile the build using the host LLVM variant
   west build -b esp32_devkitc -- -DZEPHYR_TOOLCHAIN_VARIANT=host/llvm

Using custom linker choice
--------------------------

By default, the LLVM toolchain variant compiles with the standard LLVM linker (``LLD``).
You can manually override the linker choice by setting the Kconfig choice
``CONFIG_LLVM_USE_LD=y`` if you need to fall back to the GNU linker.

Comparison: Host LLVM (``host/llvm``) vs SDK LLVM (``zephyr/llvm``)
===================================================================

Zephyr offers two distinct ways to build your firmware with the LLVM/Clang compiler:

*   **SDK-Bundled LLVM (``zephyr/llvm``)**:

    *   **Description**: Uses the precompiled Clang/LLD binary toolchain shipped directly
        inside the Zephyr SDK.
    *   **How to run**: Set ``ZEPHYR_TOOLCHAIN_VARIANT=zephyr/llvm`` (shorthand for
        setting variant to ``zephyr`` and ``TOOLCHAIN_VARIANT_COMPILER=llvm``).
    *   **LLVM_TOOLCHAIN_PATH**: Not used, as the build system automatically resolves the
        compiler path inside the active SDK directory.

*   **Host-Installed LLVM (``host/llvm``)**:

    *   **Description**: Uses a Clang/LLVM compiler installed directly on your host machine
        (e.g., Apple Clang, Homebrew LLVM, or system-packaged LLVM on Linux) while
        referencing target headers and libraries from the Zephyr SDK.
    *   **How to run**: Set ``ZEPHYR_TOOLCHAIN_VARIANT=host/llvm``.
    *   **LLVM_TOOLCHAIN_PATH**: Can be used (or the ``CLANG_ROOT_DIR`` environment
        variable) to point to the base installation folder of your host LLVM toolchain.
        If left unset, the build system automatically queries your host environment for
        standard installation folders.
