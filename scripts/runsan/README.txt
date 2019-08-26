
runsan
======

This is a highly simplified rig to run Zephyr ztest tests in a
constrained environment where they will not have to compete with
builds, other tests, or sanitycheck itself for host CPU resources, but
still using maximum parallelism.  It's useful for determining if
spurious test failures are the result of coarse host scheduling or not
(micro-timing races will/should still be present in a setup like this,
of course).

Because it doesn't build the tests for every iteration, it's also
useful when run in a loop for exercising as many tests as possible in
a given amount of time.

Unlike sanitycheck, it doesn't run a thread in parallel with each
test, it simply loops watching the output to see if the test has
finished.

Usage:

0. Apply the included patch to your zephyr tree.  This modifies the
   build and test environment to produce metadata (just the list of
   test directories and the qemu commands run).

1. Run sanitycheck with the desired options to populate a
   $ZEPHYR_BASE/sanity-out tree with built images.

2. Make a temporary run directory somewhere.

3. In that directory, run setup-runsan.pl.  It will populate the local
   directory from the trees it finds in $ZEPHYR_BASE by copying the
   relevant data (sanitycheck cleans its output every time it is run,
   making managing big test environments like this annoying).  Note
   that it produces a "dirlist" file with a list of local test
   directories.  Note that this copy can be quite large currently:
   about 18G for a full sanitycheck tree (really it should just be
   copying out the zephyr.elf and qemu cmd files, but it grabs the
   whole directory right now)

4. Run runsan.pl with that dirlist file as input.  It will enumerate
   at least once through each test, running N-1 tests at a time on a
   machine with N physical cores.  Standard output contains the only
   output of the script, and should be self-explanatory.  In each test
   directory you will find various *.log files capturing the output
   streams (stdout, stderr, monitor and serial) from the simulator.

Limitations:

+ Only supports qemu targets

+ Only supports ztest syntax for detecting test success.  Testcases
  defined with YAML regexes are ignored and will be reported as a
  "timeout" failure.

+ Similarly doesn't support tests with "build_only: true".  If it
  qfinds a runnable zephyr.elf, it's going to try to run it and will
  hang if the output doesn't include ztest results.

+ Likewise, no support for spawning external tools required by many
  network tests.

+ (Effectively: keep to tests/{kernel,posix,cmsis*}, or else be
   prepared to hand-prune the sanity-out tree)
