
.. _CONFIG_SERIAL_INTERRUPT_LEVEL:

CONFIG_SERIAL_INTERRUPT_LEVEL
#############################


Option signifies that the serial controller uses level interrupts
instead of edge



:Symbol:           SERIAL_INTERRUPT_LEVEL
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Serial interrupt level" if IOAPIC && EXTRA_SERIAL_PORT (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: IOAPIC && EXTRA_SERIAL_PORT (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 X86_32 (value: "y")
:Locations:
 * drivers/serial/Kconfig:46