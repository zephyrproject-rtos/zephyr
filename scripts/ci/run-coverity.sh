#!/bin/bash
#
# This script builds the application using the Coverity Scan build tool,
# and prepares the archive for uploading to the cloud static analyzer.
#

function die() { echo "$@" 1>&2; exit 1; }

rm -rf /tmp/cov-build/cov-int
export PATH=$PATH:${SHIPPABLE_BUILD_DIR}/cov-analysis/bin
which cov-configure && which cov-build || die "Coverity Build Tool is not in PATH"

#cov-configure --comptype gcc --compiler i586-zephyr-elfiamcu-gcc --template
#cov-build --dir /tmp/cov-build/cov-int sanitycheck -a x86 --all -b

#cov-configure --comptype gcc --compiler arm-zephyr-eabi-gcc --template
#cov-build --dir /tmp/cov-build/cov-int sanitycheck -a arm --all -b

cov-configure --comptype gcc --compiler arc-zephyr-elf-gcc --template
cov-build --dir /tmp/cov-build/cov-int sanitycheck -a arc --all -b

cd /tmp/cov-build
ls -lR cov-int
tar czvf coverity.tgz cov-int

echo "Done. Please submit the archive to Coverity Scan now."
