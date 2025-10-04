function(add_embedded_resources target)
    foreach(in_f ${ARGN})
        get_filename_component(fname ${in_f} NAME)        # just the filename (no path)
        get_filename_component(fdir ${in_f} DIRECTORY)    # directory of the input file

        set(out_f0 "${CMAKE_CURRENT_BINARY_DIR}/${fname}.o0")
        set(out_f  "${CMAKE_CURRENT_BINARY_DIR}/${fname}.o")

        # Step 1: binary to object
        add_custom_command(OUTPUT ${out_f0}
            COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f0} ${fname}
            DEPENDS ${in_f}
            WORKING_DIRECTORY ${fdir}
            COMMENT "Embedding resource ${fname}"
        )

        # Step 2: move to .rodata
        add_custom_command(OUTPUT ${out_f}
            COMMAND ${CMAKE_OBJCOPY}
                    --rename-section .data=.rodata
                    ${out_f0} ${out_f}
            DEPENDS ${out_f0}
            COMMENT "Placing resource ${fname} in .rodata"
        )

        target_sources(${target} PRIVATE ${out_f})
    endforeach()
endfunction()
