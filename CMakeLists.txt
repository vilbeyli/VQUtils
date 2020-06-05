cmake_minimum_required (VERSION 3.4)
set(CMAKE_GENERATOR_PLATFORM x64)

project (VQUtils)

add_compile_options(/MP)
add_compile_options(/std:c++17)

set (Headers
    "Source/Log.h"
    "Source/utils.h"
)

set (Source
    "Source/Log.cpp"
    "Source/utils.cpp"
)

# set ouput directory
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/Bin)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${OUTPUTCONFIG} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${OUTPUTCONFIG} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${OUTPUTCONFIG} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

add_link_options(/SUBSYSTEM:WINDOWS)

add_library(${PROJECT_NAME} STATIC ${Headers} ${Source} )