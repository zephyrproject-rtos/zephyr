
.. _CONFIG_BOOT_BANNER:

CONFIG_BOOT_BANNER
##################


This option outputs a banner to the console device during boot up. It
also embeds a date & time stamp in the kernel and in each USAP image.



:Symbol:           BOOT_BANNER
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Boot banner"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:

 *  :ref:`CONFIG_PRINTK`
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/nanokernel/Kconfig:35