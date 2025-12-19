cmake_minimum_required(VERSION 3.16)
project(phoenix_sdr_controller VERSION 0.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Try to find SDL2 via find_package (works with vcpkg, system installs, etc.)
find_package(SDL2 QUIET)
find_package(SDL2_ttf QUIET)

if(SDL2_FOUND AND SDL2_ttf_FOUND)
    # Using vcpkg or system-installed SDL2
    message(STATUS "Using SDL2 from find_package()")
    set(USE_VCPKG_SDL2 TRUE)
else()
    # Fallback to hardcoded MinGW paths (local development)
    message(STATUS "SDL2 not found via find_package, using hardcoded MinGW paths")
    set(USE_VCPKG_SDL2 FALSE)
    
    # SDL2 paths (MinGW x86_64)
    set(SDL2_DIR "D:/claude_sandbox/phoenix_sdr/libs/SDL2/SDL2-2.30.9/x86_64-w64-mingw32")
    set(SDL2_TTF_DIR "D:/claude_sandbox/phoenix_sdr/libs/SDL2_ttf/SDL2_ttf-2.22.0/x86_64-w64-mingw32")

    # SDL2 includes and libraries
    set(SDL2_INCLUDE_DIRS "${SDL2_DIR}/include/SDL2")
    set(SDL2_LIBRARIES "${SDL2_DIR}/lib/libSDL2.dll.a" "${SDL2_DIR}/lib/libSDL2main.a")
    set(SDL2_DLL "${SDL2_DIR}/bin/SDL2.dll")

    # SDL2_ttf includes and libraries
    set(SDL2_TTF_INCLUDE_DIRS "${SDL2_TTF_DIR}/include/SDL2")
    set(SDL2_TTF_LIBRARIES "${SDL2_TTF_DIR}/lib/libSDL2_ttf.dll.a")
    set(SDL2_TTF_DLL "${SDL2_TTF_DIR}/bin/SDL2_ttf.dll")
endif()

# Source files
set(SOURCES
    src/main.c
    src/tcp_client.c
    src/sdr_protocol.c
    src/ui_core.c
    src/ui_widgets.c
    src/ui_layout.c
    src/ui_layout_debug.c
    src/ui_layout_edit.c
    src/app_state.c
    src/process_manager.c
    src/udp_telemetry.c
    src/aff.c
    src/bdc/bcd_decoder.c
)

set(HEADERS
    include/tcp_client.h
    include/sdr_protocol.h
    include/ui_core.h
    include/ui_widgets.h
    include/ui_layout.h
    include/app_state.h
    include/common.h
    include/process_manager.h
    include/udp_telemetry.h
    include/aff.h
)

# Windows resource file (icon)
if(WIN32)
    set(RESOURCE_FILE resources/phoenix_sdr.rc)
endif()

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCE_FILE})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

if(USE_VCPKG_SDL2)
    # Link using modern CMake targets from vcpkg
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        SDL2::SDL2
        SDL2::SDL2main
        SDL2_ttf::SDL2_ttf
    )
else()
    # Link using hardcoded paths
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
    )
endif()

# Windows socket library and mingw runtime
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    if(NOT USE_VCPKG_SDL2)
        # MinGW runtime only needed for hardcoded MinGW build
        target_link_libraries(${PROJECT_NAME} PRIVATE mingw32)
    endif()
endif()

# Copy SDL2 DLLs to output directory (for Windows, hardcoded path only)
# vcpkg handles DLLs automatically via install rules
if(WIN32 AND NOT USE_VCPKG_SDL2)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_TTF_DLL}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Install target
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

message(STATUS "Phoenix SDR Controller v${PROJECT_VERSION}")
