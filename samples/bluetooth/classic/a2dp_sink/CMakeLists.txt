# Minimum CMake version required by Zephyr (match your Zephyr version, e.g., 3.20+)
cmake_minimum_required(VERSION 3.20.0)

# Import Zephyr's build system (mandatory for all Zephyr samples)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Set project name (match your sample for consistency)
project(classic_bt_a2dp_avrcp_sink)

# -------------------------- Add Source Files --------------------------
# List all C source files (only main.c in this sample)
set(SRC_FILES
  src/main.c
)

# Add sources to the Zephyr application library (links to firmware)
target_sources(app PRIVATE ${SRC_FILES})

# -------------------------- Enforce Mandatory Configs (Standard Zephyr Way) --------------------------
# Check and enforce critical configs (fail build if missing to avoid runtime issues)
# 1. Bluetooth Core (Classic BT required)
if(NOT CONFIG_BT)
  message(FATAL_ERROR "CONFIG_BT must be enabled (base Bluetooth stack is mandatory)")
endif()

if(NOT CONFIG_BT_CLASSIC)
  message(FATAL_ERROR "CONFIG_BT_CLASSIC must be enabled (Classic Bluetooth required for A2DP)")
endif()

# 2. A2DP & SBC (mandatory for audio streaming)
if(NOT CONFIG_BT_A2DP)
  message(FATAL_ERROR "CONFIG_BT_A2DP must be enabled (A2DP protocol is mandatory)")
endif()

if(NOT CONFIG_BT_A2DP_SINK)
  message(FATAL_ERROR "CONFIG_BT_A2DP_SINK must be enabled (A2DP Sink role is required)")
endif()

# 3. AVRCP (required for remote control)
if(NOT CONFIG_BT_AVRCP)
  message(FATAL_ERROR "CONFIG_BT_AVRCP must be enabled (AVRCP protocol is mandatory)")
endif()

if(NOT CONFIG_BT_AVRCP_TARGET)
  message(FATAL_ERROR "CONFIG_BT_AVRCP_TARGET must be enabled (AVRCP Target role is required)")
endif()
