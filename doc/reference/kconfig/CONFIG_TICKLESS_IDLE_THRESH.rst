
.. _CONFIG_TICKLESS_IDLE_THRESH:

CONFIG_TICKLESS_IDLE_THRESH
###########################


This option disables clock interrupt suppression when the kernel idles
for only a short period of time. It specifies the minimum number of
ticks that must occur before the next kernel timer expires in order
for suppression to happen.



:Symbol:           TICKLESS_IDLE_THRESH
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Tickless idle threshold" if TICKLESS_IDLE && MICROKERNEL (value: "n")
:Default values:

 *  3 (value: "n")
 *   Condition: TICKLESS_IDLE && MICROKERNEL (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:191