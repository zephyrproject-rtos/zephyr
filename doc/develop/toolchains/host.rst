.. _host_toolchains:

Host Toolchains
###############

In some specific configurations, like when building for non-MCU x86 targets on
a Linux host, you may be able to reuse the native development tools provided
by your operating system.

To use your host gcc, set the :envvar:`ZEPHYR_TOOLCHAIN_VARIANT`
:ref:`environment variable <env_vars>` to ``host``. To use clang, set
:envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``llvm``.
