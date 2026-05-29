.. _cpptest:

Parasoft C/C++test support
##########################

Parasoft `C/C++test <https://www.parasoft.com/products/parasoft-c-ctest/>`__ is a software testing
and static analysis tool for C and C++. It is a commercial software and you must acquire a
commercial license to use it.

Documentation of C/C++test can be found at https://docs.parasoft.com/.  Please refer to the
documentation for how to use it.

Generating Build Data Files
***************************

To use C/C++test, ``cpptestscan`` must be found in your :envvar:`PATH` environment variable.  And
:ref:`west build <west-building>` should be called with a ``-DZEPHYR_SCA_VARIANT=cpptest``
parameter, e.g.

.. code-block:: shell

    west build -b qemu_cortex_m3 zephyr/samples/hello_world -- -DZEPHYR_SCA_VARIANT=cpptest


A ``.bdf`` file will be generated as :file:`build/sca/cpptest/cpptestscan.bdf`.

Generating a report file
************************

Please refer to Parasoft C/C++test documentation for more details.

To import and generate a report file, something like the following should work.

.. code-block:: shell

    cpptestcli -data out -localsettings local.conf -bdf build/sca/cpptest/cpptestscan.bdf -config "builtin://Recommended Rules" -report out/report


You might need to set ``bdf.import.c.compiler.exec``, ``bdf.import.cpp.compiler.exec``, and
``bdf.import.linker.exec`` to the toolchain :ref:`west build <west-building>` used.
