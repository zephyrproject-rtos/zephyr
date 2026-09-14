.. _toolchain_hexagon:

Qualcomm Hexagon LLVM Toolchain
###############################

#. Download a pre-built Hexagon LLVM cross-toolchain from the
   `toolchain_for_hexagon releases page
   <https://github.com/quic/toolchain_for_hexagon/releases>`_ and extract it,
   for example to ``/opt/hexagon-toolchain``. The install directory is the one
   holding ``bin/clang``.

   Use a release built from LLVM 23 or newer. Earlier releases miscompile the
   ``~BIT(n)``.  The fix landed
   in `#205489 <https://github.com/llvm/llvm-project/pull/205489>`_.

#. Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``hexagon`` and
   :envvar:`HEXAGON_TOOLCHAIN_PATH` to that directory:

   .. code-block:: bash

      export ZEPHYR_TOOLCHAIN_VARIANT=hexagon
      export HEXAGON_TOOLCHAIN_PATH=/opt/hexagon-toolchain

.. envvar:: HEXAGON_TOOLCHAIN_PATH

   Install directory of the Hexagon LLVM cross-toolchain.

The toolchain is an ordinary LLVM installation, so this variant is the
:ref:`host_toolchains` ``llvm`` variant driving a different compiler. It has a
path variable of its own because the Hexagon clang registers only the Hexagon
target and so cannot build host-compiled targets such as
:zephyr:board:`native_sim`; an environment that covers both has to name the two
LLVM installations independently, as Zephyr's CI does when a single Twister run
spans both kinds of platform.

Building a Hexagon target with ``ZEPHYR_TOOLCHAIN_VARIANT=host/llvm`` and
:envvar:`LLVM_TOOLCHAIN_PATH` is equivalent and remains supported, as long as
the installation it names is the Hexagon one.
