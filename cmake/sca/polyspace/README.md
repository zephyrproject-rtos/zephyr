# Static Code Analysis with Polyspace
This folder contains the CMake files to run a Static Code Analysis with the tool [Polyspace](https://mathworks.com/products/polyspace.html)&reg; from MathWorks. It can check compliance to coding guidelines like MISRA C and CERT C, find CWEs, detect bugs, calculate code complexity metrics, and optionally run a formal proof to verify the absence of run-time errors like array out of bounds access, overflows, race conditions and more.

## Prerequisites
The Polyspace tools must be installed and made available in the operating system's or container's PATH variable. Specifically, the path `<polyspace_root>/polyspace/bin` must be on the list.

For installation instructions, see [here](https://mathworks.com/help/bugfinder/install-polyspace.html).
To use formal verification (proving the *absence* of defects), you additionally need to install [this](https://mathworks.com/help/codeprover/install-polyspace.html).

A license file must be available in the installation. To request a trial license, visit [this page](https://www.mathworks.com/campaigns/products/trials.html).

## Usage
The code analysis can be triggered through the `west` command, for example:

    west build -b qemu_x86 samples/hello_world -- -DZEPHYR_SCA_VARIANT=polyspace

The following options are supported, to tweak analysis behavior:

| Option | Effect | Example |
|--------|--------|---------|
| `POLYSPACE_ONLY_APP` | If set, only user code is analyzed and Zephyr sources are ignored. | `-DPOLYSPACE_ONLY_APP=1` |
| `POLYSPACE_OPTIONS` | Provide additional command line flags, e.g., for selection of coding rules. Separate the options and their values by semicolon. For a list of options, see [here](https://mathworks.com/help/bugfinder/referencelist.html?type=analysisopt&s_tid=CRUX_topnav). | `-DPOLYSPACE_OPTIONS="-misra3;mandatory-required;-checkers;all"`|
| `POLYSPACE_OPTIONS_FILE` | Command line flags can also be provided in a text file, line by line. Use the absolute path to the file. | `-DPOLYSPACE_OPTIONS_FILE=/workdir/zephyr/myoptions.txt` |
| `POLYSPACE_MODE` | Switch between bugfinding and proving mode. Default is bugfinding. See [here](https://mathworks.com/help/bugfinder/gs/use-bug-finder-and-code-prover.html) for more details. | `-DPOLYSPACE_MODE=prove` |
| `POLYSPACE_PROG_NAME` | Override the name of the analyzed application. Default is board and application name. | `-DPOLYSPACE_PROG_NAME=myapp` |
| `POLYSPACE_PROG_VERSION` | Override the version of the analyzed application. Default is taken from git-describe. | `-DPOLYSPACE_PROG_VERSION=v1.0b-28f023` |
