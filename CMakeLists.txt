cmake_minimum_required(VERSION 3.16)
project(FramePassthroughShortcuts VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/bin")
endforeach()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit builds are supported.")
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    message(FATAL_ERROR "Build for Linux ARM64 (Steam Frame).")
endif()

include(cmake/OpenVRLoader.cmake)
find_package(Threads REQUIRED)

add_library(fps_core STATIC src/config.cpp src/controller.cpp)
target_include_directories(fps_core PUBLIC src third_party/json)
target_link_libraries(fps_core PUBLIC openvr_loader Threads::Threads)
target_compile_definitions(fps_core PUBLIC FPS_VERSION="${PROJECT_VERSION}")

add_executable(frame-passthrough-shortcuts src/main.cpp)
target_link_libraries(frame-passthrough-shortcuts PRIVATE fps_core)
    target_compile_options(fps_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(frame-passthrough-shortcuts PRIVATE -Wall -Wextra -Wpedantic)
    # Keep the device bundle independent of the compiler's libstdc++ version.
    target_link_options(frame-passthrough-shortcuts PRIVATE -static-libgcc -static-libstdc++)


add_custom_command(TARGET frame-passthrough-shortcuts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:frame-passthrough-shortcuts>"
    COMMENT "Copying configuration and SteamVR bindings")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # SteamVR identifies clients by executable path, even Utility clients.
    # Management commands need a distinct real executable to avoid taking the
    # resident shortcut application's process identity.
    add_custom_command(TARGET frame-passthrough-shortcuts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:frame-passthrough-shortcuts>"
            "$<TARGET_FILE_DIR:frame-passthrough-shortcuts>/frame-passthrough-control"
        COMMENT "Copying SteamVR management companion")
endif()

install(TARGETS frame-passthrough-shortcuts RUNTIME DESTINATION .)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(PROGRAMS "$<TARGET_FILE:frame-passthrough-shortcuts>"
        DESTINATION . RENAME frame-passthrough-control)
endif()
install(DIRECTORY assets/ DESTINATION .)
install(FILES README.md LICENSE DESTINATION .)
