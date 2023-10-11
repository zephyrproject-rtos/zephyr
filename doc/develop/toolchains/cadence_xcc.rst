.. _toolchain_cadence_xcc:

Cadence Tensilica Xtensa C/C++ Compiler (XCC)
#############################################

#. Obtain Tensilica Software Development Toolkit targeting the specific SoC
   on hand. This usually contains two parts:

   * The Xtensa Xplorer which contains the necessary executables and
     libraries.

   * A SoC-specific add-on to be installed on top of Xtensa Xplorer.

     * This add-on allows the compiler to generate code for the SoC on hand.

#. Install Xtensa Xplorer and then the SoC add-on.

   * Follow the instruction from Cadence on how to install the SDK.

   * Depending on the SDK, there are two set of compilers:

     * GCC-based compiler: ``xt-xcc`` and its friends.

     * Clang-based compiler: ``xt-clang`` and its friends.

#. Make sure you have obtained a license to use the SDK, or has access to
   a remote licensing server.

#. :ref:`Set these environment variables <env_vars>`:

   * Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xcc`` or ``xt-clang``.
   * Set :envvar:`XTENSA_TOOLCHAIN_PATH` to the toolchain installation
     directory.
   * Set :envvar:`XTENSA_CORE` to the SoC ID where application is being
     targeting.
   * Set :envvar:`TOOLCHAIN_VER` to the Xtensa SDK version.

#. For example, assuming the SDK is installed in ``/opt/xtensa``, and
   using the SDK for application development on ``intel_adsp_cavs15``,
   setup the environment using:

   .. code-block:: console

      # Linux
      export ZEPHYR_TOOLCHAIN_VARIANT=xcc
      export XTENSA_TOOLCHAIN_PATH=/opt/xtensa/XtDevTools/install/tools/
      export XTENSA_CORE=X6H3SUE_RI_2018_0
      export TOOLCHAIN_VER=RI-2018.0-linux

#. To use Clang-based compiler:

   * Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xt-clang``.

   * Note that the Clang-based compiler may contain an old LLVM bug which
     results in the following error:

     .. code-block:: console

        /tmp/file.s: Assembler messages:
        /tmp/file.s:20: Error: file number 1 already allocated
        clang-3.9: error: Xtensa-as command failed with exit code 1

     If this happens, set :envvar:`XCC_NO_G_FLAG` to ``1``.

     * For example:

       .. code-block:: console

          # Linux
          export XCC_NO_G_FLAG=1

   * Also note that setting :envvar:`XCC_USE_CLANG` to ``1`` and
     :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xcc`` is deprecated.
     Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xt-clang`` instead.
