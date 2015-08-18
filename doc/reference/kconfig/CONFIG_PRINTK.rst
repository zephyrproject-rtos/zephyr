
.. _CONFIG_PRINTK:

CONFIG_PRINTK
#############


This option directs printk() debugging output to the supported
console device (UART), rather than suppressing the generation
of printk() output entirely. Output is sent immediately, without
any mutual exclusion or buffering.



:Symbol:           PRINTK
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Send printk() to console"
:Default values:

 *  y (value: "y")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 BOOT_BANNER (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:105