.. _snippet-ram-console:

RAM Console Snippet (ram-console)
#################################

.. code-block:: console

   west build -S ram-console [...]

Overview
********

This snippet redirects console output to a RAM buffer, which is linked to
a dedicated section created by linker scripts through a device tree node
with compatible string "zephyr,memory-region".

How to add support of a new board
*********************************

Add board dts overlay to this snippet to add memory-region node with node
label ``snippet_ram_console`` as the following:

.. code-block:: DTS

   snippet_ram_console: memory@93d00000 {
        compatible = "zephyr,memory-region";
        reg = <0x93d00000 DT_SIZE_K(4)>;
        zephyr,memory-region = "RAM_CONSOLE";
   };
