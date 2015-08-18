
.. _CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC:

CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
##################################


This option specifies the frequency of the hardware timer used for the
system clock (in Hz). It is normally set by the platform's defconfig file
and the user should generally avoid modifying it via the menu configuration.



:Symbol:           SYS_CLOCK_HW_CYCLES_PER_SEC
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "System clock's h/w timer frequency"
:Default values:

 *  0 (value: "n")
 *   Condition: (none)
 *  150000000 (value: "n")
 *   Condition: LOAPIC_TIMER (value: "n")
 *  25000000 (value: "n")
 *   Condition: HPET_TIMER (value: "y")
 *  150000000 (value: "n")
 *   Condition: LOAPIC_TIMER (value: "n")
 *  25000000 (value: "n")
 *   Condition: HPET_TIMER (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 PLATFORM_IA32_PCI (value: "y")
:Locations:
 * kernel/Kconfig:55
 * arch/x86/platforms/ia32/Kconfig:43
 * arch/x86/platforms/ia32_pci/Kconfig:44