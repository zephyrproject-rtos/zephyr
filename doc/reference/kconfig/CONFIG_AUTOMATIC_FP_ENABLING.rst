
.. _CONFIG_AUTOMATIC_FP_ENABLING:

CONFIG_AUTOMATIC_FP_ENABLING
############################


This option allows tasks and fibers to safely utilize floating
point hardware resources without requiring them to first indicate
their intention to do so. The system automatically detects when
a task or fiber that does not currently have floating point support
enabled uses a floating point instruction, and automatically executes
task_float_enable() or fiber_float_enable() on its behalf. The
task or fiber is enabled for using x87 FPU, MMX, or SSEx instructions
if SSE is configured, otherwise it is enabled for using x87 FPU or
MMX instructions only.



:Symbol:           AUTOMATIC_FP_ENABLING
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Automatically enable floating point resource sharing" if !ENHANCED_SECURITY && FP_SHARING (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: FP_SHARING (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 ENHANCED_SECURITY && FLOAT && !CPU_FLOAT_UNSUPPORTED && FP_SHARING (value: "n")
:Additional dependencies from enclosing menus and ifs:
 !CPU_FLOAT_UNSUPPORTED (value: "n")
:Locations:
 * arch/x86/Kconfig:241