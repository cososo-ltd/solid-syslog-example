# T1 baseline device image — FreeRTOS + lwIP + mbedTLS + FatFs on QEMU
# mps2-an385 (Cortex-M3), with ZERO SolidSyslog. Built with the `baseline-cross`
# preset inside the ghcr.io/cososo-ltd/cpputest-freertos-cross container, which
# provides arm-none-eabi-gcc, qemu-system-arm, and the upstream dependency trees
# at the FREERTOS_KERNEL_PATH / LWIP_PATH / MBEDTLS_DIR / FATFS_PATH env paths.
#
# Layout mirrors the proven solid-syslog Bdd/Targets/FreeRtosLwip build: the
# upstream + third-party sources compile into an OBJECT library under a relaxed
# warning set; our own app code links them under the strict bar. The example
# consumes the container's dependency trees — it never vendors FreeRTOS, lwIP,
# mbedTLS, or FatFs.

cmake_minimum_required(VERSION 3.25)
project(solid_syslog_example_baseline C ASM)

set(APP_DIR ${CMAKE_SOURCE_DIR}/app)

# --- container-provided upstream dependency trees -----------------------------

set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}")
if(NOT FREERTOS_KERNEL_PATH)
    message(FATAL_ERROR "FREERTOS_KERNEL_PATH not set — build inside the cpputest-freertos-cross container.")
endif()

set(LWIP_DIR "$ENV{LWIP_PATH}")
if(NOT LWIP_DIR)
    message(FATAL_ERROR "LWIP_PATH not set — build inside the cpputest-freertos-cross container.")
endif()

set(MBEDTLS_SOURCE_DIR "$ENV{MBEDTLS_DIR}")
if(NOT EXISTS "${MBEDTLS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "MBEDTLS_DIR not set or invalid — build inside the cpputest-freertos-cross container.")
endif()

set(FATFS_PATH "$ENV{FATFS_PATH}")
if(NOT FATFS_PATH)
    message(FATAL_ERROR "FATFS_PATH not set — build inside the cpputest-freertos-cross container.")
endif()

set(FREERTOS_PORT_DIR "${FREERTOS_KERNEL_PATH}/portable/GCC/ARM_CM3")
set(LWIP_CONTRIB_FREERTOS_DIR "${LWIP_DIR}/contrib/ports/freertos")

# --- SolidSyslog ---------------------------------------------------------------
#
# Pulled in the way an integrator does. FetchContent is the documented route; it
# nests the library under this build (MakeAvailable calls add_subdirectory), so
# the library's own tests, BDD targets, and developer checks stay out of ours.
#
# Pinned to a commit because the library has no release tag yet. Swap this for a
# version tag at 0.1.0. To try an unreleased library checkout without editing
# this file:
#
#   cmake --preset baseline-cross -DFETCHCONTENT_SOURCE_DIR_SOLIDSYSLOG=/path/to/solid-syslog
#
# Name the platforms, rather than letting the library infer them from whatever
# the build environment happens to export. Atomics for the sequence counter,
# FreeRtos for the mutex the circular buffer needs, FatFs for the store's files,
# MbedTls for the TLS stream.
# The list is authoritative — anything unnamed is off, including the host platforms
# — so this one line is the whole selection. The configure echoes back which roles
# it filled.
set(SOLIDSYSLOG_PLATFORMS "LwipRaw;Atomics;FreeRtos;FatFs;MbedTls" CACHE STRING "" FORCE)

# Compile-time limits. Every tunable is #ifndef-guarded, so this file only needs
# the ones this device wants changed.
set(SOLIDSYSLOG_USER_TUNABLES_FILE "${APP_DIR}/config/solid_syslog_tunables.h")

include(FetchContent)
FetchContent_Declare(SolidSyslog
    GIT_REPOSITORY https://github.com/cososo-ltd/solid-syslog.git
    GIT_TAG        e4bc54d23d8b46c7479f319886bc775e4e8f3fbd
)
FetchContent_MakeAvailable(SolidSyslog)

# --- stage FatFs so ff.h's `#include "ffconf.h"` resolves to our integrator config
# GCC's "" lookup checks the including file's own directory first, so colocating
# the upstream ff.c/ff.h/diskio.h with our ffconf.h is how our config wins.
set(FATFS_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/fatfs")
configure_file(${FATFS_PATH}/source/ff.c     ${FATFS_STAGE_DIR}/ff.c     COPYONLY)
configure_file(${FATFS_PATH}/source/ff.h     ${FATFS_STAGE_DIR}/ff.h     COPYONLY)
configure_file(${FATFS_PATH}/source/diskio.h ${FATFS_STAGE_DIR}/diskio.h COPYONLY)
configure_file(${APP_DIR}/config/ffconf.h    ${FATFS_STAGE_DIR}/ffconf.h COPYONLY)

# lwIP source lists (IPv4 core + api set for the NO_SYS=0 tcpip thread).
include(${LWIP_DIR}/src/Filelists.cmake)

# --- upstream / third-party OBJECT library (relaxed warnings) ------------------
add_library(baseline_upstream OBJECT
    ${FREERTOS_KERNEL_PATH}/tasks.c
    ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c
    ${FREERTOS_KERNEL_PATH}/timers.c
    ${FREERTOS_KERNEL_PATH}/event_groups.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${lwipcore_SRCS}
    ${lwipcore4_SRCS}
    ${lwipapi_SRCS}
    ${LWIP_DIR}/src/netif/ethernet.c
    ${LWIP_CONTRIB_FREERTOS_DIR}/sys_arch.c
    ${APP_DIR}/net/smsc9220/smsc9220_eth_drv.c
    ${FATFS_STAGE_DIR}/ff.c
    ${APP_DIR}/storage/ffsystem.c
)

target_compile_options(baseline_upstream PRIVATE
    -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -fno-common
    -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-implicit-fallthrough
    -Wno-array-bounds -Wno-conversion -Wno-sign-conversion -Wno-shadow -Wno-pedantic
)

target_include_directories(baseline_upstream PRIVATE
    ${APP_DIR}/config                       # FreeRTOSConfig.h, lwipopts.h, arch/cc.h
    ${APP_DIR}/net/smsc9220                  # smsc9220_eth_drv.h, smsc9220_emac_config.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}                     # portmacro.h
    ${LWIP_DIR}/src/include
    ${LWIP_CONTRIB_FREERTOS_DIR}/include     # arch/sys_arch.h
    ${FATFS_STAGE_DIR}                       # ff.h, diskio.h + integrator ffconf.h
)

# --- mbedTLS as a cross-compiled subproject -----------------------------------
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
add_subdirectory(${MBEDTLS_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/_mbedtls EXCLUDE_FROM_ALL)

# MBEDTLS_USER_CONFIG_FILE is consumed via #include, so the value must be a
# quoted C string literal that survives the shell -> compiler hop.
set(MBEDTLS_USER_CONFIG_HEADER "\"${APP_DIR}/config/mbedtls_user_config.h\"")

foreach(_mbedtls_target IN ITEMS mbedtls mbedx509 mbedcrypto everest p256m)
    if(TARGET ${_mbedtls_target})
        target_compile_options(${_mbedtls_target} PRIVATE -w)
        target_compile_definitions(${_mbedtls_target} PRIVATE MBEDTLS_USER_CONFIG_FILE=${MBEDTLS_USER_CONFIG_HEADER})
        set_target_properties(${_mbedtls_target} PROPERTIES C_CLANG_TIDY "" C_CPPCHECK "")
    endif()
endforeach()

# --- baseline executable (our code, strict warnings) --------------------------
add_executable(baseline
    ${APP_DIR}/main.c
    ${APP_DIR}/sim/SimulatedExistingApp.c
    ${APP_DIR}/tasks/LogTask.c
    ${APP_DIR}/tasks/ServiceTask.c
    ${APP_DIR}/measure/Measure.c
    ${APP_DIR}/measure/Baseline.c
    ${APP_DIR}/platform/Startup.c
    ${APP_DIR}/platform/CmsdkUart.c
    ${APP_DIR}/platform/DeviceClock.c
    ${APP_DIR}/platform/DeviceCertStore.c
    ${APP_DIR}/platform/Syscalls.c
    ${APP_DIR}/platform/SemihostingExit.c
    ${APP_DIR}/platform/SemihostingIo.c
    ${APP_DIR}/net/EthernetIf.c
    ${APP_DIR}/storage/diskio.c
    ${APP_DIR}/storage/SemihostingDisk.c
    ${APP_DIR}/syslog/Syslog.c
    ${APP_DIR}/syslog/SyslogFields.c
    ${APP_DIR}/syslog/SyslogErrorHandler.c
    $<TARGET_OBJECTS:baseline_upstream>
)

set_target_properties(baseline PROPERTIES SUFFIX ".elf")

target_compile_options(baseline PRIVATE
    -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -fno-common
    -Wall -Wextra
)

target_include_directories(baseline PRIVATE
    ${APP_DIR}                               # AppConfig.h
    ${APP_DIR}/config                        # FreeRTOSConfig.h, lwipopts.h, arch/cc.h
    ${APP_DIR}/sim                           # SimulatedExistingApp.h
    ${APP_DIR}/tasks                         # LogTask.h, ServiceTask.h
    ${APP_DIR}/measure                       # Measure.h, Baseline.h
    ${APP_DIR}/net                           # EthernetIf.h
    ${APP_DIR}/net/smsc9220
    ${APP_DIR}/platform                      # CmsdkUart.h, SemihostingExit.h, SemihostingIo.h
    ${APP_DIR}/storage                       # SemihostingDisk.h
    ${APP_DIR}/syslog                        # SyslogErrorHandler.h
    ${FREERTOS_KERNEL_PATH}/include
    ${FREERTOS_PORT_DIR}
    ${LWIP_DIR}/src/include
    ${LWIP_CONTRIB_FREERTOS_DIR}/include
    ${FATFS_STAGE_DIR}
)

# sim/SimulatedExistingApp.c includes <mbedtls/ssl.h> — it MUST see the same user
# config as the library, or context struct sizes diverge between consumer and library.
target_compile_definitions(baseline PRIVATE MBEDTLS_USER_CONFIG_FILE=${MBEDTLS_USER_CONFIG_HEADER})

# SolidSyslog is the Core library (a real .a); the SolidSyslog::* packs are
# *source* packs — linking one compiles its adapters into this target, so they see
# our lwipopts.h and our warning bar, which is the point of a header-configured
# upstream. Only those have a link target: a platform over a stable system API,
# like Atomics, is already inside libSolidSyslog.a and is named but never linked.
target_link_libraries(baseline PRIVATE mbedtls mbedx509 mbedcrypto SolidSyslog SolidSyslog::LwipRaw SolidSyslog::FreeRtos SolidSyslog::FatFs SolidSyslog::MbedTls)

target_link_options(baseline PRIVATE
    -mcpu=cortex-m3 -mthumb
    -T${APP_DIR}/platform/mps2-an385.ld
    --specs=nano.specs --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE:baseline>.map
)

set_target_properties(baseline PROPERTIES LINK_DEPENDS ${APP_DIR}/platform/mps2-an385.ld)
