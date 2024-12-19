from twister_harness import Shell

def test_shell_print_version(shell: Shell):
    lines = shell.exec_command("kernel version")
    assert any(['Zephyr version' in line for line in lines]), 'failed to retrieve Zephyr version'
