Footprint and Behavior Test for cbprintf variants
#################################################

This ensures that formatted output to the console works as expected with
minimal libc and newlib versions with printk, printf, and cbprintf.

Footprint data can be obtained with:

    for f in sanity-out/*/tests/lib/cbprintf_fp/benchmark.cbprintf_fp.*/build.log ; do
      basename $(dirname $f)
      sed -n '/Memory/,/^\[/p' < $f
    done
