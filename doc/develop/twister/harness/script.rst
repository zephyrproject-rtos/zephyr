.. _twister_script_harness:

Script
######

The ``script`` harness executes shell scripts as test cases. It resolves scripts
from the ``tests_scripts`` harness configuration option, runs each script as
a subprocess, and reports individual pass/fail results based on the script
exit code.

The ``script`` harness also serves as a base class for the :ref:`bsim <twister_bsim_harness>`,
:ref:`pytest <twister_pytest_harness>`, and :ref:`ctest <twister_ctest_harness>` harnesses,
providing shared subprocess execution, output streaming, and log handling.

tests_scripts: <list of script paths> (default tests_scripts)
    Specify a list of shell script paths, relative to the test source
    directory, that need to be executed when a test scenario runs.
    Each entry can be a path to a single file, a directory, or a glob pattern.
    When a directory is specified, all ``.sh`` files in that directory,
    including its sub-directories, are collected (excluding files starting with ``_``).
    The default is the ``tests_scripts`` directory.

    .. code-block:: yaml

        harness: script
        harness_config:
          tests_scripts:
            - tests_scripts/test_a.sh
            - ../../test/test_b.sh
            - $ENV_VAR/tests_scripts

tests_scripts_parallel: <integer> (default 1)
    Maximum number of the scripts of one test scenario that are run at the same time, at least 1.
    By default the scripts run one after the other. Set this for scenarios whose scripts
    are independent of each other, for example because each uses its own simulation id,
    so that a scenario with many scripts does not keep one CPU busy while the others idle.
    The scripts started this way are not counted against the ``-j/--jobs`` limit of
    Twister, so keep the value small enough for the test load.
    The results and logs are recorded in the order the scripts are listed, whichever
    order they finish in.

Any extra command-line arguments passed to Twister after ``--`` are forwarded to every script as
additional positional arguments.
