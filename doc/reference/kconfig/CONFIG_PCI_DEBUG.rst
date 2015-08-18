
.. _CONFIG_PCI_DEBUG:

CONFIG_PCI_DEBUG
################


This options enables PCI debigging functions



:Symbol:           PCI_DEBUG
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable PCI debugging" if PCI (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: PCI (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 X86_32 (value: "y")
:Locations:
 * drivers/pci/Kconfig:12