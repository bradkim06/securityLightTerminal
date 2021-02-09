# CMake에서 주석은 #을 사용해 작성합니다
set(AUTHOR_NAME "bradkim06")
cmake_minimum_required(VERSION 3.14)    # CMake 버전을 명시
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions(__MSP430F5419A__)
add_compile_definitions(__MODEM_BC95G__)
add_compile_definitions(DEBUG)
add_compile_definitions(REPEATER_DEVICE=0)
add_compile_definitions(LORA_DEVICE=0)
add_compile_definitions(NBIOT_DEVICE=1)
set(CCS_BASE_ROOT /mnt/c/ti/ccsv8/ccs_base)
set(PROJECT_ROOT /mnt/d/terminal/Source)
set(CG_TOOL_ROOT /mnt/c/ti/ccsv8/tools/compiler/ti-cgt-msp430_18.12.0.LTS)
project(securityTerminal)


file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    "*.h"
    "*.c"
    )
SET(App ${SRC})
ADD_EXECUTABLE(App ${SRC})
include_directories(
    ${CCS_BASE_ROOT}/msp430/include
    ${CG_TOOL_ROOT}/include
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/app
    ${PROJECT_ROOT}/dev
    ${PROJECT_ROOT}/misc
    ${PROJECT_ROOT}/osal
    ${PROJECT_ROOT}/test
    ${PROJECT_ROOT}/modem/nb
    ${PROJECT_ROOT}/modem/nb/uuid
    )
