# Bluetooth GATT Elapsed Time Service

# Copyright (c) 2025 Dipak Shetty
# SPDX-License-Identifier: Apache-2.0

menuconfig BT_ETS
	bool "GATT Elapsed Time service [EXPERIMENTAL]"
	select EXPERIMENTAL
	help
	  Enable Bluetooth Elapsed Time Service (ETS).
	  This service provides a time service for embedded devices that either:
	  - Keep track of absolute time (time of day, UTC or local)
	  - Maintain a tick counter for relative time measurements

if BT_ETS

#
# Time Mode Selection
#
choice BT_ETS_TIME_MODE
	prompt "Time mode"
	default BT_ETS_SUPPORT_TIME_OF_DAY
	help
	  Select the time representation mode (bit 0 of flags field):

	  Time of Day: Reports absolute time since 2000-01-01 00:00:00.
	  Use this for devices with real-time clocks or time synchronization.

	  Tick Counter: Reports relative time (elapsed ticks/counter).
	  Use this for simple event timing without absolute time reference.
	  The counter starting point is implementation-dependent.

config BT_ETS_SUPPORT_TIME_OF_DAY
	bool "Time of day (absolute time)"
	help
	  Report absolute time since ETS epoch (2000-01-01 00:00:00).
	  Time Value represents the number of time units (based on resolution)
	  since the epoch. Supports UTC or local time with timezone offset.

config BT_ETS_SUPPORT_TICK_COUNTER
	bool "Tick counter (relative time)"
	help
	  Report relative time using a counter/tick value.
	  Time Value represents elapsed time units (based on resolution).
	  The counter starting point is implementation-dependent.
	  Timezone and UTC/local settings are not applicable in this mode.

endchoice

#
# Time of Day Configuration
#
if BT_ETS_SUPPORT_TIME_OF_DAY

choice BT_ETS_TIME_TYPE
	prompt "Time reference"
	default BT_ETS_SUPPORT_LOCAL_TIME
	help
	  Select the time reference type (bit 1 of flags field):

	  Local Time (bit 1 = 0): Time Value is in local time (already adjusted).
	  No timezone offset needed as time is already local.

	  UTC (bit 1 = 1): Time Value is in UTC (Coordinated Universal Time).
	  Optionally include timezone offset so clients can calculate local time.

config BT_ETS_SUPPORT_LOCAL_TIME
	bool "Local time"
	help
	  Time Value represents local time (already adjusted for timezone/DST).
	  Bit 1 = 0 (local time)

	  You can optionally enable TZ/DST offset below to indicate which
	  timezone the local time represents. This allows recipients to
	  convert the local time back to UTC using the formula:
	    UTC time = Local time - TZ/DST Offset × 15 minutes

config BT_ETS_SUPPORT_UTC
	bool "UTC (Coordinated Universal Time)"
	help
	  Time Value represents UTC time.
	  Bit 1 = 1 (UTC)

	  You can optionally enable TZ/DST offset below to allow clients
	  to calculate local time using the formula:
	    Local time = UTC time + TZ/DST Offset × 15 minutes

endchoice

config BT_ETS_SUPPORT_TZ_DST
	bool "Include timezone/DST offset"
	depends on BT_ETS_SUPPORT_TIME_OF_DAY
	help
	  Include the TZ/DST offset field in the characteristic (bit 4 = 1).
	  The offset is specified in 15-minute increments from UTC.

	  The offset can be used in both UTC and local time modes:
	  - UTC mode: Offset is informational for clients to calculate local time
	    Formula: Local time = UTC time + TZ/DST Offset × 15 minutes
	  - Local mode: Offset indicates which timezone the local time represents
	    Formula: UTC time = Local time - TZ/DST Offset × 15 minutes

config BT_ETS_HELPER_API
	bool "Helper API for Unix time conversion"
	depends on BT_ETS_SUPPORT_UTC || BT_ETS_SUPPORT_TZ_DST
	help
	  Enable helper functions to convert between ETS time format and
	  Unix milliseconds (epoch 1970-01-01 00:00:00 UTC):
	  - bt_ets_time_to_unix_ms(): ETS format -> Unix milliseconds (UTC)
	  - bt_ets_time_from_unix_ms(): Unix milliseconds (UTC) -> ETS format

	  These functions handle epoch conversion (2000 vs 1970),
	  resolution scaling, and timezone conversion.

	  Requirements:
	  - UTC mode: Direct conversion, no timezone offset needed
	  - Local mode: Requires TZ/DST offset to convert between UTC and local time

	  This option requires either UTC mode or TZ/DST offset support.

endif # BT_ETS_SUPPORT_TIME_OF_DAY

#
# Time Resolution (applies to both modes)
#
choice BT_ETS_TIME_RESOLUTION
	prompt "Time resolution"
	default BT_ETS_RESOLUTION_1_SEC
	help
	  Time resolution for the Time Value field (bits 2-3 of flags).
	  Determines how frequently the time value increments.
	  Higher resolution provides finer granularity but reduces maximum time range.

config BT_ETS_RESOLUTION_1_SEC
	bool "1 second"
	help
	  Time value increments every 1 second.
	  Maximum range: ~8925512 years.

config BT_ETS_RESOLUTION_100_MS
	bool "100 milliseconds"
	help
	  Time value increments every 100 milliseconds (0.1 second).
	  Maximum range: ~892551 years.

config BT_ETS_RESOLUTION_1_MS
	bool "1 millisecond"
	help
	  Time value increments every 1 millisecond (0.001 second).
	  Maximum range: ~8,920 years.

config BT_ETS_RESOLUTION_100_US
	bool "100 microseconds"
	help
	  Time value increments every 100 microseconds (0.0001 second).
	  Maximum range: ~892 years.

endchoice

#
# Optional Features
#
config BT_ETS_CURRENT_ELAPSED_TIME_WRITABLE
	bool "Allow clients to write time"
	help
	  Enable write permission on the Current Elapsed Time characteristic.
	  When enabled:
	  - Clients can set/adjust the time by writing to the characteristic
	  - Application must implement write_elapsed_time callback
	  - Written value is validated against supported flags
	  - Indication is sent to other clients after successful write
	  - "Current timeline" bit is set to indicate new time reference

	  Use this if clients need to synchronize the device's time.

#
# Clock Capabilities (static, compile-time)
#
config BT_ETS_CLOCK_CAP_DST_AUTO
	bool "Clock autonomously applies DST rules"
	depends on BT_ETS_SUPPORT_TIME_OF_DAY
	help
	  Enable if the clock autonomously applies Daylight Saving Time rules.
	  When enabled, bit 0 of Clock Capabilities will be set.
	  This indicates the device automatically adjusts for DST transitions
	  without external intervention.

config BT_ETS_CLOCK_CAP_TZ_AUTO
	bool "Clock autonomously manages timezone changes"
	depends on BT_ETS_SUPPORT_TIME_OF_DAY
	help
	  Enable if the clock autonomously manages timezone changes.
	  When enabled, bit 1 of Clock Capabilities will be set.
	  This indicates the device automatically adjusts for timezone changes
	  (e.g., when traveling) without external intervention.

module = BT_ETS
module-str = ets
source "subsys/logging/Kconfig.template.log_config"

endif # BT_ETS
