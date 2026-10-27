cmake_minimum_required(VERSION 3.15)
project(FLTube CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(CURL REQUIRED)

# FLTK finding
find_package(FLTK REQUIRED NO_MODULE)

# Check platform
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # Windows specific libraries
    set(PLATFORM_LIBRARIES ws2_32 crypt32 wsock32 winmm ole32 shell32)
    
    # Default paths for Windows
    set(SYSTEM_CONFIGFILE_PATH "fltube.conf")
    set(RESOURCES_PATH "resources")
else()
    set(PLATFORM_LIBRARIES pthread)
    set(SYSTEM_CONFIGFILE_PATH "/usr/local/etc/fltube/fltube.conf")
    set(RESOURCES_PATH "/usr/local/share/fltube/resources")
endif()

# Source files
set(SOURCES
    src/FLTube.cxx
    src/FLTube_View.cxx
    src/cache.cxx
    src/configuration_manager.cxx
    src/custom_widgets.cxx
    src/file_manipulation_examples.cxx
    src/fltube_utils.cxx
    src/gnugettext_utils.cxx
    src/userdata_manager.cxx
    src/ytdlp_helper.cxx
)

add_executable(FLTube WIN32 ${SOURCES})

target_include_directories(FLTube PRIVATE include ${FLTK_INCLUDE_DIRS})

target_link_libraries(FLTube PRIVATE 
    ${FLTK_LIBRARIES} 
    CURL::libcurl
    ${PLATFORM_LIBRARIES}
)

if(WIN32)
    # Copy resources and config for Windows convenience
    add_custom_command(TARGET FLTube POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:FLTube>/resources
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/fltube.conf $<TARGET_FILE_DIR:FLTube>/fltube.conf
    )
endif()
