import os


def crash_if_zephyr_was_cloned_with_wrong_line_endings():
    f = open('{}/Kconfig'.format(os.environ["ZEPHYR_BASE"]), 'U')
    f.readline()

    error_msg = "Re-clone with autocrlf false. $ git config --global core.autocrlf false"

    assert f.newlines == '\n', error_msg

def main():
    crash_if_zephyr_was_cloned_with_wrong_line_endings()

if __name__ == "__main__":
    main()
