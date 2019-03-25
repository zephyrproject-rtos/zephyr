.. _mpu_stack_objects:

MPU Stack Objects
#################

Thread Stack Creation
*********************

Thread stacks are declared statically with :c:macro:`K_THREAD_STACK_DEFINE()`
or embedded within structures using :c:macro:`K_THREAD_STACK_MEMBER()`

For architectures which utilize memory protection unit (MPU) hardware,
stacks are physically contiguous allocations.  This contiguous allocation
has implications for the placement of stacks in memory, as well as the
implementation of other features such as stack protection and userspace.  The
implications for placement are directly attributed to the alignment
requirements for MPU regions.  This is discussed in the memory placement
section below.

Stack Guards
************

Stack protection mechanisms require hardware support that can restrict access
to memory.  Memory protection units can provide this kind of support.
The MPU provides a fixed number of regions.  Each region contains information
about the start, end, size, and access attributes to be enforced on that
particular region.

Stack guards are implemented by using a single MPU region and setting the
attributes for that region to not allow write access.  If invalid accesses
occur, a fault ensues.  The stack guard is defined at the bottom (the lowest
address) of the stack.

Memory Placement
****************

During stack creation, a set of constraints are enforced on the allocation of
memory.  These constraints include determining the alignment of the stack and
the correct sizing of the stack.  During linking of the binary, these
constraints are used to place the stacks properly.

The main source of the memory constraints is the MPU design for the SoC.  The
MPU design may require specific constraints on the region definition.  These
can include alignment of beginning and end addresses, sizes of allocations,
or even interactions between overlapping regions.

Some MPUs require that each region be aligned to a power of two.  These SoCs
will have :option:`CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT` defined.
This means that a 1500 byte stack should be aligned to a 2kB boundary and the
stack size should also be adjusted to 2kB to ensure that nothing else is
placed in the remainder of the region.  SoCs which include the unmodified ARM
v7m MPU will have these constraints.

Some ARM MPUs use start and end addresses to define MPU regions and both the
start and end addresses require 32 byte alignment.  An example of this kind of
MPU is found in the NXP FRDM K64F.

MPUs may have a region priority mechanisms that use the highest priority region
that covers the memory access to determine the enforcement policy.  Others may
logically OR regions to determine enforcement policy.

Size and alignment constraints may result in stack allocations being larger
than the requested size.  Region priority mechanisms may result in
some added complexity when implementing stack guards.

