cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(dfsMaze C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()



# Add executable. Default name is the project name, version 0.1
add_executable(dfsMaze dfsMaze.cpp VL53L0X.cpp)

# Link the VL53L0X library to the dfsMaze executable
target_link_libraries(dfsMaze
        pico_stdlib
        hardware_i2c
        hardware_pio
        )

# Set program name and version
pico_set_program_name(dfsMaze "dfsMaze")
pico_set_program_version(dfsMaze "0.1")

# Generate PIO header
pico_generate_pio_header(dfsMaze ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(dfsMaze 0)
pico_enable_stdio_usb(dfsMaze 1)

# Add the standard include files to the build
target_include_directories(dfsMaze PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Add any extra outputs
pico_add_extra_outputs(dfsMaze)
