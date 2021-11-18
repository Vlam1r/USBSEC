cmake_minimum_required(VERSION 3.17)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_firmware
        VERSION 0.1.0
        DESCRIPTION "USBSEC Firmware for RPi Pico"
        LANGUAGES C CXX ASM)

pico_sdk_init()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall
        -Wno-format
        -Wno-unused-function
        -Wno-maybe-uninitialized
        )

add_executable(pico_firmware
        src/main_spi_tusb.c
        src/lib/messages/messages.c
        src/lib/rp2040/dcd_rp2040.c
        src/lib/rp2040/hcd_rp2040.c
        src/lib/rp2040/rp2040_usb.c
        src/lib/usb_event_handlers/device_event_handlers.c
        src/lib/usb_event_handlers/host_event_handlers.c
        src/lib/event_queue/event_queue.c
        src/lib/edpt_registry/edpt_registry.c
        src/lib/debug/debug.c)


#pico_generate_pio_header(pico_firmware ${CMAKE_CURRENT_LIST_DIR}/src/lib/i2c/i2c.pio)
#pico_generate_pio_header(pico_firmware ${CMAKE_CURRENT_LIST_DIR}/src/lib/usb_pio/usb.pio)

target_link_libraries(pico_firmware
        pico_stdlib
        pico_multicore
        hardware_resets
        hardware_irq
        #hardware_pio
        hardware_spi
        )

pico_enable_stdio_usb(pico_firmware 1)
pico_enable_stdio_uart(pico_firmware 1)

pico_add_extra_outputs(pico_firmware)

#add_subdirectory(src/test)
