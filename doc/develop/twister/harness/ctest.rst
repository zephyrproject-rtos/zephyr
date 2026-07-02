.. _twister_ctest_harness:

Ctest
#####

ctest_args: <list of arguments> (default empty)
    Specify a list of additional arguments to pass to ``ctest`` e.g.:
    ``ctest_args: [‘--repeat until-pass:5’]``. Note that
    ``--ctest-args`` can be passed multiple times to pass several arguments
    to the ctest.
