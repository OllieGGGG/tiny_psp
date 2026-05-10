cmake_minimum_required(VERSION 3.13...3.27)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)

project(tiny_psp)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(tiny_psp
    main.c
    buttons.c
    display.c
    st7789.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(tiny_psp pico_stdlib)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(tiny_psp)

# pull in common dependencies
target_link_libraries(tiny_psp pico_stdlib hardware_pwm hardware_spi hardware_dma)

# enable usb output, disable uart output
pico_enable_stdio_usb(tiny_psp 1)
pico_enable_stdio_uart(tiny_psp 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(tiny_psp)


