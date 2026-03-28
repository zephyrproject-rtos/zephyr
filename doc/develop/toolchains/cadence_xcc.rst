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

   * There are two ways to specify the SoC ID and the SDK version to use.
     They are mutually exclusive, and cannot be used together.

     #. When building for a single SoC:

        * Set :envvar:`XTENSA_CORE` to the SoC ID where application is being
          targeted.
        * Set :envvar:`TOOLCHAIN_VER` to the Xtensa SDK version.

     #. When building for multiple SoCs, for each SoC and board combination:

        * Set :envvar:`XTENSA_CORE_{normalized_board_target}`
          to the SoC ID where application is being targeted.
        * Set :envvar:`TOOLCHAIN_VAR_{normalized_board_target}`
          to the Xtensa SDK version.

#. For example, assuming the SDK is installed in ``/opt/xtensa``, and
   using the SDK for application development on ``intel_adsp/ace15_mtpm``,
   setup the environment using the two above mentioned ways:

   #. Single SoC:

      .. code-block:: console

         # Linux
         export ZEPHYR_TOOLCHAIN_VARIANT=xt-clang
         export XTENSA_TOOLCHAIN_PATH=/opt/xtensa/XtDevTools/install/tools/
         export XTENSA_CORE=ace10_LX7HiFi4_2022_10
         export TOOLCHAIN_VER=RI-2022.10-linux

   #. Multiple SoCs:

      .. code-block:: console

         # Linux
         export ZEPHYR_TOOLCHAIN_VARIANT=xt-clang
         export XTENSA_TOOLCHAIN_PATH=/opt/xtensa/XtDevTools/install/tools/
         export TOOLCHAIN_VER_intel_adsp_ace15_mtpm=RI-2022.10-linux
         export XTENSA_CORE_intel_adsp_ace15_mtpm=ace10_LX7HiFi4_2022_10

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
