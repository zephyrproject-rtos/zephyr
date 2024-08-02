This directory used to contain the edtlib.py and dtlib.py libraries
and tests, alongside the gen_defines.py script that uses them for
converting DTS to the C macros used by Zephyr.

The libraries and tests have now been moved to the 'python-devicetree'
subdirectory.

We are now in the process of extracting edtlib and dtlib into a
standalone source code library that we intend to share with other
projects.

Links related to the work making this standalone:

    https://pypi.org/project/devicetree/
    https://python-devicetree.readthedocs.io/en/latest/
    https://github.com/zephyrproject-rtos/python-devicetree

The 'python-devicetree' subdirectory you find here next to this
README.txt matches the standalone python-devicetree repository linked
above.

For now, the 'main' copy will continue to be hosted here in the zephyr
repository. We will mirror changes into the standalone repository as
needed; you can just ignore it for now.

Code in the zephyr repository which needs these libraries will import
devicetree.edtlib from now on, but the code will continue to be found
by manipulating sys.path for now.

Eventually, as APIs stabilize, the python-devicetree code in this
repository will disappear, and a standalone repository will be the
'main' one.
