
.. _CONFIG_BOI_HANDLER_SUPPORTED:

CONFIG_BOI_HANDLER_SUPPORTED
############################


This option signifies that the target has one or more devices whose
driver utilizes a "beginning of interrupt" handler that gets called
before the standard interrupt handling code. This capability
can be used by the driver to suppress spurious interrupts generated
by the device (or for other purposes).



:Symbol:           BOI_HANDLER_SUPPORTED
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "BOI Handler Supported"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:111