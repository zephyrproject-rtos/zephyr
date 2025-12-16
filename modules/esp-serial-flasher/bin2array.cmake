# Creates C resources file from files in given directory recursively
function(create_resources dir output)
    message(STATUS "[bin2array] Creating resources from directory: ${dir}")
    message(STATUS "[bin2array] Output file: ${output}")

    # Check if directory exists
    if(NOT EXISTS ${dir})
        message(WARNING "[bin2array] Directory does not exist: ${dir}")
        return()
    endif()

    # Create empty output file
    file(WRITE ${output} "#include <stdint.h>\n\n")
    # Collect input files
    file(GLOB bin_paths ${dir}/*)

    # Iterate through input files
    foreach(bin ${bin_paths})
        message(STATUS "[bin2array] Processing file: ${bin}")
        # Get short filenames, by discarding relative path
        get_filename_component(name ${bin} NAME)
        message(STATUS "[bin2array]   File name: ${name}")
        # Replace filename spaces & extension separator for C compatibility
        string(REGEX REPLACE "[\\./-]" "_" filename ${name})
        message(STATUS "[bin2array]   Variable name: ${filename}")

        # Check if file exists and is readable
        if(NOT EXISTS ${bin})
            message(WARNING "[bin2array]   File does not exist: ${bin}")
            continue()
        endif()

        # Read hex data from file
        file(READ ${bin} filedata HEX)
        # Convert hex data for C compatibility
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
        # Compute MD5 hash of the file
        execute_process(COMMAND md5sum ${bin} OUTPUT_VARIABLE md5_output)
        string(REGEX REPLACE " .*" "" md5_hash ${md5_output})
        message(STATUS "[bin2array]   MD5: ${md5_hash}")
        # Append data to output file
        file(APPEND ${output}
            "const uint8_t  ${filename}[] = {${filedata}};\n"
            "const uint32_t ${filename}_size = sizeof(${filename});\n"
            "const uint8_t  ${filename}_md5[] = \"${md5_hash}\";\n"
        )
    endforeach()
endfunction()

# Main execution when run as script
if(INPUT_DIR AND OUTPUT_FILE)
    create_resources(${INPUT_DIR} ${OUTPUT_FILE})
endif()
