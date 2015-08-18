
.. _CONFIG_SERIAL_INTERRUPT_LOW:

CONFIG_SERIAL_INTERRUPT_LOW
###########################


Option signifies that the serial controller uses low level interrupts
instead of high



:Symbol:           SERIAL_INTERRUPT_LOW
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Serial interrupt low" if IOAPIC && EXTRA_SERIAL_PORT (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: IOAPIC && EXTRA_SERIAL_PORT (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 X86_32 (value: "y")
:Locations:
 * drivers/serial/Kconfig:54