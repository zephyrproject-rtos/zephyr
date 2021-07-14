# ToDo:
# - Ensure LMA / VMA sections are correctly grouped similar to scatter file creation.
cmake_minimum_required(VERSION 3.18)

set(SORT_TYPE_NAME SORT_BY_NAME)

function(memory_content)
  cmake_parse_arguments(MC "" "CONTENT;NAME;START;SIZE;FLAGS" "" ${ARGN})

  set(TEMP)
  if(MC_NAME)
    set(TEMP "${TEMP} ${MC_NAME}")
  endif()

  if(MC_FLAGS)
    set(TEMP "${TEMP} (${MC_FLAGS})")
  endif()

  if(MC_START)
    set(TEMP "${TEMP} : ORIGIN = (${MC_START})")
  endif()

  if(MC_SIZE)
    set(TEMP "${TEMP}, LENGTH = (${MC_SIZE})")
  endif()

  set(${MC_CONTENT} "${${MC_CONTENT}}\n${TEMP}" PARENT_SCOPE)
endfunction()

function(section_content)
  set(SEC_TYPE_NOLOAD NOLOAD)
  set(SEC_TYPE_BSS    NOLOAD)

  cmake_parse_arguments(SEC "" "CONTENT;NAME;ADDRESS;TYPE;ALIGN;PASS;SUBALIGN;VMA;LMA;NOINPUT" "" ${ARGN})

  if(DEFINED SEC_PASS AND NOT "${PASS}" IN_LIST SEC_PASS)
    # This section is not active in this pass, ignore.
    return()
  endif()

  # SEC_NAME is required, test for that.
  #if(SEC_NAME)
  set(TEMP "${SEC_NAME}")
  #endif()

  if(DEFINED SEC_ADDRESS)
    set(TEMP "${TEMP} ${SEC_ADDRESS}")
  endif()

  if(SEC_TYPE)
    set(TEMP "${TEMP} (${SEC_TYPE_${SEC_TYPE}})")
  endif()

  set(TEMP "${TEMP} :")

  if(SEC_SUBALIGN)
    set(TEMP "${TEMP} SUBALIGN(${SEC_SUBALIGN})")
  endif()

  if(SEC_ALIGN)
    set(TEMP "${TEMP} ALIGN(${SEC_ALIGN})")
  endif()

  string(REGEX REPLACE "^[\.]" "" SEC_NAME_CLEAN "${SEC_NAME}")
  string(REPLACE "." "_" SEC_NAME_CLEAN "${SEC_NAME_CLEAN}")

  set(TEMP "${TEMP}\n{")
  set(TEMP "${TEMP}\n  __${SEC_NAME_CLEAN}_start = .;")
  if(NOT SEC_NOINPUT)
    set(TEMP "${TEMP}\n  *(${SEC_NAME})")
    set(TEMP "${TEMP}\n  *(\"${SEC_NAME}.*\")")
  endif()

  set(INDEX_KEY    SECTION_${SEC_NAME}_INDEX)
  set(SETTINGS_KEY SECTION_${SEC_NAME}_SETTINGS)

  if(DEFINED ${INDEX_KEY})
    list(SORT ${INDEX_KEY} COMPARE NATURAL)
    foreach(idx ${${INDEX_KEY}})

      cmake_parse_arguments(SETTINGS "" "INPUT;KEEP;ALIGN;SORT" "SYMBOLS" ${${SETTINGS_KEY}_${idx}})

      if(DEFINED SETTINGS_ALIGN)
        set(TEMP "${TEMP}\n  . = ALIGN(${SETTINGS_ALIGN});")
      endif()

      if(DEFINED SETTINGS_SYMBOLS)
        list(LENGTH SETTINGS_SYMBOLS symbols_count)
	if(${symbols_count} GREATER 0)
          list(GET SETTINGS_SYMBOLS 0 symbol_start)
	endif()
	if(${symbols_count} GREATER 1)
          list(GET SETTINGS_SYMBOLS 1 symbol_end)
	endif()
      endif()

      if(DEFINED symbol_start)
        set(TEMP "${TEMP}\n  ${symbol_start} = .;")
      endif()

      if(NOT DEFINED SETTINGS_INPUT)
        continue()
      endif()

      if(SETTINGS_KEEP AND SETTINGS_SORT)
        set(TEMP "${TEMP}\n  KEEP(*(${SORT_TYPE_${SETTINGS_SORT}}(${SETTINGS_INPUT})));")
      elseif(SETTINGS_SORT)
        message(WARNING "Not tested")
        set(TEMP "${TEMP}\n  *(${SORT_TYPE_${SETTINGS_SORT}}(${SETTINGS_INPUT}));")
      elseif(SETTINGS_KEEP)
        set(TEMP "${TEMP}\n  KEEP(*(${SETTINGS_INPUT}));")
      else()
        set(TEMP "${TEMP}\n  *(${SETTINGS_INPUT})")
      endif()

      if(DEFINED symbol_end)
        set(TEMP "${TEMP}\n  ${symbol_end} = .;")
      endif()

    endforeach()
  endif()

  set(TEMP "${TEMP}\n  __${SEC_NAME_CLEAN}_end = .;")

  set(TEMP "${TEMP}\n}")

  if(SEC_VMA)
    set(TEMP "${TEMP} > ${SEC_VMA}")
  endif()

  if(SEC_VMA AND SEC_LMA)
    set(TEMP "${TEMP} AT")
  endif()

  if(SEC_LMA)
    set(TEMP "${TEMP} > ${SEC_LMA}")
  endif()

  set(TEMP "${TEMP}\n__${SEC_NAME_CLEAN}_size = __${SEC_NAME_CLEAN}_end - __${SEC_NAME_CLEAN}_start;")
  set(TEMP "${TEMP}\n__${SEC_NAME_CLEAN}_load_start = LOADADDR(${SEC_NAME});")

  set(${SEC_CONTENT} "${${SEC_CONTENT}}\n${TEMP}\n" PARENT_SCOPE)
endfunction()

function(section_discard)
  cmake_parse_arguments(SEC "" "CONTENT;NAME;ADDRESS;TYPE;ALIGN;SUBALIGN;VMA;LMA" "" ${ARGN})

  # SEC_NAME is required, test for that.
  set(SEC_NAME "/DISCARD/")
  set(TEMP "${SEC_NAME} :")

  set(TEMP "${TEMP}\n{")

  set(INDEX_KEY    SECTION_${SEC_NAME}_INDEX)
  set(SETTINGS_KEY SECTION_${SEC_NAME}_SETTINGS)

  if(DEFINED ${INDEX_KEY})
    foreach(idx ${${INDEX_KEY}})

      cmake_parse_arguments(SETTINGS "" "INPUT" "" ${${SETTINGS_KEY}_${idx}})
      set(TEMP "${TEMP}\n  *(${SETTINGS_INPUT})")
    endforeach()
  endif()

  set(TEMP "${TEMP}\n}")

  set(${SEC_CONTENT} "${${SEC_CONTENT}}\n${TEMP}\n" PARENT_SCOPE)
endfunction()

set(OUT "OUTPUT_FORMAT(\"${FORMAT}\")\n")

set(OUT "${OUT}MEMORY\n{")
foreach(region ${MEMORY_REGIONS})
  if("${region}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(REGION "" "NAME;START" "" ${CMAKE_MATCH_1})
    set(REGION_${REGION_NAME}_START ${REGION_START})
    memory_content(CONTENT OUT ${CMAKE_MATCH_1})
    list(APPEND memory_regions ${REGION_NAME})
  endif()
endforeach()
set(OUT "${OUT}\n}\n")

set(OUT "${OUT}\nSECTIONS {\n")
foreach(settings ${SECTION_SETTINGS})
  if("${settings}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(SETTINGS "" "PASS;SECTION;PRIO" "" ${CMAKE_MATCH_1})

    if(DEFINED SETTINGS_PASS AND NOT "${PASS}" IN_LIST SETTINGS_PASS)
      # This section setting is not active in this pass, ignore.
      continue()
    endif()

    set(INDEX_KEY    SECTION_${SETTINGS_SECTION}_INDEX)
    set(INDEX_COUNT  SECTION_${SETTINGS_SECTION}_COUNT)
    set(SETTINGS_KEY SECTION_${SETTINGS_SECTION}_SETTINGS)

    if(DEFINED SETTINGS_PRIO)
      set(KEY ${SETTINGS_PRIO})
    else()
      if(NOT DEFINED ${INDEX_COUNT})
        set(${INDEX_COUNT} 999)
      endif()

      math(EXPR ${INDEX_COUNT} "${${INDEX_COUNT}} + 1")
      set(KEY ${${INDEX_COUNT}})
    endif()

    list(APPEND ${INDEX_KEY} ${KEY})
    set(${SETTINGS_KEY}_${KEY} ${CMAKE_MATCH_1})
  endif()
endforeach()


foreach(section ${SECTIONS})
  if("${section}" MATCHES "^{(.*)}$")
    cmake_parse_arguments(SECTION "" "VMA;LMA;TYPE" "" ${CMAKE_MATCH_1})

    set(MEM_REGION MEM_REGION)
    if("${SECTION_TYPE}" STREQUAL "NOLOAD")
      set(MEM_REGION ${MEM_REGION}_NOLOAD)
    endif()

    if(DEFINED SECTION_VMA)
      set(MEM_REGION ${MEM_REGION}_${SECTION_VMA})
    endif()

    if(DEFINED SECTION_LMA)
      set(MEM_REGION ${MEM_REGION}_${SECTION_LMA})
    endif()

    set(INDEX_COUNT    ${MEM_REGION}_COUNT)

    if(NOT DEFINED ${INDEX_COUNT})
      set(${INDEX_COUNT} -1)
      set(KEY ${${INDEX_COUNT}})
    endif()

    math(EXPR ${INDEX_COUNT} "${${INDEX_COUNT}} + 1")
    set(KEY ${${INDEX_COUNT}})

    set(${MEM_REGION}_${KEY} ${CMAKE_MATCH_1})
  endif()
endforeach()




foreach(region ${memory_regions})
  if(DEFINED REGION_${region}_START)
    set(OUT "${OUT}\n . = ${REGION_${region}_START};\n")
  endif()

  set(MEM_REGION MEM_REGION_${region})
  set(INDEX_COUNT ${MEM_REGION}_COUNT)

  if(DEFINED ${INDEX_COUNT})
    foreach(idx RANGE 0 ${${INDEX_COUNT}})
      section_content(CONTENT OUT ${${MEM_REGION}_${idx}})
    endforeach()
  endif()

  foreach(lregion ${memory_regions})
    set(MEM_REGION MEM_REGION_${region}_${lregion})
    set(INDEX_COUNT ${MEM_REGION}_COUNT)

    if(DEFINED ${INDEX_COUNT})
      foreach(idx RANGE 0 ${${INDEX_COUNT}})
        section_content(CONTENT OUT ${${MEM_REGION}_${idx}})
      endforeach()
    endif()
  endforeach()

  set(MEM_REGION MEM_REGION_NOLOAD_${region})
  set(INDEX_COUNT ${MEM_REGION}_COUNT)

  if(DEFINED ${INDEX_COUNT})
    foreach(idx RANGE 0 ${${INDEX_COUNT}})
      section_content(CONTENT OUT ${${MEM_REGION}_${idx}})
    endforeach()
  endif()

  foreach(lregion ${memory_regions})
    set(MEM_REGION MEM_REGION_NOLOAD_${region}_${lregion})
    set(INDEX_COUNT ${MEM_REGION}_COUNT)

    if(DEFINED ${INDEX_COUNT})
      foreach(idx RANGE 0 ${${INDEX_COUNT}})
        section_content(CONTENT OUT ${${MEM_REGION}_${idx}})
      endforeach()
    endif()
  endforeach()
endforeach()

set(MEM_REGION MEM_REGION)
set(INDEX_COUNT ${MEM_REGION}_COUNT)

if(DEFINED ${INDEX_COUNT})
  foreach(idx RANGE 0 ${${INDEX_COUNT}})
    section_content(CONTENT OUT ${${MEM_REGION}_${idx}})
  endforeach()
endif()

section_discard(CONTENT OUT)
set(OUT "${OUT}\n}\n")

if(OUT_FILE)
  file(WRITE ${OUT_FILE} "${OUT}")
else()
  message("${OUT}")
endif()
