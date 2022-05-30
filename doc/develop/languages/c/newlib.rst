.. _c_library_newlib:

Newlib
######

`Newlib`_ is a complete C library implementation written for the embedded
systems. It is a separate open source project and is not included in source
code form with Zephyr. Instead, the :ref:`toolchain_zephyr_sdk` includes a
precompiled library for each supported architecture (:file:`libc.a` and
:file:`libm.a`).

.. note::
   Other 3rd-party toolchains, such as :ref:`toolchain_gnuarmemb`, also bundle
   the Newlib as a precompiled library.

Zephyr implements the "API hook" functions that are invoked by the C standard
library functions in the Newlib. These hook functions are implemented in
:file:`lib/libc/newlib/libc-hooks.c` and translate the library internal system
calls to the equivalent Zephyr API calls.

The Newlib included in the :ref:`toolchain_zephyr_sdk` comes in two versions:
'full' and 'nano' variants.

Full Newlib
***********

The Newlib full variant (:file:`libc.a` and :file:`libm.a`) is the most capable
variant of the Newlib available in the Zephyr SDK, and supports almost all
standard C library features. It is optimized for performance (prefers
performance over code size) and its footprint is significantly larger than the
the nano variant.

This variant can be enabled by selecting the
:kconfig:option:`CONFIG_NEWLIB_LIBC` and de-selecting the
:kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` in the application configuration
file.

Nano Newlib
***********

The Newlib nano variant (:file:`libc_nano.a` and :file:`libm_nano.a`) is the
size-optimized version of the Newlib, and supports all features that the full
variant supports except the ``long long`` and ``double`` type format
specifiers.

This variant can be enabled by selecting the
:kconfig:option:`CONFIG_NEWLIB_LIBC` and
:kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` in the application configuration
file.

Note that the Newlib nano variant is not available for all architectures. The
availability of the nano variant is specified by the
:kconfig:option:`CONFIG_HAS_NEWLIB_LIBC_NANO`.

When available (:kconfig:option:`CONFIG_HAS_NEWLIB_LIBC_NANO` is selected),
the Newlib nano variant is enabled by default unless
:kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` is explicitly de-selected.

.. _`Newlib`: https://sourceware.org/newlib/
