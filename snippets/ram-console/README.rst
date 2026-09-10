.. _snippet-ram-console:

RAM Console Snippet (ram-console)
#################################

.. code-block:: console

   west build -S ram-console [...]

Overview
********

This snippet redirects console output to a RAM buffer. The RAM console
buffer is a global array located in RAM region by default, whose address
is unknown before building. The RAM console driver also supports using
a dedicated section for the RAM console buffer with prefined address.

How to enable RAM console buffer section
****************************************

Add board dts overlay to this snippet to add property ``zephyr,ram-console``
in the chosen node and memory-region node with compatible string
:dtcompatible:`zephyr,memory-region` as the following:

.. code-block:: DTS

    / {
        chosen {
            zephyr,ram-console = &snippet_ram_console;
        };

       snippet_ram_console: memory@93d00000 {
            compatible = "zephyr,memory-region";
            reg = <0x93d00000 DT_SIZE_K(4)>;
            zephyr,memory-region = "RAM_CONSOLE";
       };
    };
