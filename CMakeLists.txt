cmake_minimum_required(VERSION 3.16)
project(phoenix_sdr_controller VERSION 0.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

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

# Source files
set(SOURCES
    src/main.c
    src/tcp_client.c
    src/sdr_protocol.c
    src/ui_core.c
    src/ui_widgets.c
    src/ui_layout.c
    src/app_state.c
)

set(HEADERS
    include/tcp_client.h
    include/sdr_protocol.h
    include/ui_core.h
    include/ui_widgets.h
    include/ui_layout.h
    include/app_state.h
    include/common.h
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)

# Windows socket library and mingw runtime
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 mingw32)
endif()

# Copy SDL2 DLLs to output directory (for Windows)
if(WIN32)
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
