# Copyright (c) 2024 Tenstorrent
#
# SPDX-License-Identifier: Apache-2.0

config POSIX_SYSTEM_INTERFACES
	bool
	select NATIVE_LIBC_INCOMPATIBLE
	help
	  Make POSIX headers available to the system without the "zephyr/posix" prefix.

config POSIX_API
	bool "POSIX APIs"
	select POSIX_SYSTEM_INTERFACES
	select POSIX_BASE_DEFINITIONS # clock_gettime(), pthread_create(), sem_get(), etc
	select POSIX_AEP_REALTIME_MINIMAL # CLOCK_MONOTONIC, pthread_attr_setstack(), etc
	select POSIX_NETWORKING if NETWORKING # inet_ntoa(), socket(), etc
	imply EVENTFD # eventfd(), eventfd_read(), eventfd_write()
	imply POSIX_FD_MGMT # open(), close(), read(), write()
	imply POSIX_MULTI_PROCESS # sleep(), getpid(), etc
	imply XSI_SINGLE_PROCESS # gettimeofday()
	help
	  This option enables the required POSIX System Interfaces (base definitions), all of PSE51,
	  and some features found in PSE52.

	  Note: in the future, this option may be deprecated in favour of subprofiling options.

choice POSIX_AEP_CHOICE
	prompt "POSIX Subprofile"
	default POSIX_AEP_CHOICE_ZEPHYR
	help
	  This choice is intended to help users select the correct POSIX profile for their
	  application. Choices are based on IEEE 1003.13-2003 (now inactive / reserved) and
	  extrapolated to the more recent Subprofiling Option Groups in IEEE 1003.3-2017.

	  For more information, please refer to
	  https://standards.ieee.org/ieee/1003.13/3322/
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

config POSIX_AEP_CHOICE_NONE
	bool "No POSIX subprofile"
	help
	  No POSIX subprofile is selected.

config POSIX_AEP_CHOICE_ZEPHYR
	bool "Minimal Zephyr System Profile"
	select POSIX_C_LIB_EXT
	select POSIX_C_LANG_SUPPORT_R
	help
	  Zephyr expects certain POSIX functions to be available throughout the build environment,
	  such as gmtime_r(), strnlen(), strtok_r(), and possibly others.

	  These functions are divided into two standalone Option Groups that may be enabled
	  independently of the remainder of the POSIX API implementation; namely POSIX_C_LIB_EXT and
	  POSIX_C_LANG_SUPPORT_R. If not referenced by the Zephyr kernel or application, there are no
	  resource implications for enabling these option groups.

	  Unlike pre-defined, standard POSIX subprofiles, this subprofile is custom to Zephyr and
	  therefore does not need to include the base definitions or system interfaces that would
	  otherwise be required for a conformant POSIX system or subprofile. This system profile
	  does not itself meet the requirements for POSIX implementation conformance.

	  For more information, see
	  https://docs.zephyrproject.org/latest/contribute/coding_guidelines/index.html
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

config POSIX_AEP_CHOICE_BASE
	bool "Minimal POSIX System Profile"
	select POSIX_SYSTEM_INTERFACES
	select POSIX_BASE_DEFINITIONS
	help
	  Only enable the base definitions required for all POSIX systems.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap02.html#tag_02_01_03_01

config POSIX_AEP_CHOICE_PSE51
	bool "Minimal Realtime System Profile (PSE51)"
	select POSIX_SYSTEM_INTERFACES
	select POSIX_BASE_DEFINITIONS
	select POSIX_AEP_REALTIME_MINIMAL
	help
	  PSE51 includes the POSIX Base Definitions (System Interfaces) as well as several Options and
	  Option Groups to facilitate device I/O, signals, mandatory configuration utilities, and
	  threading.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

config POSIX_AEP_CHOICE_PSE52
	bool "Realtime Controller System Profile (PSE52)"
	select POSIX_SYSTEM_INTERFACES
	select POSIX_BASE_DEFINITIONS
	select POSIX_AEP_REALTIME_MINIMAL
	select POSIX_AEP_REALTIME_CONTROLLER
	help
	  PSE52 includes the POSIX Base Definitions (System Interfaces) as well as all features of
	  PSE51. Additionally, it includes interfaces for file descriptor management, filesystem
	  support, support for message queues, and tracing.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

config POSIX_AEP_CHOICE_PSE53
	bool "Dedicated Realtime System Profile (PSE53)"
	select POSIX_SYSTEM_INTERFACES
	select POSIX_BASE_DEFINITIONS
	select POSIX_AEP_REALTIME_MINIMAL
	select POSIX_AEP_REALTIME_CONTROLLER
	select POSIX_AEP_REALTIME_DEDICATED
	help
	  PSE53 includes the POSIX Base Definitions (System Interfaces) as well as all features of
	  PSE52. Additionally, it includes interfaces for POSIX multi-processing, networking, pipes,
	  and prioritized I/O.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

# TODO: PSE54: Multi-purpose Realtime System Profile

endchoice # POSIX_AEP_CHOICE

if POSIX_SYSTEM_INTERFACES

# Mandatory POSIX System Interfaces (base profile)
config POSIX_BASE_DEFINITIONS
	bool
	select POSIX_ASYNCHRONOUS_IO
	select POSIX_BARRIERS
	select POSIX_CLOCK_SELECTION
	select POSIX_MAPPED_FILES
	select POSIX_MEMORY_PROTECTION
	select POSIX_RW_LOCKS
	select POSIX_REALTIME_SIGNALS
	select POSIX_SEMAPHORES
	select POSIX_SPIN_LOCKS
	select POSIX_THREAD_SAFE_FUNCTIONS
	select POSIX_THREADS
	select POSIX_TIMEOUTS
	select POSIX_TIMERS
	help
	  This option is not user configurable. It may be configured indirectly by selecting
	  CONFIG_POSIX_AEP_CHOICE_BASE=y.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap02.html#tag_02_01_03_01

config POSIX_AEP_REALTIME_MINIMAL
	bool
	# Option Groups
	select POSIX_DEVICE_IO
	select POSIX_SIGNALS
	select POSIX_SINGLE_PROCESS
	select XSI_THREADS_EXT
	# Options
	select POSIX_FSYNC
	select POSIX_MEMLOCK
	select POSIX_MEMLOCK_RANGE
	select POSIX_MONOTONIC_CLOCK
	select POSIX_SHARED_MEMORY_OBJECTS
	select POSIX_SYNCHRONIZED_IO
	select POSIX_THREAD_ATTR_STACKADDR
	select POSIX_THREAD_ATTR_STACKSIZE
	select POSIX_THREAD_CPUTIME
	select POSIX_THREAD_PRIO_INHERIT
	select POSIX_THREAD_PRIO_PROTECT
	select POSIX_THREAD_PRIORITY_SCHEDULING
	# select POSIX_THREAD_SPORADIC_SERVER
	help
	  This option is not user configurable. It may be configured indirectly by selecting
	  CONFIG_POSIX_AEP_CHOICE_PSE51=y.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

config POSIX_AEP_REALTIME_CONTROLLER
	bool
	# Option Groups
	select POSIX_FD_MGMT
	select POSIX_FILE_SYSTEM
	# Options
	select POSIX_MESSAGE_PASSING
	# select POSIX_TRACE
	# select POSIX_TRACE_EVENT_FILTER
	# select POSIX_TRACE_LOG
	help
	  This option is not user configurable. It may be configured indirectly by selecting
	  CONFIG_POSIX_AEP_CHOICE_PSE52=y.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

config POSIX_AEP_REALTIME_DEDICATED
	bool
	# Option Groups
	select POSIX_MULTI_PROCESS
	select POSIX_NETWORKING
	# select POSIX_PIPE
	# select POSIX_SIGNAL_JUMP
	# Options
	select POSIX_CPUTIME
	# select POSIX_PRIORITIZED_IO
	select POSIX_PRIORITY_SCHEDULING
	select POSIX_RAW_SOCKETS
	# select POSIX_SPAWN
	# select POSIX_SPORADIC_SERVER
	help
	  This option is not user configurable. It may be configured indirectly by selecting
	  CONFIG_POSIX_AEP_CHOICE_PSE53=y.

	  For more information, please see
	  https://pubs.opengroup.org/onlinepubs/9699919799/xrat/V4_subprofiles.html

endif # POSIX_SYSTEM_INTERFACE
