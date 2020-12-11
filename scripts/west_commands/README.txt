This directory contains implementations for west commands which are
tightly coupled to the zephyr tree. This includes the build, flash,
and debug commands.

Before adding more here, consider whether you might want to put new
extensions in upstream west. For example, any commands which operate
on the multi-repo need to be in upstream west, not here. Try to limit
what goes in here to Zephyr-specific features.

When extending this code, please keep the unit tests (in tests/) up to
date. The mypy static type checker is also run on the runners package.

To run these tests locally on Windows, run:

   py -3 run_tests.py

On macOS and Linux:

   ./run_tests.py

Note that these tests are run as part of Zephyr's CI when submitting
an upstream pull request, and pull requests which break the tests
cannot be merged.

Thanks!
