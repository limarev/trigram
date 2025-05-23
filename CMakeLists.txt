cmake_minimum_required(VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(Version)

project(template
        DESCRIPTION "How template works"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 23)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_executable(template)

target_sources(template
        PRIVATE
        main.cpp)

target_include_directories(template PRIVATE ${CMAKE_BINARY_DIR}/include)

file(CONFIGURE OUTPUT include/version.h CONTENT "\
#pragma once

#define PROJECT_VERSION \"${THIS_PROJECT_VERSION}\"
#define PROJECT_SHA \"${THIS_PROJECT_SHA}\"
")

set(CPACK_PACKAGE_VERSION ${THIS_PROJECT_VERSION})

include(CPack)
include(GNUInstallDirs)
install(TARGETS template
        RUNTIME)