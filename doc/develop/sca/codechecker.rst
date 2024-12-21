.. _codechecker:

CodeChecker support
###################

`CodeChecker <https://codechecker.readthedocs.io/>`__ is a static analysis infrastructure.
It executes analysis tools available on the build system, such as
`Clang-Tidy <http://clang.llvm.org/extra/clang-tidy/>`__,
`Clang Static Analyzer <http://clang-analyzer.llvm.org/>`__ and
`Cppcheck <https://cppcheck.sourceforge.io/>`__. Refer to the analyzer's websites for installation
instructions.

Installing CodeChecker
**********************

CodeChecker itself is a python package available on `pypi <https://pypi.org/project/codechecker/>`__.

.. code-block:: shell

    pip install codechecker

Building with CodeChecker
*************************

To run CodeChecker, :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=codechecker`` parameter, e.g.

.. code-block:: shell

    west build -b mimxrt1064_evk samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=codechecker


Configuring CodeChecker
***********************

CodeChecker uses different command steps, each with their respective configuration
parameters. The following table lists all these options.

.. list-table::
   :header-rows: 1

   * - Parameter
     - Description
   * - ``CODECHECKER_ANALYZE_JOBS``
     - The number of threads to use in analysis. (default: <CPU count>)
   * - ``CODECHECKER_ANALYZE_OPTS``
     - Arguments passed to the ``analyze`` command directly. (e.g. ``--timeout;360``)
   * - ``CODECHECKER_CLEANUP``
     - Perform a cleanup after parsing/storing. This will remove all ``plist`` files.
   * - ``CODECHECKER_CONFIG_FILE``
     - A JSON or YAML file with configuration options passed to all commands.
   * - ``CODECHECKER_EXPORT``
     - A comma separated list of report types. Allowed types are:
       ``html,json,codeclimate,gerrit,baseline``.
   * - ``CODECHECKER_NAME``
     - The CodeChecker run metadata name. (default: ``zephyr``)
   * - ``CODECHECKER_PARSE_EXIT_STATUS``
     - By default, CodeChecker identified issues will not fail the build, set this option to fail
       during analysis.
   * - ``CODECHECKER_PARSE_OPTS``
     - Arguments passed to the ``parse`` command directly. (e.g. ``--verbose;debug``)
   * - ``CODECHECKER_PARSE_SKIP``
     - Skip parsing the analysis, useful if you simply want to store the results.
   * - ``CODECHECKER_STORE``
     - Run the ``store`` command after analysis.
   * - ``CODECHECKER_STORE_OPTS``
     - Arguments passed to the ``store`` command directly. Implies ``CODECHECKER_STORE``.
       (e.g. ``--url;localhost:8001/Default``)
   * - ``CODECHECKER_STORE_TAG``
     - Pass an identifier ``--tag`` to the ``store`` command.
   * - ``CODECHECKER_TRIM_PATH_PREFIX``
     - Trim the defined path from the analysis results, e.g. ``/home/user/zephyrproject``.
       The value from ``west topdir`` is added by default.

These parameters can be passed on the command line, or be set as environment variables.


Running twister with CodeChecker
********************************

When running CodeChecker as part of ``twister`` some default options are set as following:

.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
   * - ``CODECHECKER_ANALYZE_JOBS``
     - ``1``
   * - ``CODECHECKER_NAME``
     - ``<board target>:<testsuite name>``
   * - ``CODECHECKER_STORE_TAG``
     - The value from ``git describe`` in the application source directory.

To override these values, set an environment variable or pass them as extra arguments.
