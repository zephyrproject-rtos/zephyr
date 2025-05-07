.. _clang:

Clang static analyzer support
#############################

Clang Static Analyzer is built on top of Clang and LLVM.
Strictly speaking, the analyzer is part of Clang, as Clang
consists of a set of reusable C++ libraries for building
powerful source-level tools. The static analysis engine used by the
Clang Static Analyzer is a Clang library, and has the capability to
be reused in different contexts and by different clients.

LLVM provides various methods to run the analyzer on a codebase,
through either a dedicated set of tools (scan-build and analyze-build),
or via command line arguments when running clang ('--analyze').

- 'scan-build' utility comes as the most convenient way for projects
  using a simple $CC makefile variables, as it will wraps and replace
  the compiler calls to perform it's analysis.

- 'analyze-build' utility is a sub-tool from 'scan-build', it only
  relies on a 'compile_commands.json' database to perform the analysis.

- clang option '--analyze' will run the analyzer alongside the build, but
  objects files are not generated, making any link stage impossible. In
  our case the first link stage will fail and stop the analysis.

Because of it's complexe build infrastructure, invoking clang analyzer with
'analyze-build' is the most simple way to analyze a Zephyr project.

`Clang static analyzer documentation <https://clang.llvm.org/docs/ClangStaticAnalyzer.html>`__

Installing clang analyzer
*************************

'scan-build' and its sub-tool 'analyze-build' come natively with llvm as part of the binaries.
Make sure to have the binary directory accessible into your PATH.

'scan-build' is also available as a standalone python package available on `pypi <https://pypi.org/project/scan-build/>`__.

.. code-block:: shell

    pip install scan-build

Run clang static analyzer
*************************

.. note::

  The analyser requires that the project builds with a LLVM toolchain, and
  produces a 'compile_commands.json' database.

To run clang static analyzer, :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=clang`` parameter, alongside the llvm
toolchain parameters, e.g.

.. zephyr-app-commands::
   :zephyr-app: samples/userspace/hello_world_user
   :board: qemu_x86
   :gen-args: -DZEPHYR_TOOLCHAIN_VARIANT=llvm -DLLVM_TOOLCHAIN_PATH=... -DZEPHYR_SCA_VARIANT=clang
   :goals: build
   :compact:

.. note::

  By default, clang static analyzer produces a html report, but various other
  outputs can be selected with options (sarif, plist, html)

Configuring clang static analyzer
*********************************

Clang static analyzer can be controlled using specific options.
To get an exaustive list of available options, report to the
'analyze-build' helper and 'scan-build' helper.

.. code-block:: shell

    analyze-build --help

Options already activated by default:

* --analyze-headers : Also analyze functions in #included files.

.. list-table::
   :header-rows: 1

   * - Parameter
     - Description
   * - ``CLANG_SCA_OPTS``
     - A semicolon separated list of 'analyze-build' options.

These parameters can be passed on the command line, or be set as environment variables.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h573i_dk
   :gen-args: -DZEPHYR_TOOLCHAIN_VARIANT=llvm -DLLVM_TOOLCHAIN_PATH=... -DZEPHYR_SCA_VARIANT=clang -DCLANG_SCA_OPTS="--sarif;--verbose"
   :goals: build
   :compact:
