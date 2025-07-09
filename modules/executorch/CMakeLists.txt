# Copyright (c) 2025 Petri Oksanen
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_EXECUTORCH)

# AGGRESSIVE WARNING SUPPRESSION: Apply immediately for ARM builds
if(CONFIG_EXECUTORCH_BACKEND_ARM)
	# Modify global CMAKE flags to ensure ALL ExecuTorch compilation gets these flags
	string(APPEND CMAKE_C_FLAGS " -Wno-double-promotion -Wno-float-conversion -Wno-conversion -Wno-deprecated-declarations")
	string(APPEND CMAKE_CXX_FLAGS " -Wno-double-promotion -Wno-float-conversion -Wno-conversion -Wno-deprecated-declarations")
	
	# Also set as cache variables to ensure they persist
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C flags with warning suppressions" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "CXX flags with warning suppressions" FORCE)
	
	# Force include our warning suppression header in every compilation unit
	string(APPEND CMAKE_CXX_FLAGS " -include ${CMAKE_CURRENT_SOURCE_DIR}/warning_suppressions.h")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "CXX flags with forced include" FORCE)
	
	# Add compile definitions to help with warning suppression
	add_compile_definitions(
		__STDC_NO_ATOMICS__=1
		_LIBCPP_DISABLE_AVAILABILITY=1
	)
	
	message(STATUS "Applied aggressive warning suppression to CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
endif()

# These samples use local static initialization. Since Zephyr doesn't support the
# C++ ABI for thread-safe initialization of local statics and the constructors don't
# appear to require thread safety, we turn it off in the C++ compiler.
set(NO_THREADSAFE_STATICS $<TARGET_PROPERTY:compiler-cpp,no_threadsafe_statics>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${NO_THREADSAFE_STATICS}>)

set(EXECUTORCH_DIR ${ZEPHYR_CURRENT_MODULE_DIR})
message(STATUS "EXECUTORCH_DIR set to: ${EXECUTORCH_DIR}")
include(${EXECUTORCH_DIR}/tools/cmake/common/preset.cmake)
include(${EXECUTORCH_DIR}/tools/cmake/preset/zephyr.cmake)

# HACK: Force the parent directory of executorch into the global include path.
# This is needed because the executorch CMake scripts generate a relative
# include path ("../") that doesn't resolve correctly in a Zephyr out-of-tree
# module build.
get_filename_component(EXECUTORCH_PARENT_DIR ${EXECUTORCH_DIR} DIRECTORY)
include_directories(${EXECUTORCH_PARENT_DIR})

# Note: We don't create executorch_pal as a separate library to avoid circular 
# dependencies with Zephyr's build system. Instead, we'll add the PAL source 
# directly to our final library.

set(EXECUTORCH_BUILD_EXECUTORCH_PYBIND OFF)
set(EXECUTORCH_BUILD_GENERATED_SRC OFF)
set(EXECUTORCH_BUILD_SDK OFF)
#set(EXECUTORCH_BUILD_TESTS OFF)
# Use Kconfig to control portable ops building
if(CONFIG_EXECUTORCH_BUILD_PORTABLE_OPS)
	set(EXECUTORCH_BUILD_PORTABLE_OPS ON)
else()
	set(EXECUTORCH_BUILD_PORTABLE_OPS OFF)
endif()
#set(EXECUTORCH_BUILD_PTHREADPOOL ON)
#set(EXECUTORCH_BUILD_CPUINFO OFF)

if(CONFIG_EXECUTORCH_BUILD_EXECUTOR_RUNNER)
	set(EXECUTORCH_BUILD_EXECUTOR_RUNNER ON)
else()
	set(EXECUTORCH_BUILD_EXECUTOR_RUNNER OFF)
endif()

if(CONFIG_EXECUTORCH_BUILD_DATA_LOADER)
	set(EXECUTORCH_BUILD_EXTENSION_DATA_LOADER ON)
else()
	set(EXECUTORCH_BUILD_EXTENSION_DATA_LOADER OFF)
endif()

if(CONFIG_EXECUTORCH_BACKEND_ARM)
	set(EXECUTORCH_BUILD_ARM_BAREMETAL ON)
	set(EXECUTORCH_BUILD_CORTEX_M ON)
	set(EXECUTORCH_BUILD_XNNPACK OFF)
elseif(CONFIG_EXECUTORCH_BACKEND_XNNPACK)
	set(EXECUTORCH_BUILD_ARM_BAREMETAL OFF)
	set(EXECUTORCH_BUILD_CORTEX_M OFF)
	set(EXECUTORCH_BUILD_XNNPACK ON)
else()
	set(EXECUTORCH_BUILD_ARM_BAREMETAL OFF)
	set(EXECUTORCH_BUILD_CORTEX_M OFF)
	set(EXECUTORCH_BUILD_XNNPACK OFF)
endif()

# Suppress noisy ABI warnings from GCC that are not relevant when building
# from source.
add_compile_options(-Wno-psabi)
add_compile_options(-Wno-double-promotion)
add_compile_options(-Wno-float-conversion)
add_compile_options(-Wno-sign-conversion)
# Additional suppressions for ExecuTorch template and math libraries
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Wno-unused-variable)
# Suppress warnings that commonly occur in template-heavy C++ code like ExecuTorch
add_compile_options(-Wno-conversion)
add_compile_options(-Wno-shadow)

# Nuclear option: Add aggressive warning suppression for the entire module
if(CONFIG_EXECUTORCH_BACKEND_ARM)
	# Force these flags into every compilation unit
	add_compile_options(-Wno-double-promotion)
	add_compile_options(-Wno-float-conversion)
	add_compile_options(-Wno-conversion)
endif()

set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE} PARENT_SCOPE)

if(CONFIG_EXECUTORCH_ENABLE_LOGGING)
	set(EXECUTORCH_ENABLE_LOGGING ON)
else()
	set(EXECUTORCH_ENABLE_LOGGING OFF)
endif()

if(CONFIG_EXECUTORCH_ENABLE_EVENT_TRACER)
	set(EXECUTORCH_ENABLE_EVENT_TRACER ON)
else()
	set(EXECUTORCH_ENABLE_EVENT_TRACER OFF)
endif()

if(CONFIG_EXECUTORCH_ENABLE_PROGRAM_VERIFICATION)
	set(EXECUTORCH_ENABLE_PROGRAM_VERIFICATION ON)
else()
	set(EXECUTORCH_ENABLE_PROGRAM_VERIFICATION OFF)
endif()

if(CONFIG_EXECUTORCH_OPTIMIZE_FOR_SIZE)
	set(EXECUTORCH_OPTIMIZE_SIZE ON)
else()
	set(EXECUTORCH_OPTIMIZE_SIZE OFF)
endif()

# PRE-OVERRIDE: Set the _common_compile_options variable BEFORE ExecuTorch 
# processes it, ensuring ARM builds get the correct flags from the start.
if(CONFIG_EXECUTORCH_BACKEND_ARM)
	# Get Zephyr's compiler flags that include the correct ARM architecture settings
	get_property(ZEPHYR_COMPILE_OPTIONS TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)
	
	# Also add warning suppressions to global CMAKE_CXX_FLAGS to ensure they're applied everywhere
	string(APPEND CMAKE_CXX_FLAGS " -Wno-double-promotion -Wno-float-conversion -Wno-conversion -Wno-sign-conversion")
	string(APPEND CMAKE_CXX_FLAGS " -include ${CMAKE_CURRENT_SOURCE_DIR}/warning_suppressions.h")
	
	# Override with ARM-compatible flags (no -fPIC) and comprehensive warning suppressions
	set(_common_compile_options 
		${ZEPHYR_COMPILE_OPTIONS}
		-Wno-deprecated-declarations 
		-Wno-psabi
		-Wno-double-promotion
		-Wno-float-conversion
		-Wno-sign-conversion
		-Wno-unused-parameter
		-Wno-unused-variable
		-Wno-conversion
		-Wno-shadow
		-include ${CMAKE_CURRENT_SOURCE_DIR}/warning_suppressions.h
		-fno-PIC
		CACHE INTERNAL "ARM-compatible compile options for ExecuTorch" FORCE
	)
	message(STATUS "Pre-overriding _common_compile_options for ARM build")
endif()

add_subdirectory(
	${EXECUTORCH_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/executorch
)

# Apply warning suppressions to all ExecuTorch targets after they're created
if(CONFIG_EXECUTORCH_BACKEND_ARM)
	# Get all targets created in the ExecuTorch directory
	get_property(ET_TARGETS DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/executorch PROPERTY BUILDSYSTEM_TARGETS)
	foreach(target ${ET_TARGETS})
		if(TARGET ${target})
			target_compile_options(${target} PRIVATE 
				-Wno-deprecated-declarations
				-Wno-double-promotion
				-Wno-float-conversion
				-Wno-conversion
				-Wno-sign-conversion
			)
		endif()
	endforeach()
endif()

# If portable ops are disabled, we need to manually build just the portable 
# kernels (not the ops library) to support selective operator builds
if(NOT CONFIG_EXECUTORCH_BUILD_PORTABLE_OPS)
	message(STATUS "Building portable kernels separately from ops library")
	
	# First ensure the kernels utility library is available
	if(NOT TARGET kernels_util_all_deps)
		add_subdirectory(
			${EXECUTORCH_DIR}/kernels/portable/cpu/util
			${CMAKE_CURRENT_BINARY_DIR}/executorch/kernels/portable/cpu/util
		)
	endif()
	
	# Manually create portable_kernels target without the ops library
	file(GLOB_RECURSE _portable_kernels_srcs
		"${EXECUTORCH_DIR}/kernels/portable/cpu/*.cpp"
	)
	list(FILTER _portable_kernels_srcs EXCLUDE REGEX "test/*.cpp")
	list(FILTER _portable_kernels_srcs EXCLUDE REGEX "codegen")
	
	add_library(portable_kernels ${_portable_kernels_srcs})
	target_link_libraries(portable_kernels PRIVATE executorch_core kernels_util_all_deps)
	target_compile_options(portable_kernels PUBLIC -Wno-deprecated-declarations)
	
	# Apply ARM-compatible compile options if building for ARM
	if(CONFIG_EXECUTORCH_BACKEND_ARM)
		target_compile_options(portable_kernels PUBLIC ${CORRECTED_COMPILE_OPTIONS})
	endif()
endif()

# Apply the most aggressive possible warning suppression to ExecuTorch directory
if(CONFIG_EXECUTORCH_BACKEND_ARM)
	# Set properties on the ExecuTorch source directory itself
	set_property(DIRECTORY ${EXECUTORCH_DIR} 
		PROPERTY COMPILE_OPTIONS 
		"-Wno-double-promotion;-Wno-float-conversion;-Wno-conversion;-Wno-sign-conversion"
	)
	
	# Also try setting it on the binary directory
	set_property(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/executorch 
		PROPERTY COMPILE_OPTIONS 
		"-Wno-double-promotion;-Wno-float-conversion;-Wno-conversion;-Wno-sign-conversion"
	)
	
	# Force set the CMAKE_CXX_FLAGS for the ExecuTorch subdirectory
	set_property(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/executorch 
		PROPERTY CMAKE_CXX_FLAGS 
		"${CMAKE_CXX_FLAGS} -Wno-double-promotion -Wno-float-conversion -Wno-conversion"
	)
endif()

# Apply global warning suppressions to the entire ExecuTorch build directory
# This catches any targets that might not be caught by individual target fixes
set_property(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/executorch 
	PROPERTY COMPILE_OPTIONS 
	"-Wno-double-promotion;-Wno-float-conversion;-Wno-conversion;-Wno-sign-conversion;-Wno-psabi;-Wno-deprecated-declarations"
)

# Apply warning suppressions to the source directory as well
set_property(DIRECTORY ${EXECUTORCH_DIR} 
	PROPERTY COMPILE_OPTIONS 
	"-Wno-double-promotion;-Wno-float-conversion;-Wno-conversion;-Wno-sign-conversion;-Wno-psabi;-Wno-deprecated-declarations"
)

# CRITICAL FIX: Override the hardcoded -fPIC flags that ExecuTorch applies to all targets.
# ExecuTorch hardcodes "_common_compile_options" with -fPIC, which is incompatible with
# ARM bare-metal builds that require -fno-PIC and specific architecture flags.
if(CONFIG_EXECUTORCH_BACKEND_ARM)
	# Get Zephyr's compiler flags that include the correct ARM architecture settings
	get_property(ZEPHYR_COMPILE_OPTIONS TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)
	
	# Create the corrected compile options without -fPIC
	set(CORRECTED_COMPILE_OPTIONS 
		${ZEPHYR_COMPILE_OPTIONS}
		-Wno-deprecated-declarations 
		-Wno-psabi
		-Wno-double-promotion
		-Wno-float-conversion
		-Wno-sign-conversion
		-Wno-unused-parameter
		-Wno-unused-variable
		-Wno-conversion
		-Wno-shadow
		-fno-PIC
	)
	
	# Function to aggressively clean and fix target compile options
	function(fix_executorch_target_flags target_name)
		if(TARGET ${target_name})
			# Check if this is an INTERFACE target
			get_target_property(TARGET_TYPE ${target_name} TYPE)
			
			if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
				# For INTERFACE targets, only modify INTERFACE properties
				get_target_property(CURRENT_INTERFACE_COMPILE_OPTIONS ${target_name} INTERFACE_COMPILE_OPTIONS)
				
				if(CURRENT_INTERFACE_COMPILE_OPTIONS)
					list(REMOVE_ITEM CURRENT_INTERFACE_COMPILE_OPTIONS "-fPIC")
					set_target_properties(${target_name} PROPERTIES INTERFACE_COMPILE_OPTIONS "${CURRENT_INTERFACE_COMPILE_OPTIONS}")
				endif()
				
				# Apply corrected options as INTERFACE
				target_compile_options(${target_name} INTERFACE ${CORRECTED_COMPILE_OPTIONS})
				message(STATUS "Fixed INTERFACE compile flags for target: ${target_name}")
			else()
				# For regular targets, handle both regular and interface properties
				get_target_property(CURRENT_COMPILE_OPTIONS ${target_name} COMPILE_OPTIONS)
				get_target_property(CURRENT_INTERFACE_COMPILE_OPTIONS ${target_name} INTERFACE_COMPILE_OPTIONS)
				
				# Clean options by removing -fPIC
				if(CURRENT_COMPILE_OPTIONS)
					list(REMOVE_ITEM CURRENT_COMPILE_OPTIONS "-fPIC")
					set_target_properties(${target_name} PROPERTIES COMPILE_OPTIONS "${CURRENT_COMPILE_OPTIONS}")
				endif()
				
				if(CURRENT_INTERFACE_COMPILE_OPTIONS)
					list(REMOVE_ITEM CURRENT_INTERFACE_COMPILE_OPTIONS "-fPIC")
					set_target_properties(${target_name} PROPERTIES INTERFACE_COMPILE_OPTIONS "${CURRENT_INTERFACE_COMPILE_OPTIONS}")
				endif()
				
				# Apply our corrected options
				target_compile_options(${target_name} PUBLIC ${CORRECTED_COMPILE_OPTIONS})
				message(STATUS "Fixed compile flags for target: ${target_name}")
			endif()
		endif()
	endfunction()
	
	# List of all ExecuTorch targets that need flag correction
	set(EXECUTORCH_TARGETS_TO_FIX
		executorch_core
		executorch
		portable_ops_lib
		portable_kernels
		optimized_portable_kernels
		quantized_kernels
		extension_data_loader
		extension_runner_util
		extension_tensor
		extension_flat_tensor
		extension_threadpool
		program_schema
		kernels_util_all_deps
		cpublas
		optimized_kernels
		cortex_m_ops_lib
		cortex_m_kernels
	)
	
	# Apply corrected flags to each target
	foreach(target IN LISTS EXECUTORCH_TARGETS_TO_FIX)
		if(TARGET ${target})
			fix_executorch_target_flags(${target})
		else()
			message(STATUS "Target not found in explicit list: ${target}")
		endif()
	endforeach()
	
	# Additional comprehensive approach: Find all targets created by ExecuTorch
	# and apply warning suppressions to any we might have missed
	get_property(ALL_TARGETS DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/executorch PROPERTY BUILDSYSTEM_TARGETS)
	message(STATUS "Found ExecuTorch targets: ${ALL_TARGETS}")
	foreach(target IN LISTS ALL_TARGETS)
		if(TARGET ${target})
			# Get current compile options
			get_target_property(CURRENT_OPTIONS ${target} COMPILE_OPTIONS)
			get_target_property(CURRENT_INTERFACE_OPTIONS ${target} INTERFACE_COMPILE_OPTIONS)
			
			# Always apply warning suppressions to all ExecuTorch targets
			# Check if this target is already in our explicit list
			list(FIND EXECUTORCH_TARGETS_TO_FIX ${target} TARGET_IN_LIST)
			if(TARGET_IN_LIST EQUAL -1)
				message(STATUS "Auto-fixing additional ExecuTorch target: ${target}")
				fix_executorch_target_flags(${target})
			endif()
		endif()
	endforeach()
	
	# Force-fix specific problematic targets that might be in subdirectories
	set(ADDITIONAL_TARGETS_TO_CHECK
		kernels_util_all_deps
		portable_kernels  
		optimized_kernels
		cpublas
		cortex_m_ops_lib
		cortex_m_kernels
		portable_ops_lib
	)
	
	foreach(target IN LISTS ADDITIONAL_TARGETS_TO_CHECK)
		if(TARGET ${target})
			message(STATUS "Force-fixing known problematic target: ${target}")
			fix_executorch_target_flags(${target})
		endif()
	endforeach()
endif()

# Create a single Zephyr library that includes ExecuTorch integration and PAL
zephyr_library_named(libexecutorch)

# Link ExecuTorch libraries
target_link_libraries(libexecutorch PUBLIC
	executorch_core
)

# Only link portable_ops_lib if it was built (i.e., if portable ops are enabled)
if(TARGET portable_ops_lib)
	target_link_libraries(libexecutorch PUBLIC portable_ops_lib)
	message(STATUS "Linking with default portable_ops_lib")
else()
	message(STATUS "portable_ops_lib not available - using selective operators from application")
endif()

target_include_directories(libexecutorch PUBLIC
	${EXECUTORCH_DIR}/runtime
	${EXECUTORCH_DIR}/third-party/flatcc/include
)

endif() 
