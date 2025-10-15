set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})
# Find Microchip dsPIC33 DFP (MC/MP) for specified architectures by locating c30_device.info,
# preferring HOME packs over system installs, and extracting a clean DFP root
# (.../dsPIC33XX-YY_DFP/<version>), regardless of deeper subfolders (e.g., xc16/bin/...).
#
# Usage:
#   find_dspic33_dfp(
#     OUT_INFO   <var>                # output: full path to c30_device.info
#     OUT_ROOT   <var>                # output: DFP root (.../dsPIC33XX-YY_DFP/<version>)
#     [ARCHES    <AK|CD|CH|...> ...]  # default: AK CD CH
#     [FAMILIES  <MC|MP|...> ...]     # default: MC MP
#     [ROOTS     <root1> <root2> ...] # optional; HOME is always searched first
#     [HINT_VERSION <ver>]            # optional preferred version (e.g., 1.1.109)
#   )
function(find_dspic33_dfp)
  set(options)
  set(oneValueArgs OUT_INFO OUT_ROOT HINT_VERSION)
  set(multiValueArgs ARCHES FAMILIES ROOTS)
  cmake_parse_arguments(FD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FD_OUT_INFO OR NOT FD_OUT_ROOT)
    message(FATAL_ERROR "find_dspic33_dfp: You must pass OUT_INFO and OUT_ROOT variable names.")
  endif()

  # --- Defaults ---
  if(NOT FD_ARCHES)
    set(FD_ARCHES AK CD CH)
  endif()
  if(NOT FD_FAMILIES)
    set(FD_FAMILIES MC MP)
  endif()

  # Home path (works on Win/Linux)
  set(_home "$ENV{HOME}")
  if(WIN32 AND NOT _home)
    set(_home "$ENV{USERPROFILE}")
    file(TO_CMAKE_PATH "${_home}" _home)
  endif()

  # Root search list: HOME first (highest priority), then system roots
  set(_default_roots
    "${_home}/.mchp_packs"
    "/opt/microchip" "/opt/microchip/mplabx"
    "C:/Program Files/Microchip" "C:/Program Files (x86)/Microchip"
  )
  set(_search_roots "${_default_roots}")
  if(FD_ROOTS)
    # If caller passes ROOTS, append them after HOME so HOME still wins by default
    list(APPEND _search_roots ${FD_ROOTS})
  endif()

  # Build case-insensitive regex for pack dir: dspic33<arch>[-_]<family>_dfp
  set(_alts "")
  foreach(_a IN LISTS FD_ARCHES)
    string(TOLOWER "${_a}" _al)
    foreach(_f IN LISTS FD_FAMILIES)
      string(TOLOWER "${_f}" _fl)
      list(APPEND _alts "dspic33${_al}[-_]${_fl}_dfp")
    endforeach()
  endforeach()
  string(REPLACE ";" "|" _alts_regex "${_alts}")
  set(_pack_regex "(${_alts_regex})")

  # Collect candidates, tagging each with a priority rank (0 = HOME, 1 = system/others)
  # We'll sort by (rank ASC, version/path NATURAL DESC)
  set(_ranked_candidates)  # entries like: "0|/full/path/to/c30_device.info"
  foreach(_root IN LISTS _search_roots)
    if(NOT _root OR NOT EXISTS "${_root}")
      continue()
    endif()
    # Determine rank: 0 for home packs, 1 otherwise
    set(_rank 1)
    if(_root MATCHES "^${_home}(/|\\\\)\\.mchp_packs")
      set(_rank 0)
    endif()

    file(GLOB_RECURSE _found "${_root}/**/c30_device.info")
    foreach(_p IN LISTS _found)
      string(TOLOWER "${_p}" _pl)
      if(_pl MATCHES "${_pack_regex}")
        list(APPEND _ranked_candidates "${_rank}|${_p}")
      endif()
    endforeach()
  endforeach()

  if(NOT _ranked_candidates)
    message(FATAL_ERROR
      "find_dspic33_dfp: No c30_device.info found for architectures {${FD_ARCHES}} "
      "and families {${FD_FAMILIES}} under roots: ${_search_roots}")
  endif()

  # Optional: version hint filter
  if(FD_HINT_VERSION)
    set(_hinted)
    foreach(_rp IN LISTS _ranked_candidates)
      string(REPLACE "|" ";" _parts "${_rp}")
      list(GET _parts 1 _path)
      if(_path MATCHES "/${FD_HINT_VERSION}(/|\\\\)")
        list(APPEND _hinted "${_rp}")
      endif()
    endforeach()
    if(_hinted)
      set(_ranked_candidates "${_hinted}")
    endif()
  endif()

  # ---- Prefer rank 0; else fall back to rank 1 (Windows-safe) ----
  # Split into path-only buckets first
  set(_r0_paths)
  set(_r1_paths)
  foreach(_entry IN LISTS _ranked_candidates)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _rk)
    list(GET _parts 1 _pth)
    if(_rk EQUAL 0)
      list(APPEND _r0_paths "${_pth}")
    else()
      list(APPEND _r1_paths "${_pth}")
    endif()
  endforeach()

  # Sort each bucket naturally (DESC → newest-looking first) and pick
  if(_r0_paths)
    list(SORT _r0_paths COMPARE NATURAL ORDER DESCENDING)
    list(GET _r0_paths 0 _best_path)
  elseif(_r1_paths)
    list(SORT _r1_paths COMPARE NATURAL ORDER DESCENDING)
    list(GET _r1_paths 0 _best_path)
  else()
    message(FATAL_ERROR "find_dspic33_dfp: No ranked candidates after filtering.")
  endif()

  # --- DFP root extraction ---
  # Find ".../<packname>/<version>/" inside the path and cut there.
  string(TOLOWER "${_best_path}" _best_lc)
  string(REGEX MATCH "(.*[/\\\\](dspic33[a-z0-9]+[-_][a-z0-9]+_dfp)[/\\\\]([^/\\\\]+))" _m "${_best_lc}")
  if(_m)
    string(REGEX REPLACE "^(.*[/\\\\](dspic33[a-z0-9]+[-_][a-z0-9]+_dfp)[/\\\\]([^/\\\\]+)).*$" "\\1" _dfp_root_lc "${_best_lc}")
    string(LENGTH "${_dfp_root_lc}" _keep_len)
    string(SUBSTRING "${_best_path}" 0 ${_keep_len} DFP_ROOT)
  else()
    # Fallback: strip known tails to version dir
    set(_tmp "${_best_path}")
    string(REGEX REPLACE "([/\\\\])xc16([/\\\\].*)?$" "" _tmp "${_tmp}")
    string(REGEX REPLACE "([/\\\\])c30_device.info$" "" _tmp "${_tmp}")
    set(DFP_ROOT "${_tmp}")
  endif()

  # --- OUT_INFO should be the DIRECTORY containing c30_device.info (e.g., .../xc16/bin) ---
  get_filename_component(_info_dir "${_best_path}" DIRECTORY)

  # Outputs
  set(${FD_OUT_INFO} "${_info_dir}" PARENT_SCOPE)  # directory only, no filename
  set(${FD_OUT_ROOT} "${DFP_ROOT}"  PARENT_SCOPE)
endfunction()

if("${BOARD_QUALIFIERS}" MATCHES "/p33ak128mc106" AND
    "${BOARD}" MATCHES "dspic33a_curiosity")
    set(TARGET_CPU "33AK128MC106")
    find_dspic33_dfp(
    OUT_INFO C30_DEVICE_INFO
    OUT_ROOT DFP_ROOT
    ARCHES AK
    FAMILIES MC
    )
elseif("${BOARD_QUALIFIERS}" MATCHES "/p33ak512mps512" AND
    "${BOARD}" MATCHES "dspic33a_curiosity")
    set(TARGET_CPU "33AK512MPS512")
    find_dspic33_dfp(
    OUT_INFO C30_DEVICE_INFO
    OUT_ROOT DFP_ROOT
    ARCHES AK
    FAMILIES MP
    )
endif()
message(STATUS "DFP file in ${C30_DEVICE_INFO}")
message(STATUS "DFP path  ${DFP_ROOT}")

set(ENV{C30_DEVICE_INFO} "${C30_DEVICE_INFO}")
set(ENV{DFP_ROOT}        "${DFP_ROOT}")
set(ENV{TARGET_CPU}      "${TARGET_CPU}")

set(CMAKE_C_FLAGS "-D__XC_DSC__ -mdfp=\"${DFP_ROOT}/xc16\"" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "-D__XC_DSC__ -mdfp=\"${DFP_ROOT}/xc16\"" CACHE STRING "" FORCE)

# Append to DTS preprocessor flags with DFP path
if(DEFINED DTS_EXTRA_CPPFLAGS AND NOT "${DTS_EXTRA_CPPFLAGS}" STREQUAL "")
  list(APPEND DTS_EXTRA_CPPFLAGS "-mdfp=\"${DFP_ROOT}/xc16\"")
else()
  set(DTS_EXTRA_CPPFLAGS "-mdfp=\"${DFP_ROOT}/xc16\"")
endif()
# Find and validate the xc-dsc-gcc compiler binary
find_program(CMAKE_C_COMPILER xc-dsc-gcc PATHS ${XCDSC_TOOLCHAIN_PATH}/bin/ NO_DEFAULT_PATH REQUIRED )

# Get compiler version
execute_process(
    COMMAND ${CMAKE_C_COMPILER} --version
    OUTPUT_VARIABLE XCDSC_VERSION_STR
    ERROR_VARIABLE XCDSC_VERSION_ERR
    OUTPUT_STRIP_TRAILING_WHITESPACE )
# Verify that the installed version is v3.30 or higher
if("${XCDSC_VERSION_STR}" MATCHES ".*v([0-9]+)\\.([0-9]+).*")
    string(REGEX REPLACE ".*v([0-9]+)\\.([0-9]+).*" "\\1\\2" __XCDSC_VERSION__ "${XCDSC_VERSION_STR}")
    math(EXPR XCDSC_VERSION_INT "${__XCDSC_VERSION__}")
    if(XCDSC_VERSION_INT LESS 330)
    message(FATAL_ERROR "XC-DSC compiler v3.30 or newer is required. Found version: ${XCDSC_VERSION_STR}")
    endif()
else()
    message(FATAL_ERROR "Unable to detect XC-DSC compiler version from: '${XCDSC_VERSION_STR}'")
endif()
