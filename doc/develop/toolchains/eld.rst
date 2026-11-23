.. _toolchain_eld:

Embedded Linker (ELD)
#####################

`ELD`_ is Qualcomm's open-source, LLVM-based, GNU-compatible linker. It can
be used as an alternative to LLD or GNU ld when building Zephyr with an
LLVM toolchain (for example :ref:`Arm Toolchain for Embedded
<toolchain_atfe>` or a host clang). See the `ELD user guide`_ for details on
ELD itself.

ELD is just a linker (``ld.eld``); a separate C/C++ toolchain is still
required to compile sources.

Installation
************

There are three ways to obtain ELD:

#. **Nightly binary release.** Prebuilt ``ld.eld`` binaries are published on
   the `ELD releases`_ page.

#. **Build from source.** Build ELD against an LLVM tree as described in the
   `ELD README`_; an integrated ``llvm-project`` build produces ``ld.eld``
   under the build tree's ``bin/`` directory.

#. **Full toolchain via cpullvm.** The `cpullvm`_ toolchain is a complete
   LLVM toolchain for Arm, AArch64 and RISC-V embedded targets that bundles
   both ``ld.eld`` and ``ld.lld`` along with clang, the runtime libraries
   and headers. Prebuilt cpullvm archives are published on its
   `cpullvm releases page`_ for Linux and Windows, and cpullvm 22.1.1 has
   been tested with Zephyr in the ELD CI.

Options 1 and 2 only provide ``ld.eld``; a separate LLVM toolchain (clang,
runtime libraries and headers) is still needed to build Zephyr.

Verify the installation:

.. code-block:: console

   $ ld.eld --version
   eld 22.0 (GNU Compatible linker)

ELD 22.0 or later is required.

Usage
*****

ELD is selected through the ``host/llvm`` toolchain variant. Point
:envvar:`LLVM_TOOLCHAIN_PATH` at the LLVM toolchain (for example a cpullvm
installation, or another toolchain with ``ld.eld`` on its ``bin/`` directory
or :envvar:`PATH`), then enable ELD by setting
:kconfig:option:`CONFIG_LLVM_USE_ELD` to ``y``.

For example:

.. code-block:: console

   west build ... -- -DZEPHYR_TOOLCHAIN_VARIANT=host/llvm \
                     -DLLVM_TOOLCHAIN_PATH=<path-to-llvm-toolchain> \
                     -DCONFIG_LLVM_USE_ELD=y

See :ref:`toolchain_atfe` or :ref:`toolchain_zephyr_sdk` for more on
configuring an LLVM toolchain.

.. _ELD: https://github.com/qualcomm/eld
.. _ELD user guide: https://qualcomm.github.io/eld/
.. _ELD releases: https://github.com/qualcomm/eld/releases
.. _ELD README: https://github.com/qualcomm/eld#building-eld-and-running-tests
.. _cpullvm: https://github.com/qualcomm/cpullvm-toolchain
.. _cpullvm releases page: https://github.com/qualcomm/cpullvm-toolchain/releases
