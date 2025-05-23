# ToDo:
# - Ensure LMA / VMA sections are correctly grouped similar to scatter file creation.
cmake_minimum_required(VERSION 3.20.0)

set(SORT_TYPE_NAME SORT_BY_NAME)

function(system_to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(name    GLOBAL PROPERTY ${STRING_OBJECT}_NAME)
  get_property(regions GLOBAL PROPERTY ${STRING_OBJECT}_REGIONS)
  get_property(format  GLOBAL PROPERTY ${STRING_OBJECT}_FORMAT)
  get_property(entry   GLOBAL PROPERTY ${STRING_OBJECT}_ENTRY)
  get_property(symbols GLOBAL PROPERTY ${STRING_OBJECT}_SYMBOLS)

  set(${STRING_STRING} "OUTPUT_FORMAT(\"${format}\")\n\n")

  set(${STRING_STRING} "${${STRING_STRING}}MEMORY\n{\n")
  foreach(region ${regions})
    get_property(name    GLOBAL PROPERTY ${region}_NAME)
    get_property(address GLOBAL PROPERTY ${region}_ADDRESS)
    get_property(flags   GLOBAL PROPERTY ${region}_FLAGS)
    get_property(size    GLOBAL PROPERTY ${region}_SIZE)

    if(DEFINED flags)
      set(flags "(${flags})")
    endif()

    if(DEFINED address)
      set(start ": ORIGIN = (${address})")
    endif()

    if(DEFINED size)
      set(size ", LENGTH = (${size})")
    endif()
    set(memory_region "  ${name} ${flags} ${start}${size}")

    set(${STRING_STRING} "${${STRING_STRING}}${memory_region}\n")
  endforeach()

  set(${STRING_STRING} "${${STRING_STRING}}}\n\n")

  set(${STRING_STRING} "${${STRING_STRING}}ENTRY(\"${entry}\")\n\n")

  set(${STRING_STRING} "${${STRING_STRING}}SECTIONS\n{")
  foreach(region ${regions})
    to_string(OBJECT ${region} STRING ${STRING_STRING})
  endforeach()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS_FIXED)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
  endforeach()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
  endforeach()

  foreach(symbol ${symbols})
    to_string(OBJECT ${symbol} STRING ${STRING_STRING})
  endforeach()

  set(${STRING_STRING} "${${STRING_STRING}}\n}\n" PARENT_SCOPE)
endfunction()

function(symbol_to_string)
  cmake_parse_arguments(STRING "" "SYMBOL;STRING" "" ${ARGN})

  get_property(name     GLOBAL PROPERTY ${STRING_SYMBOL}_NAME)
  get_property(expr     GLOBAL PROPERTY ${STRING_SYMBOL}_EXPR)
  get_property(size     GLOBAL PROPERTY ${STRING_SYMBOL}_SIZE)
  get_property(symbol   GLOBAL PROPERTY ${STRING_SYMBOL}_SYMBOL)
  get_property(subalign GLOBAL PROPERTY ${STRING_SYMBOL}_SUBALIGN)

  string(REPLACE "\\" "" expr "${expr}")
  string(REGEX MATCHALL "@([^@]*)@" match_res ${expr})

  foreach(match ${match_res})
    string(REPLACE "@" "" match ${match})
    string(REPLACE "@${match}@" "${match}" expr ${expr})
  endforeach()

  set(${STRING_STRING} "${${STRING_STRING}}\n${symbol} = ${expr};\n" PARENT_SCOPE)
endfunction()

function(group_to_string)
  cmake_parse_arguments(STRING "" "OBJECT;STRING" "" ${ARGN})

  get_property(type GLOBAL PROPERTY ${STRING_OBJECT}_OBJ_TYPE)

  # Output a comment for the section start
  set(${STRING_STRING} "${${STRING_STRING}}\n  /* ${type} : ${STRING_OBJECT} */")

  if(${type} STREQUAL REGION)
    get_property(empty GLOBAL PROPERTY ${STRING_OBJECT}_EMPTY)
    if(empty)
      return()
    endif()

    get_property(address GLOBAL PROPERTY ${STRING_OBJECT}_ADDRESS)
    set(${STRING_STRING} "${${STRING_STRING}}\n  . = ${address};\n\n")
  else()
    get_property(name GLOBAL PROPERTY ${STRING_OBJECT}_NAME)
    get_property(symbol GLOBAL PROPERTY ${STRING_OBJECT}_SYMBOL)
    string(TOLOWER ${name} name)

    get_objects(LIST sections OBJECT ${STRING_OBJECT} TYPE SECTION)
    list(GET sections 0 section)
    get_property(first_section_name GLOBAL PROPERTY ${section}_NAME)

    if(DEFINED first_section_name AND "${symbol}" STREQUAL "SECTION")
      set_property(GLOBAL APPEND PROPERTY ${section}_START_SYMBOLS __${name}_start)
    else()
      set(${STRING_STRING} "${${STRING_STRING}}\n  __${name}_start = .;\n")
    endif()

    set(${STRING_STRING} "${${STRING_STRING}}\n  __${name}_size = __${name}_end - __${name}_start;\n")
    set(${STRING_STRING} "${${STRING_STRING}}\n  __${name}_load_start = LOADADDR(${first_section_name});\n")
  endif()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS_FIXED)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
  endforeach()

  get_property(groups GLOBAL PROPERTY ${STRING_OBJECT}_GROUPS)
  foreach(group ${groups})
    to_string(OBJECT ${group} STRING ${STRING_STRING})
  endforeach()

  get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_SECTIONS)
  foreach(section ${sections})
    to_string(OBJECT ${section} STRING ${STRING_STRING})
  endforeach()

  get_parent(OBJECT ${STRING_OBJECT} PARENT parent TYPE SYSTEM)
  get_property(regions GLOBAL PROPERTY ${parent}_REGIONS)
  list(REMOVE_ITEM regions ${STRING_OBJECT})
  foreach(region ${regions})
    if(${type} STREQUAL REGION)
      get_property(address GLOBAL PROPERTY ${region}_ADDRESS)
      set(${STRING_STRING} "${${STRING_STRING}}\n  . = ${address};\n\n")
    endif()

    get_property(vma GLOBAL PROPERTY ${region}_NAME)
    get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_SECTIONS_FIXED)
    foreach(section ${sections})
      to_string(OBJECT ${section} STRING ${STRING_STRING})
    endforeach()

    get_property(groups GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_GROUPS)
    foreach(group ${groups})
      to_string(OBJECT ${group} STRING ${STRING_STRING})
    endforeach()

    get_property(sections GLOBAL PROPERTY ${STRING_OBJECT}_${vma}_SECTIONS)
    foreach(section ${sections})
      to_string(OBJECT ${section} STRING ${STRING_STRING})
    endforeach()
  endforeach()

  if(NOT ${type} STREQUAL REGION)
    set(${STRING_STRING} "${${STRING_STRING}}\n  __${name}_end = .;\n")
  endif()

  get_property(symbols GLOBAL PROPERTY ${STRING_OBJECT}_SYMBOLS)
  foreach(symbol ${symbols})
    to_string(OBJECT ${symbol} STRING ${STRING_STRING})
  endforeach()

  set(${STRING_STRING} ${${STRING_STRING}} PARENT_SCOPE)
endfunction()

# Helper to add size-checks and padding to handle MIN_SIZE and MAX_SIZE
macro(add_min_max_size TEMP min_size max_size symbol_start symbol_end)
  if(DEFINED ${min_size})
    set(${TEMP} "${${TEMP}}\n  . = MAX(${${symbol_start}} + ${${min_size}}, .); /*MIN_SIZE*/")
  endif()
  if(DEFINED ${max_size})
    set(${TEMP} "${${TEMP}}\n  ASSERT( ${${symbol_end}} <= ( ${${symbol_start}} + ${${max_size}}), \"MAX_SIZE\");\n . = MIN( ., ${${symbol_start}} + ${${max_size}}); /*MAX_SIZE*/")
  endif()
endmacro()

function(section_to_string)
  cmake_parse_arguments(STRING "" "SECTION;STRING" "" ${ARGN})

  get_property(name       GLOBAL PROPERTY ${STRING_SECTION}_NAME)
  get_property(name_clean GLOBAL PROPERTY ${STRING_SECTION}_NAME_CLEAN)
  get_property(address    GLOBAL PROPERTY ${STRING_SECTION}_ADDRESS)
  get_property(type       GLOBAL PROPERTY ${STRING_SECTION}_TYPE)
  get_property(align_in   GLOBAL PROPERTY ${STRING_SECTION}_ALIGN_WITH_INPUT)
  get_property(align      GLOBAL PROPERTY ${STRING_SECTION}_ALIGN)
  get_property(subalign   GLOBAL PROPERTY ${STRING_SECTION}_SUBALIGN)
  get_property(vma        GLOBAL PROPERTY ${STRING_SECTION}_VMA)
  get_property(lma        GLOBAL PROPERTY ${STRING_SECTION}_LMA)
  get_property(noinput    GLOBAL PROPERTY ${STRING_SECTION}_NOINPUT)
  get_property(noinit     GLOBAL PROPERTY ${STRING_SECTION}_NOINIT)
  get_property(nosymbols  GLOBAL PROPERTY ${STRING_SECTION}_NOSYMBOLS)
  get_property(parent     GLOBAL PROPERTY ${STRING_SECTION}_PARENT)
  get_property(start_syms GLOBAL PROPERTY ${STRING_SECTION}_START_SYMBOLS)
  get_property(sect_min_size GLOBAL PROPERTY ${STRING_SECTION}_MIN_SIZE)
  get_property(sect_max_size GLOBAL PROPERTY ${STRING_SECTION}_MAX_SIZE)

  string(REGEX REPLACE "^[\.]" "" name_clean "${name}")
  string(REPLACE "." "_" name_clean "${name_clean}")

  set(SECTION_TYPE_NOLOAD NOLOAD)
  set(SECTION_TYPE_BSS    NOLOAD)
  if(DEFINED type)
    set(type " (${SECTION_TYPE_${type}})")
  endif()

  set(TEMP "${TEMP} :")
  set(secalign "")

  if(align_in)
    set(secalign " ALIGN_WITH_INPUT")
  endif()

  if(DEFINED align)
    set(secalign "${secalign} ALIGN(${align})")
  endif()

  if(DEFINED subalign)
    set(secalign "${secalign} SUBALIGN(${subalign})")
  endif()

  set(TEMP "${name} ${address}${type} :${secalign}\n{")

  # Symbol names for size_check
  set(sect_symbol_start)
  set(sect_symbol_end)

  foreach(start_symbol ${start_syms})
    set(TEMP "${TEMP}\n  ${start_symbol} = .;")
    set(sect_symbol_start "${start_symbol}")
  endforeach()

  if(NOT nosymbols)
    set(TEMP "${TEMP}\n  __${name_clean}_start = .;")
    set(sect_symbol_start "__${name_clean}_start")
  endif()

  # Min and Max size may need symbols too, even if the user did not want them
  if((DEFINED sect_min_size OR DEFINED sect_max_size)
    AND NOT DEFINED sect_symbol_start)
      set(sect_symbol_start "__${name_clean}_size_check_start")
  endif()

  if(NOT noinput)
    set(TEMP "${TEMP}\n  *(${name})")
    set(TEMP "${TEMP}\n  *(\"${name}.*\")")
  endif()

  get_property(indicies GLOBAL PROPERTY ${STRING_SECTION}_SETTINGS_INDICIES)
  foreach(idx ${indicies})
    get_property(align    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ALIGN)
    get_property(any      GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_ANY)
    get_property(first    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FIRST)
    get_property(keep     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_KEEP)
    get_property(sort     GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_SORT)
    get_property(flags    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_FLAGS)
    get_property(input    GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_INPUT)
    get_property(symbols  GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_SYMBOLS)
    get_property(offset   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_OFFSET)
    get_property(min_size   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_MIN_SIZE)
    get_property(max_size   GLOBAL PROPERTY ${STRING_SECTION}_SETTING_${idx}_MAX_SIZE)

    if(DEFINED align)
      set(TEMP "${TEMP}\n  . = ALIGN(${align});")
    endif()

    if(DEFINED symbols)
      list(LENGTH symbols symbols_count)
      if(${symbols_count} GREATER 0)
        list(GET symbols 0 symbol_start)
      endif()
      if(${symbols_count} GREATER 1)
        list(GET symbols 1 symbol_end)
      endif()
    endif()
    # Min and Max size may need symbols too, even if the user did not want them
    if(DEFINED min_size OR DEFINED max_size)
      foreach(se "start" "end")
        if(NOT DEFINED symbol_${se})
          set(symbol_${se} "__${name_clean}_${idx}_size_check_${se}")
        endif()
      endforeach()
    endif()

    if(DEFINED offset AND NOT ("${offset}" STREQUAL "${current_offset}"))
      set(TEMP "${TEMP}\n  . = ${offset}; /* OFFSET */")
      set(current_offset ${offset})
    endif()

    if(DEFINED symbol_start)
      set(TEMP "${TEMP}\n  ${symbol_start} = .;")
    endif()

    foreach(setting ${input})
      if(DEFINED offset AND NOT ("${offset}" STREQUAL "${current_offset}"))
        set(TEMP "${TEMP}\n  . = ${offset};")
        set(current_offset ${offset})
      endif()

      #setting may have file-pattern or not.
      if(setting MATCHES "(.+)\\((.+)\\)")
        set(file_pattern "${CMAKE_MATCH_1}")
        set(section_pattern "${CMAKE_MATCH_2}")
      else()
        set(file_pattern "*")
        set(section_pattern "${setting}")
      endif()

      if(sort)
        set(section_pattern "${SORT_TYPE_${sort}}(${section_pattern})")
      endif()
      set(pattern "${file_pattern}(${section_pattern})")
      if(keep)
        set(pattern "KEEP(${pattern})")
      endif()
      set(TEMP "${TEMP}\n  ${pattern};")
    endforeach()

    if(DEFINED symbol_end)
      set(TEMP "${TEMP}\n  ${symbol_end} = .;")
    endif()

    add_min_max_size(TEMP min_size max_size symbol_start symbol_end)

    set(symbol_start)
    set(symbol_end)
  endforeach()

  if(NOT nosymbols)
    set(TEMP "${TEMP}\n  __${name_clean}_end = .;")
    set(sect_symbol_end "__${name_clean}_end")
  endif()

  if(DEFINED extra_symbol_end)
    set(TEMP "${TEMP}\n  ${extra_symbol_end} = .;")
    set(sect_symbol_end "${extra_symbol_end}")
  endif()

  # Min and Max size may need symbols too, even if the user did not want them
  if((DEFINED sect_min_size OR DEFINED sect_max_size)
    AND NOT DEFINED sect_symbol_end)
      set(sect_symbol_end "__${name_clean}_size_check_end")
  endif()

  add_min_max_size(TEMP sect_min_size sect_max_size sect_symbol_start sect_symbol_end)

  set(TEMP "${TEMP}\n}")

  get_property(parent_type GLOBAL PROPERTY ${parent}_OBJ_TYPE)
  if(${parent_type} STREQUAL GROUP)
    get_property(vma GLOBAL PROPERTY ${parent}_VMA)
    get_property(lma GLOBAL PROPERTY ${parent}_LMA)
  endif()

  if(DEFINED vma)
    set(TEMP "${TEMP} > ${vma}")
  endif()

  if(DEFINED vma AND DEFINED lma)
    set(TEMP "${TEMP} AT")
  endif()

  if(DEFINED lma)
    set(TEMP "${TEMP} > ${lma}")
  endif()

  if(NOT nosymbols)
    set(TEMP "${TEMP}\n__${name_clean}_size = __${name_clean}_end - __${name_clean}_start;")
    set(TEMP "${TEMP}\nPROVIDE(__${name_clean}_align = ALIGNOF(${name}));")
    set(TEMP "${TEMP}\n__${name_clean}_load_start = LOADADDR(${name});")
  endif()

  set(${STRING_STRING} "${${STRING_STRING}}\n${TEMP}\n" PARENT_SCOPE)
endfunction()

# /DISCARD/ is an ld specific section, so let's append it here before processing.
list(APPEND SECTIONS "{NAME\;/DISCARD/\;NOINPUT\;TRUE\;NOSYMBOLS\;TRUE}")

function(process_region)
  cmake_parse_arguments(REGION "" "OBJECT" "" ${ARGN})

  process_region_common(${ARGN})

  set(groups)
  get_objects(LIST groups OBJECT ${REGION_OBJECT} TYPE GROUP)
  foreach(group ${groups})
    get_property(parent GLOBAL PROPERTY ${group}_PARENT)
    get_property(parent_type GLOBAL PROPERTY ${parent}_OBJ_TYPE)

    if(${parent_type} STREQUAL GROUP)
      get_property(vma GLOBAL PROPERTY ${parent}_VMA)
      get_property(lma GLOBAL PROPERTY ${parent}_LMA)

      set_property(GLOBAL PROPERTY ${group}_VMA ${vma})
      set_property(GLOBAL PROPERTY ${group}_LMA ${lma})
    endif()
  endforeach()
endfunction()

include(${CMAKE_CURRENT_LIST_DIR}/../linker_script_common.cmake)
