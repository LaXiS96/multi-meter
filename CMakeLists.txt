cmake_minimum_required(VERSION 3.20)

project(multi_meter C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    lib/*.c
    sdk/*.c
    sdk/*.S
    src/*.c
)

add_executable(firmware ${SOURCES})

target_include_directories(firmware PRIVATE
    include
    lib
    sdk/Core
    sdk/Debug
    sdk/Peripheral/inc
    sdk/User
)

set(FLAGS
    -march=rv32ec_xw
    -mabi=ilp32e
    -specs=nano.specs
    -specs=nosys.specs
    -flto
    -ffunction-sections
    -fdata-sections
)

target_compile_options(firmware PRIVATE
    ${FLAGS}
    -Og
    -g
    -fno-builtin
    -include "${CMAKE_SOURCE_DIR}/config.h"
    -Wall
    -Wno-misleading-indentation
)

target_link_options(firmware PRIVATE
    ${FLAGS}
    -T${CMAKE_SOURCE_DIR}/sdk/Ld/Link.ld
    -nostartfiles
    "-Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map,--print-memory-usage"
    -Wl,--gc-sections
)

set_target_properties(firmware PROPERTIES
    SUFFIX ".elf"
)

add_custom_target(flash
    COMMAND wlink flash --enable-sdi-print $<TARGET_FILE:firmware>
)

add_custom_target(erase
    COMMAND wlink erase --chip CH32V003 --method power-off
)

add_custom_target(enable-sdi
    COMMAND wlink sdi-print enable
)
