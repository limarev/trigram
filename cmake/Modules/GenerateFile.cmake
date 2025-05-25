# Function to generate a text file filled with repeated content
# Usage:
#   generate_repeated_text_file(
#     OUTPUT_FILE <path>
#     CONTENT <string>
#     TARGET_SIZE_MB <size>
#   )
function(generate_repeated_text_file)
    # Parse arguments
    set(options)
    set(oneValueArgs OUTPUT_FILE CONTENT TARGET_SIZE_MB)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate arguments
    if(NOT ARG_OUTPUT_FILE)
        message(FATAL_ERROR "OUTPUT_FILE argument is required")
    endif()
    if(NOT ARG_CONTENT)
        message(FATAL_ERROR "CONTENT argument is required")
    endif()
    if(NOT ARG_TARGET_SIZE_MB)
        set(ARG_TARGET_SIZE_MB 20) # Default to 20MB if not specified
    endif()

    message(STATUS "Generating ${ARG_TARGET_SIZE_MB}MB file at ${ARG_OUTPUT_FILE}")

    # Calculate required repetitions
    string(LENGTH "${ARG_CONTENT}" STR_LENGTH)
    math(EXPR TARGET_SIZE "${ARG_TARGET_SIZE_MB} * 1024 * 1024")  # Convert MB to bytes
    math(EXPR REPEAT_COUNT "${TARGET_SIZE} / ${STR_LENGTH} + 1")

    # Open file for writing
    file(WRITE "${ARG_OUTPUT_FILE}" "") # Create empty file

    # Write content in chunks to be more memory efficient
    set(BUFFER_SIZE 1000)
    math(EXPR FULL_CHUNKS "${REPEAT_COUNT} / ${BUFFER_SIZE}")
    math(EXPR REMAINDER "${REPEAT_COUNT} % ${BUFFER_SIZE}")

    # Create a buffer string
    unset(BUFFER)
    foreach(i RANGE 1 ${BUFFER_SIZE})
        string(APPEND BUFFER "${ARG_CONTENT}")
    endforeach()

    # Write full chunks
    foreach(i RANGE 1 ${FULL_CHUNKS})
        file(APPEND "${ARG_OUTPUT_FILE}" "${BUFFER}")
    endforeach()

    # Write remainder
    unset(REMAINDER_BUFFER)
    foreach(i RANGE 1 ${REMAINDER})
        string(APPEND REMAINDER_BUFFER "${ARG_CONTENT}")
    endforeach()
    file(APPEND "${ARG_OUTPUT_FILE}" "${REMAINDER_BUFFER}")

    message(STATUS "File ${ARG_OUTPUT_FILE} successfully generated")
endfunction()