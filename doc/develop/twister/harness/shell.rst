.. _twister_shell_harness:

Shell
#####

The shell harness is used to execute shell commands and parse the output and utilizes the pytest
framework and the pytest harness of twister.

The following options apply to the shell harness:

shell_commands: <list of pairs of commands and their expected output> (default empty)
    Specify a list of shell commands to be executed and their expected output.
    For example:

    .. code-block:: yaml

        harness_config:
          shell_commands:
          - command: "kernel cycles"
            expected: "cycles: .* hw cycles"
          - command: "kernel version"
            expected: "Zephyr version .*"
          - command: "kernel sleep 100"


    If expected output is not provided, the command will be executed and the output
    will be logged.

shell_commands_file: <string> (default empty)
    Specify a file containing test parameters to be used in the test.
    The file should contain a list of commands and their expected output. For example:

    .. code-block:: yaml

      - command: "mpu mtest 1"
        expected: "The value is: 0x.*"
      - command: "mpu mtest 2"
        expected: "The value is: 0x.*"


    If no file is specified, the shell harness will use the default file
    ``test_shell.yml`` in the test directory.
    ``shell_commands`` will take precedence over ``shell_commands_file``.
