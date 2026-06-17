.. _twister_script_harness:

Script
######

The ``script`` harness executes shell scripts as test cases. It resolves scripts
from the ``tests_scripts`` harness configuration option, runs each script as
a subprocess, and reports individual pass/fail results based on the script
exit code.

The ``script`` harness also serves as a base class for the ``bsim``, ``pytest``,
and ``ctest`` harnesses, providing shared subprocess execution, output
streaming, and log handling.

tests_scripts: <list of script paths> (default tests_scripts)
    Specify a list of shell script paths, relative to the test source
    directory, that need to be executed when a test scenario runs.
    Each entry can be a path to a single file or a directory.
    When a directory is specified, all ``.sh`` files in that directory
    are collected (excluding files starting with ``_``). The default
    is the ``tests_scripts`` directory.

    .. code-block:: yaml

        harness: script
        harness_config:
          tests_scripts:
            - tests_scripts/test_a.sh
            - ../../test/test_b.sh
            - $ENV_VAR/tests_scripts

Extra arguments following ``--`` on the twister command line are passed to
every script as additional positional arguments.
