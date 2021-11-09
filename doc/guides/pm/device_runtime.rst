Device Runtime Power Management
###############################

The Device Runtime Power Management framework is an Active Power
Management mechanism which reduces the overall system Power consumtion
by suspending the devices which are idle or not being used while the
System is active or running.

The framework uses :c:func:`pm_device_state_set()` API set the
device power state accordingly based on the usage count.

The interfaces and APIs provided by the Device Runtime PM are
designed to be generic and architecture independent.
