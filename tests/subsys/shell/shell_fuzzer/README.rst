Fuzzing Shell
###############

Overview
********

This is a simple fuzz test for shell module.
It uses LLVM libfuzzer's.

Building and Running
********************

There is needed clang (tested at clang 10 x64).

.. code-block:: console

   $apt install clang
   $clang --version

There is needed llvm toolchain selected:

.. code-block:: console

   $ export ZEPHYR_TOOLCHAIN_VARIANT=llvm

Build test application:

.. code-block:: console

   $ cd tests/subsys/shell/shell_fuzzer
   $ west build -b native_posix_64

Prepare for running

Make UndefinedBehaviorSanitizer stopping at error and showing stacktrace:
(this is the place you can add suppression file)

.. code-block:: console

   $ export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1


Enable more checks from AddressSanitizer and stopping at error:

(this is the place you can add suppression file)

.. code-block:: console

   $ export ASAN_OPTIONS=check_initialization_order=1:detect_leaks=1:detect_stack_use_after_return=1:halt_on_error=1

Run fuzzer:

(Enable detailed tracking of code and leak detection. Specify max length of test vectors and number of executions. Allow to use only printable characters for input vectors.)

.. code-block:: console

   $ ./build/zephyr/zephyr.exe -use_value_profile=1 -detect_leaks=1 -max_len=64 -runs=100000 -only_ascii

Details:

https://llvm.org/docs/LibFuzzer.html
