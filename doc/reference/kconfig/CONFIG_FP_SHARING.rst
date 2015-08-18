
.. _CONFIG_FP_SHARING:

CONFIG_FP_SHARING
#################


This option allows multiple tasks and fibers to safely utilize
floating point hardware resources. Every task or fiber that wishes
to use floating point instructions (i.e. x87 FPU, MMX, or SSEx) must
be created with such support already enabled, or must enable this
support via task_float_enable() or fiber_float_enable() before
executing these instructions.

Enabling this option adds 108 bytes to the stack size requirement
of each task or fiber that utilizes x87 FPU or MMX instructions,
and adds 464 bytes to the stack size requirement of each task or
fiber that utilizes SSEx instructions. (The stack size requirement
of tasks and fibers that do not utilize floating point instructions
remains unchanged.)

Disabling this option means that only a single task or fiber may
utilize x87 FPU, MMX, or SSEx instructions. (The stack size
requirement of all tasks and fibers remains unchanged.)



:Symbol:           FP_SHARING
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Floating point resource sharing" if !ENHANCED_SECURITY && FLOAT (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: FLOAT (value: "n")
:Selects:

 *  AUTOMATIC_FP_ENABLING_ if ENHANCED_SECURITY && FLOAT (value: "n")
:Reverse (select-related) dependencies:
 ENHANCED_SECURITY && !CPU_FLOAT_UNSUPPORTED && FLOAT (value: "n")
:Additional dependencies from enclosing menus and ifs:
 !CPU_FLOAT_UNSUPPORTED (value: "n")
:Locations:
 * arch/x86/Kconfig:216