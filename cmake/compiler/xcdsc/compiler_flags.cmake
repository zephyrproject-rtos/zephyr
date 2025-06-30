# Enable basic debug information
set_compiler_property(PROPERTY debug -g)
# Optimization for debug builds
set_compiler_property(PROPERTY optimization_debug -O1)
set_compiler_property(PROPERTY optimization_speed -O1)
# Optimize for size
set_compiler_property(PROPERTY optimization_size -O1)
set_compiler_property(PROPERTY optimization_size_aggressive -O1)
# Basic warnings
check_set_compiler_property(PROPERTY warning_base -Wall -Wformat -Wformat-security -Wno-format-zero-length -Wno-unused-but-set-variable )
# Compiler flags for imacros. The specific header must be appended by user.
set_compiler_property(PROPERTY imacros -imacros)
# Use the default C Language standard
set_compiler_property(PROPERTY cstd -std=)
# Linker script usage
set_compiler_property(PROPERTY linker_script -Wl,-T)
# Additional compiler flags for dspic(XC-DSC-Specific Flags).
# Save intermediate files (optional, helpful for inspection)
set_compiler_property(PROPERTY save_temps -save-temps=obj)
# Smart I/O conversion for format strings (safe printf optimizations)
list(APPEND TOOLCHAIN_C_FLAGS -msmart-io=1)
# Disable SFR access warnings
list(APPEND TOOLCHAIN_C_FLAGS -msfr-warn=off)
# Disable instruction scheduling (for stability)
list(APPEND TOOLCHAIN_C_FLAGS -fno-schedule-insns -fno-schedule-insns2)
list(APPEND TOOLCHAIN_C_FLAGS -fno-omit-frame-pointer)
# assembler compiler flags for imacros. The specific header must be appended by user.
set_property(TARGET asm PROPERTY imacros -imacros)