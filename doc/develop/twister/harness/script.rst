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

tests_scripts_parallel: <boolean> (default true)
    Run the scripts of a scenario at the same time, as far as Twister has free job slots.
    The first script in flight runs on the job the worker already holds for the scenario,
    so it never waits; each further script in flight takes a free job from the ``-j``
    budget, without waiting for one, and gives it back when it ends. The load therefore
    never exceeds ``-j``, and a scenario with many scripts speeds up as other work
    finishes. Set it to ``false`` for scripts that must not run at the same time, for
    example because they share a simulation id or a file. The results and logs are recorded
    in the order the scripts are listed, whichever order they finish in.

Any extra command-line arguments passed to Twister after ``--`` are forwarded to every script as
additional positional arguments.
