
.. _CONFIG_EOI_HANDLER_SUPPORTED:

CONFIG_EOI_HANDLER_SUPPORTED
############################


This option signifies that the target has one or more devices whose
driver utilizes an "end of interrupt" handler that gets called
after the standard interrupt handling code. This capability
can be used by the driver to tell the device that an interrupt
has been handled (or for other purposes).



:Symbol:           EOI_HANDLER_SUPPORTED
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "EOI Handler Supported"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 PLATFORM_IA32_PCI || PLATFORM_IA32 (value: "y")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:121