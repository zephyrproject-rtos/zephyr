#!/bin/bash
# run the filter-known-issues.py script to remove "expected" warning
# messages from the output of the document build process
# Only argument is the name of the log file saved by the build.

$ZEPHYR_BASE/scripts/filter-known-issues.py --config-dir \
      $ZEPHYR_BASE/.known-issues/doc $1 > $1.warn || \
      (echo " ==> Error running filter-known-issues"; exit 1); \
      if [ -s $1.warn ]; then \
        echo " => New documentation warnings/errors"; \
        cat $1.warn;
        exit 1; \
      fi;
