.. _gcc:

GCC static analysis support
###########################

Static analysis was introduced in `GCC <https://gcc.gnu.org/>`__ 10 and it is enabled
with the option ``-fanalyzer``. This option performs a much more expensive and thorough
analysis of the code than traditional warnings.

Run GCC static analysis
***********************

To run GCC static analysis, :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=gcc`` parameter, e.g.

.. zephyr-app-commands::
   :zephyr-app: samples/userspace/hello_world_user
   :board: qemu_x86
   :gen-args: -DZEPHYR_SCA_VARIANT=gcc
   :goals: build
   :compact:

Configuring GCC static analyzer
*******************************

GCC static analyzer can be controlled using specific options.

* `Options controlling the
  analyzer <https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html>`__
* `Options controlling the diagnostic message
  formatting <https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Message-Formatting-Options.html>`__

.. list-table::
   :header-rows: 1

   * - Parameter
     - Description
   * - ``GCC_SCA_OPTS``
     - A semicolon separated list of GCC analyzer options.

These parameters can be passed on the command line, or be set as environment variables.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h573i_dk
   :gen-args: -DZEPHYR_SCA_VARIANT=gcc -DGCC_SCA_OPTS="-fdiagnostics-format=json;-fanalyzer-verbosity=3"
   :goals: build
   :compact:

.. note::

   GCC static analyzer is under active development, and each new release comes with new options.
   This `page <https://gcc.gnu.org/wiki/StaticAnalyzer>`__ gives an overview of options and fix
   introduced with each new release of the analyzer.


Latest version of the analyzer
******************************

Since the Zephyr toolchain may not include the most recent version of the GCC static analyzer,
the GCC static analysis can be run with a more recent `GNU Arm embedded toolchain
<https://docs.zephyrproject.org/latest/develop/toolchains/gnu_arm_embedded.html>`__
to take advantage of the latest analyzer version.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h573i_dk
   :gen-args: -DZEPHYR_SCA_VARIANT=gcc -DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb -DGNUARMEMB_TOOLCHAIN_PATH=...
   :goals: build
   :compact:
