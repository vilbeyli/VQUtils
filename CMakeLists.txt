cmake_minimum_required (VERSION 3.4)

project (VQUtils)

add_compile_options(/MP)
add_compile_options(/std:c++17)

set (Lib_headers
    "Libs/stb/stb_image.h"
    "Libs/stb/stb_image_resize.h"
    "Libs/stb/stb_image_write.h"
    "Libs/tinyxml2/tinyxml2.h"
    "Libs/tinyexr/tinyexr.h"
    "Libs/miniz/miniz.h"
)

set (Headers
    "Include/Log.h"
    "Include/utils.h"
    "Include/SystemInfo.h"
    "Include/Image.h"
    "Include/Timer.h"
    "Include/Multithreading/ConcurrentQueue.h"
    "Include/Multithreading/BufferedContainer.h"
    "Include/Multithreading/EventSignal.h"
    "Include/Multithreading/Semaphore.h"
    "Include/Multithreading/TaskSignal.h"
    "Include/Multithreading/ThreadPool.h"
)

set (Source
    "Source/Log.cpp"
    "Source/utils.cpp"
    "Source/Multithreading/ThreadPool.cpp"
    "Source/SystemInfo.cpp"
    "Source/Image.cpp"
    "Source/Timer.cpp"
    "Libs/tinyxml2/tinyxml2.cpp"
    "Libs/miniz/miniz.c"
)

set (IncludeDirs 
    "Include/"
)

source_group("Libs"   FILES ${Lib_headers})
set_source_files_properties(${Lib_headers} PROPERTIES VS_TOOL_OVERRIDE "Text")
add_definitions(-DNOMINMAX)

# set ouput directory
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/Bin)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${OUTPUTCONFIG} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${OUTPUTCONFIG} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${OUTPUTCONFIG} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

add_link_options(/SUBSYSTEM:WINDOWS)

add_library(${PROJECT_NAME} STATIC ${Headers} ${Source} ${Lib_headers})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${IncludeDirs})