# For MWDT the elfconvert command is made into a script.
# Reason for that is because not a single command covers all use cases,
# and it must therefore be possible to call individual commands, depending
# on the arguments used.

# Handle stripping
if(STRIP_DEBUG OR STRIP_ALL OR STRIP_UNNEEDED)
  if(STRIP_ALL)
    set(obj_copy_strip "-qs")
  elseif(STRIP_DEBUG)
    set(obj_copy_strip "-ql")
  elseif(STRIP_UNNEEDED)
    set(obj_copy_strip "-qlu")
  endif()

  # MWDT strip transforms input file in place with no output file option
  configure_file(${INFILE} ${OUTFILE} COPYONLY)
  execute_process(COMMAND ${STRIP} ${obj_copy_strip} ${OUTFILE})
endif()

# no support of --srec-len in mwdt

# Handle Input and Output target types
if(DEFINED OUTTARGET)
  set(obj_copy_target_output "")
  set(obj_copy_gap_fill "")
  if(GAP_FILL)
    set(obj_copy_gap_fill "-f;${GAP_FILL}")
  endif()
  # only mwdt's elf2hex supports gap fill
  if(${OUTTARGET} STREQUAL "srec")
    set(obj_copy_target_output "-m")
  elseif(${OUTTARGET} STREQUAL "ihex")
    set(obj_copy_target_output "-I")
  elseif(${OUTTARGET} STREQUAL "binary")
    set(obj_copy_target_output "-B")
  endif()
  execute_process(
      COMMAND ${ELF2HEX} ${obj_copy_gap_fill} ${obj_copy_target_output}
      -o ${OUTFILE} ${INFILE}
  )
endif()

# Handle sections, if any
# 1. Section only selection(s)
set(obj_copy_sections_only "")
if(DEFINED ONLY_SECTION)
# There could be more than one, so need to check all args.
  foreach(n RANGE ${CMAKE_ARGC})
    foreach(argument ${CMAKE_ARGV${n}})
      if(${argument} MATCHES "-DONLY_SECTION=(.*)")
        list(APPEND obj_copy_sections_only "-sn;${CMAKE_MATCH_1}")
      endif()
    endforeach()
  endforeach()

  execute_process(
      COMMAND ${ELF2BIN} -q ${obj_copy_sections_only}
      ${INFILE} ${OUTFILE}
  )
endif()

# no support of rename sections in mwdt, here just use arc-elf32-objcopy temporarily
set(obj_copy_sections_rename "")
if(DEFINED RENAME_SECTION)
  foreach(n RANGE ${CMAKE_ARGC})
    foreach(argument ${CMAKE_ARGV${n}})
      if(${argument} MATCHES "-DRENAME_SECTION=(.*)")
        list(APPEND obj_copy_sections_rename "--rename-section;${CMAKE_MATCH_1}")
      endif()
    endforeach()
  endforeach()

  execute_process(
      COMMAND ${OBJCOPY} ${obj_copy_sections_rename}
      ${INFILE} ${OUTFILE}
  )
endif()

# no support of remove sections
