
.. _CONFIG_ARCV2_INTERRUPT_UNIT:

CONFIG_ARCV2_INTERRUPT_UNIT
###########################


The ARCv2 interrupt unit has 16 allocated exceptions associated with
vectors 0 to 15 and 240 interrupts associated with vectors 16 to 255.
The interrupt unit is optional in the ARCv2-based processors. When
building a processor, you can configure the processor to include an
interrupt unit. The ARCv2 interrupt unit is highly programmable.


:Symbol:           ARCV2_INTERRUPT_UNIT
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "ARCv2 Interrupt Unit" if ARC (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: ARC (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/interrupt_controller/Kconfig:84