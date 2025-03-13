.. _polyspace:

Polyspace support
#################

`Polyspace® <https://mathworks.com/products/polyspace.html>`__ is
a commercial static code analysis tool from MathWorks, which is certified
for use in highest safety contexts. It can check compliance to coding
guidelines like MISRA C and CERT C, find CWEs, detect bugs and calculate
code complexity metrics. Optionally, it can run a formal proof to verify
the absence of run-time errors like array out of bounds access, overflows,
race conditions and more, and thus help achieving memory safety.

Installing
**********

The Polyspace tools must be installed and made available in the
operating system's or container's PATH variable. Specifically, the path
``<polyspace_root>/polyspace/bin`` must be on the list.

For installation instructions, see
`here <https://mathworks.com/help/bugfinder/install-polyspace.html>`__.
To use formal verification (proving the *absence* of defects), you
additionally need to install
`this <https://mathworks.com/help/codeprover/install-polyspace.html>`__.

A license file must be available
in the installation folder. To request a trial license, visit `this
page <https://www.mathworks.com/campaigns/products/trials.html>`__.

Running
*******

The code analysis can be triggered through the ``west`` command by
appending the option ``-DZEPHYR_SCA_VARIANT=polyspace`` to the build,
for example:

.. code-block:: shell

   west build -b qemu_x86 samples/hello_world -- -DZEPHYR_SCA_VARIANT=polyspace

Reviewing results
*****************

The identified issues are summarized at the end of the build and printed
in the console window, along with the path of the folder containing
detailed results.

For an efficient review, the folder should be opened in the
`Polyspace user interface <https://mathworks.com/help/bugfinder/review-results-1.html>`__
or `uploaded to the web interface <https://mathworks.com/help/bugfinder/gs/run-bug-finder-on-server.html>`__
and reviewed there.

For programmatic access of the results, e.g., in the CI pipeline, the
individual issues are also described in a CSV file in the results folder.

Configuration
*************

By default, Polyspace scans for typical programming defects on all C and C++ sources.
The following options are available to customize the default behavior:

.. list-table::
   :widths: 20 40 30
   :header-rows: 1

   * - Option
     - Effect
     - Example
   * - ``POLYSPACE_ONLY_APP``
     - If set, only user code is analyzed and Zephyr sources are ignored.
     - ``-DPOLYSPACE_ONLY_APP=1``
   * - ``POLYSPACE_OPTIONS``
     - Provide additional command line flags, e.g., for selection of coding
       rules. Separate the options and their values by semicolon. For a list
       of options, see `here <https://mathworks.com/help/bugfinder/referencelist.html?type=analysisopt&s_tid=CRUX_topnav>`__.
     - ``-DPOLYSPACE_OPTIONS="-misra3;mandatory-required;-checkers;all"``
   * - ``POLYSPACE_OPTIONS_FILE``
     - Command line flags can also be provided in a text file, line by
       line. Provide the absolute path to the file.
     - ``-DPOLYSPACE_OPTIONS_FILE=/workdir/zephyr/myoptions.txt``
   * - ``POLYSPACE_MODE``
     - Switch between bugfinding and proving mode. Default is bugfinding.
       See `here <https://mathworks.com/help/bugfinder/gs/use-bug-finder-and-code-prover.html>`__ for more details.
     - ``-DPOLYSPACE_MODE=prove``
   * - ``POLYSPACE_PROG_NAME``
     - Override the name of the analyzed application. Default is board
       and application name.
     - ``-DPOLYSPACE_PROG_NAME=myapp``
   * - ``POLYSPACE_PROG_VERSION``
     - Override the version of the analyzed application. Default is taken
       from git-describe.
     - ``-DPOLYSPACE_PROG_VERSION=v1.0b-28f023``
