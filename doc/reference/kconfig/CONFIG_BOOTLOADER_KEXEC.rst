
.. _CONFIG_BOOTLOADER_KEXEC:

CONFIG_BOOTLOADER_KEXEC
#######################


This option signifies that Linux boots the kernel using kexec system call
and utility. This method is used to boot the kernel over the network.



:Symbol:           BOOTLOADER_KEXEC
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Boot using Linux kexec() system call" if X86_32 (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: X86_32 (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:151