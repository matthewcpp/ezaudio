cmake_minimum_required(VERSION 3.0)

project(ezaudio)

option(BUILD_TEST "Builds the test application.  Requires the use of conan to pull TinySoundFont." ON)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS TRUE)
endif()

set(common_src include/ezaudio/ezaudio.h src/common.cpp)

if (WIN32)
    set(platform_src src/wsapi.cpp)
    set(platform_libs setupapi.lib winmm.lib imm32.lib version.lib)
elseif(MACOS)
    set(platform_src src/coreaudio.cpp)
    set(platform_libs "-framework AudioToolbox")
endif()

add_library(ezaudio STATIC ${common_src} ${platform_src})
target_include_directories(ezaudio PUBLIC include)

set_target_properties(ezaudio PROPERTIES 
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON)

if (BUILD_TEST)
    message("Configuring test application.")

    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
    find_package(TinySoundFont REQUIRED)

    add_executable(test_ezaudio test/test_ezaudio.cpp)

    set(sample_font_path ${CMAKE_BINARY_DIR}/florestan-subset.sf2)
    if (NOT EXISTS ${sample_font_path})
        message("Downloading sample soundfont file.")
        set(sample_font_url "https://github.com/schellingb/TinySoundFont/raw/master/examples/florestan-subset.sf2")
        file(DOWNLOAD ${sample_font_url} ${sample_font_path})
    endif()

    target_compile_definitions(test_ezaudio PRIVATE SOUNDFONT_FILE=${sample_font_path})

    target_link_libraries(test_ezaudio PRIVATE ezaudio TinySoundFont::TinySoundFont ${platform_libs})
endif()

