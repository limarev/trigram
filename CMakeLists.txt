cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(Version)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(trigram
        DESCRIPTION "How template works"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 23)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_executable(trigram)

target_sources(trigram
                PRIVATE
                main.cpp
               word.cpp
               word.h
               group_if.h
               tsqueue.h)

include(GenerateFile)
generate_repeated_text_file(OUTPUT_FILE ${CMAKE_BINARY_DIR}/input.txt
                            CONTENT "A call to remove is typically followed by a call to a container's erase member function to actually remove elements from the container. These two invocations together constitute a so-called erase-remove idiom.
Для использования с UTF-8 нужно сначала декодировать строку в code points (например, с помощью функции utf8_to_codepoint из предыдущих примеров), а затем проверять каждый code point этой функцией."
                            TARGET_SIZE_MB "20")
target_include_directories(trigram PRIVATE ${CMAKE_BINARY_DIR}/include)

file(CONFIGURE OUTPUT include/version.h CONTENT "\
#pragma once

#define PROJECT_VERSION \"${THIS_PROJECT_VERSION}\"
#define PROJECT_SHA \"${THIS_PROJECT_SHA}\"
")

set(CPACK_PACKAGE_VERSION ${THIS_PROJECT_VERSION})

include(CPack)
include(GNUInstallDirs)
install(TARGETS trigram
        RUNTIME)