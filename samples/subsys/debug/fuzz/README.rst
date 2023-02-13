Fuzzing Example
###############

Overview
********

This is a simple example of fuzz test integration with Zephyr apps
that displays LLVM libfuzzer's most important feature: it's ability to
detect and explore deep and complicated call trees by exploiting
coverage information gleaned from instrumented binaries.

Building and Running
********************

Right now, the only toolchain that works with libfuzzer is a recent 64
bit clang (clang 14 was used at development time).  Make sure such a
toolchain is installed in your host environment, and build with:

.. code-block:: console

   $ clang --version
   clang version 14.0.6
   Target: x86_64-pc-linux-gnu
   Thread model: posix
   InstalledDir: /usr/bin
   $ export ZEPHYR_TOOLCHAIN_VARIANT=llvm
   $ west build -t run -b native_posix_64 samples/subsys/debug/fuzz

Over 10-20 seconds or so (runtimes can be quite variable) you will see
it discover and recurse deeper into the test's deliberately
constructed call tree, eventually crashing when it reaches the final
state and reporting the failure.

Example output:

.. code-block:: console

   -- west build: running target run
   [0/1] cd /home/andy/z/zephyr/build && .../andy/z/zephyr/build/zephyr/zephyr.exe
   INFO: Running with entropic power schedule (0xFF, 100).
   INFO: Seed: 108038547
   INFO: Loaded 1 modules   (2112 inline 8-bit counters): 2112 [0x55cbe336ec55, 0x55cbe336f495),
   INFO: Loaded 1 PC tables (2112 PCs): 2112 [0x55cbe336f498,0x55cbe3377898),
   INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
   *** Booting Zephyr OS build zephyr-v3.1.0-3976-g806034e02865  ***
   Hello World! native_posix_64
   INFO: A corpus is not provided, starting from an empty corpus
   #2	INITED cov: 101 ft: 102 corp: 1/1b exec/s: 0 rss: 30Mb
   #
   # Found key 0
   #
   NEW_FUNC[1/6]: 0x55cbe3339c45 in check1 /home/andy/z/zephyr/samples/subsys/debug/fuzz/src/main.c:43
   NEW_FUNC[2/6]: 0x55cbe333c8d8 in char_out /home/andy/z/zephyr/lib/os/printk.c:108
   ...
   ...
   ...
   #418965	REDUCE cov: 165 ft: 166 corp: 15/400b lim: 4052 exec/s: 38087 rss: 31Mb L: 5/256 MS: 1 EraseBytes-
   #524288	pulse  cov: 165 ft: 166 corp: 15/400b lim: 4096 exec/s: 40329 rss: 31Mb
   #
   # Found key 5
   #
   NEW_FUNC[1/1]: 0x55cbe3339ff7 in check6 /home/andy/z/zephyr/samples/subsys/debug/fuzz/src/main.c:48
   #579131	NEW    cov: 168 ft: 169 corp: 16/406b lim: 4096 exec/s: 38608 rss: 31Mb L: 6/256 MS: 1 InsertByte-
   #579432	NEW    cov: 170 ft: 171 corp: 17/414b lim: 4096 exec/s: 38628 rss: 31Mb L: 8/256 MS: 1 PersAutoDict- DE: "\000\000"-
   #579948	REDUCE cov: 170 ft: 171 corp: 17/413b lim: 4096 exec/s: 38663 rss: 31Mb L: 7/256 MS: 1 EraseBytes-
   #
   # Found key 6
   #
   UndefinedBehaviorSanitizer:DEADLYSIGNAL
   ==3243305==ERROR: UndefinedBehaviorSanitizer: SEGV on unknown address 0x000000000000 (pc 0x55cbe333a09d bp 0x7f3114afadf0 sp 0x7f3114afade0 T3243308)
   ==3243305==The signal is caused by a WRITE memory access.
   ==3243305==Hint: address points to the zero page.
       #0 0x55cbe333a09d in check6 /home/andy/z/zephyr/samples/subsys/debug/fuzz/src/main.c:48:1
