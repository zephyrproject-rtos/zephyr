MCUBoot Swap Testing
#####################

Tests MCUBoot's image swap support. This application is built in three parts
using sysbuild. The first application is the MCUBoot bootloader. The second
application is the main sysbuild target, and will request an image swap
from MCUBoot when booted. The third application is build with a load address
adjustment using CONFIG_BUILD_OUTPUT_ADJUST_LMA, and will be the application
that MCUBoot swaps to when the image swap is requested.

This sequence of applications allows the test to verify support for the MCUBoot
upgrade process on any platform supporting it.
