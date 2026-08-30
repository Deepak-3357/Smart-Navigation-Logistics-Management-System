cmake_minimum_required(VERSION 3.20)
project(SmartLogistics C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find Raylib
find_package(raylib REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.c
    src/theme_manager.c
    src/window_manager.c
    src/screen_manager.c
    src/ui_manager.c
    src/components.c
    src/warehouse.c
    src/package.c
    src/route.c
    src/font_manager.c
    src/screens/dashboard.c
    src/screens/warehouse.c
    src/screens/packages.c
    src/screens/routes.c
    src/screens/analytics.c
    src/screens/settings.c
)

# Executable
add_executable(smart_logistics ${SOURCES})

# Target linking
target_link_libraries(smart_logistics PRIVATE raylib)

# Package the exact UCRT64 Raylib runtime beside the executable so it can
# launch from normal Windows without requiring /ucrt64/bin on PATH.
get_target_property(RAYLIB_RUNTIME_DLL raylib IMPORTED_LOCATION_RELEASE)
if(RAYLIB_RUNTIME_DLL)
    get_filename_component(RAYLIB_BIN_DIR "${RAYLIB_RUNTIME_DLL}" DIRECTORY)
    set(RAYLIB_RUNTIME_DLLS "${RAYLIB_RUNTIME_DLL}")
    if(EXISTS "${RAYLIB_BIN_DIR}/glfw3.dll")
        list(APPEND RAYLIB_RUNTIME_DLLS "${RAYLIB_BIN_DIR}/glfw3.dll")
    endif()

    foreach(RUNTIME_DLL IN LISTS RAYLIB_RUNTIME_DLLS)
        add_custom_command(TARGET smart_logistics POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${RUNTIME_DLL}"
                    "$<TARGET_FILE_DIR:smart_logistics>"
            VERBATIM
        )
    endforeach()
endif()

# Compiler flags for GCC/Clang
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(smart_logistics PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()
