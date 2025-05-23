
find_package(Git)
if(NOT Git_FOUND)
    message(WARNING "Git not found")
endif()

macro(_git)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} ${ARGN}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE _git_res
            OUTPUT_VARIABLE _git_out OUTPUT_STRIP_TRAILING_WHITESPACE ECHO_OUTPUT_VARIABLE
            ERROR_VARIABLE _git_err ERROR_STRIP_TRAILING_WHITESPACE ECHO_ERROR_VARIABLE
            )
endmacro()

message(CHECK_START "Project version")

if (DEFINED ENV{THIS_PROJECT_VERSION} AND DEFINED ENV{THIS_PROJECT_SHA})
    set(git_tag "$ENV{THIS_PROJECT_VERSION}")
    set(git_hash "$ENV{THIS_PROJECT_SHA}")
else ()
    _git(describe --tags --abbrev=0)
    set(git_tag "${_git_out}")

    _git(rev-parse --short HEAD)
    set(git_hash "${_git_out}")

endif ()

if (NOT git_tag)
    message(WARNING "Failed to get git tag. Assume it is a first run. So project version is set to 0.0.0")
    set(git_tag 0.0.0)
endif ()

if (NOT git_hash)
    message(WARNING "Failed to get git hash. Cannot setup commit hash. So project sha is set to \"nohash\"")
    set(git_hash "nohash")
endif ()

set(THIS_PROJECT_SHA ${git_hash})

# tag sanitizer
if (git_tag MATCHES "^([0-9]*).([0-9]*).([0-9]*)$")
    set(THIS_PROJECT_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3})
    set(THIS_PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(THIS_PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
    set(THIS_PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
else ()
    message(WARNING "Git tag isn't valid semantic version: [${git_tag}]\n")
    set(THIS_PROJECT_VERSION ${git_tag})
endif ()

message(CHECK_PASS "${THIS_PROJECT_VERSION}")
