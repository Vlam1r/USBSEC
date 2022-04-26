cmake_minimum_required(VERSION 3.17)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_firmware
        VERSION 0.3.0
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

add_executable(pico_firmware_master
        src/main.c
        src/lib/messages/messages.c
        src/lib/rp2040/dcd_rp2040.c
        src/lib/rp2040/hcd_rp2040.c
        src/lib/rp2040/rp2040_usb.c
        src/lib/usb_event_handlers/device_event_handlers.c
        src/lib/usb_event_handlers/host_event_handlers.c
        src/lib/debug/debug.c
        src/lib/messages/spi_data.c
        src/lib/setup_response_aggregator/setup_response_aggregator.c
        src/lib/drivers/drivers.c src/lib/validator/validator.c src/lib/validator/validator.h src/lib/direction_ctrl/direction_ctrl.c src/lib/direction_ctrl/direction_ctrl.h)

add_executable(pico_firmware_slave
        src/main.c
        src/lib/messages/messages.c
        src/lib/rp2040/dcd_rp2040.c
        src/lib/rp2040/hcd_rp2040.c
        src/lib/rp2040/rp2040_usb.c
        src/lib/usb_event_handlers/device_event_handlers.c
        src/lib/usb_event_handlers/host_event_handlers.c
        src/lib/debug/debug.c
        src/lib/messages/spi_data.c
        src/lib/setup_response_aggregator/setup_response_aggregator.c
        src/lib/drivers/drivers.c src/lib/validator/validator.c src/lib/validator/validator.h src/lib/direction_ctrl/direction_ctrl.c src/lib/direction_ctrl/direction_ctrl.h)

target_link_libraries(pico_firmware_master
        pico_stdlib
        pico_multicore
        hardware_resets
        hardware_irq
        hardware_spi
        )

target_link_libraries(pico_firmware_slave
        pico_stdlib
        pico_multicore
        hardware_resets
        hardware_irq
        hardware_spi
        )

pico_enable_stdio_usb(pico_firmware_master 1)
pico_enable_stdio_usb(pico_firmware_slave 1)
pico_enable_stdio_uart(pico_firmware_master 1)
pico_enable_stdio_uart(pico_firmware_slave 0)

pico_add_extra_outputs(pico_firmware_master)
pico_add_extra_outputs(pico_firmware_slave)

target_compile_definitions(pico_firmware_master PUBLIC __IS_MASTER__)
target_compile_definitions(pico_firmware_master PUBLIC __DEBUG_LEVEL__=1)
