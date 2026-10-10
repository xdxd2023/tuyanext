cmake_minimum_required(VERSION 3.0.2)
project(tuyanext)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(CMAKE_VERSION VERSION_LESS 3.7)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/3.7")
endif()

if (NOT DEFINED ENV{COMMON_DIR})
	message(FATAL_ERROR "Please set COMMON_DIR first")
else()
	message(STATUS "COMMON_DIR is $ENV{COMMON_DIR}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD 11)  
add_link_options("LINKER:-rpath-link,$ENV{COMMON_DIR}/lib")

find_package(spdlog REQUIRED)
find_package(lcm REQUIRED)

#find_package(GLib2 REQUIRED)

function(add_subdirectories_recursively dir)
    file(GLOB children RELATIVE ${dir} ${dir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${dir}/${child})
            include_directories(${dir}/${child})
            message(STATUS "include dir: ${dir}/${child}")
            add_subdirectories_recursively(${dir}/${child})
        endif()
    endforeach()
endfunction()

# 设置 TUYAOS 变量指向 SDK 路径
#option(ENABLE_RK3326_TUYAOS "r328" ON)
if(ENABLE_RK3326_TUYAOS)
    set(TUYAOS ${CMAKE_SOURCE_DIR}/RK3326-TuyaOS)
    add_compile_definitions(RK3326_TUYAOS)
    file(GLOB TUYA_APP_SOURCE "next/src/tuya_apps/rvc/rk3326/*.c" "next/src/tuya_apps/rvc/rk3326/*.cpp")
    include_directories(next/src/tuya_apps/rvc/rk3326/include)
elseif(ENABLE_R328_TUYAOS)
    set(TUYAOS ${CMAKE_SOURCE_DIR}/R328_TuyaOS-3.9.1/software/TuyaOS)
    add_compile_definitions(R328_TUYAOS)
    file(GLOB TUYA_APP_SOURCE "next/src/tuya_apps/rvc/r328/*.c" "next/src/tuya_apps/rvc/r328/*.cpp")
    include_directories(next/src/tuya_apps/rvc/r328/include)
    add_subdirectories_recursively(${TUYAOS}/vendor/arm-openwrt-linux-gnueabi-6.4.1/tuyaos/tuyaos_adapter/include)
elseif(ENABLE_RV1106_TUYAOS)
    set(TUYAOS ${CMAKE_SOURCE_DIR}/RV1106_TuyaOS-3.9.1/software/TuyaOS)
    add_compile_definitions(RV1106_TUYAOS)
    file(GLOB TUYA_APP_SOURCE "next/src/tuya_apps/rvc/rv1106/*.c" "next/src/tuya_apps/rvc/rv1106/*.cpp")
    include_directories(next/src/tuya_apps/rvc/rv1106/include)
    add_subdirectories_recursively(${TUYAOS}/vendor/arm-rv1106-linux-gnueabihf-gcc-10.4.0/tuyaos/tuyaos_adapter/include)
    # set(LIBC_TYPE "glibc")
    # set(TYPE "lib")
    # add_subdirectory(ai)
    # link_directories(ai/lib/${LIBC_TYPE})
    # include_directories(ai/include)
else()
    message(FATAL_ERROR "Please specify a TUYAOS version by setting ENABLE_XXXX_TUYAOS to ON.")
endif() 
#set(TUYAOS ${CMAKE_SOURCE_IR}/RK3326-TuyaOS)
#set(TUYADEMO ${TUYAOS}/apps/tuyaos_demo_robot/application_components)

include_directories(${TUYAOS}/include)
#include_directories(${TUYADEMO}/include)
#include_directories(${TUYADEMO}/src)
add_subdirectories_recursively(${TUYAOS}/include)
add_subdirectories_recursively(${TUYAOS}/vendor/tuyaos/tuyaos_adapter/include)
add_subdirectories_recursively(${TUYAOS}/tuyaos/tuyaos_adapter/include)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
include_directories(next/include)
include_directories(next/src/bluez/includes)
include_directories(next/src/bluez/includes/src/shared)
include_directories(next/src/bluez/includes/lib)
include_directories(next/src/Bluez_ble)
add_subdirectory(next/src/bluez/libbluetooth)
add_subdirectory(next/src/bluez/libshared)
# add_subdirectory(apps/src/rkcamera)
# add_subdirectory(apps/src/rksound)


file(GLOB WIFI_SOURCES "apps/src/wifi/wifi/*.c")
file(GLOB BLE_SOURCES "next/src/Bluez_ble/*.c")
file(GLOB LZ4_SOURCES "next/src/lz4/*.c")


add_compile_definitions(OS_NET_LINUX_OS)
add_compile_definitions(TY_BT_MOD=1)
#add_compile_definitions(RK_DEVICEIO)

link_directories(${TUYAOS}/libs)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/nl/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/lib64/bluez)
link_directories($ENV{COMMON_DIR}/lib)

add_executable(${PROJECT_NAME}  next/src/tuyanext.cpp
                                next/src/tuyaos.cpp
                                next/src/tuya_utils.cpp
                                next/src/tuya_robot.cpp
                                next/src/tuya_sweeper.cpp
                                next/src/tuya_dp_utils.cpp
                                next/src/tuya_dp_function.cpp
                                next/src/tuya_pet.cpp
                                next/src/tuya_airbot.cpp
                                next/src/tuya_enums.cpp
                                next/src/tkl_wifi.c
                                next/src/tkl_bluetooth.c
                                ${LZ4_SOURCES}
                                ${BLE_SOURCES}
                                ${TUYA_APP_SOURCE}
                                # ${WIFI_SOURCES}
                                # ${TUYA_UTILS_SOURCES}
                                # ${TUYA_INIT_SOURCES}
                                # ${TUYA_PORTING_LINUX_SOURCES}
)

#用于删除某编译选项，例如 -O3,(关闭 -O3 优化后，崩溃时可提供更详细的调用栈信息)
# get_target_property(COMPILE_FLAGS ${PROJECT_NAME} COMPILE_OPTIONS )
# list(REMOVE_ITEM COMPILE_FLAGS -O3)
# set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "${COMPILE_FLAGS}" )

if(ENABLE_RV1106_TUYAOS)
# ai
# target_link_libraries(${PROJECT_NAME} ai)
# target_link_libraries(${PROJECT_NAME}
#                     ${OpenCV_LIBS}
#                     rknnmrt
#                     Threads::Threads
#                     sample_comm
#                     rockit
#                     rockchip_mpp
#                     rkaiq
#                     pthread
#                     rtsp
#                     stdc++
#                     func_none
#                     c
#                     m
#                     rt
#                     dl
#                     rga
#                     )
endif()

# spdlog
target_link_libraries(${PROJECT_NAME} spdlog::spdlog_header_only)
# lcm
target_link_libraries(${PROJECT_NAME} lcm)
# tuya
target_link_libraries(${PROJECT_NAME} tuyaos)
target_link_libraries(${PROJECT_NAME} -Wl,--no-whole-archive tuyaos_adapter)
# glib2
#target_link_libraries(${PROJECT_NAME} GLib2::glib)
# bluez
target_link_libraries(${PROJECT_NAME} shared)
target_link_libraries(${PROJECT_NAME} bluetooth)
# deviceio
#target_link_libraries(${PROJECT_NAME} DeviceIo)
#target_link_libraries(${PROJECT_NAME} asound)
# bluez
#target_link_libraries(${PROJECT_NAME} bluetooth shared)
# libnl
#target_link_libraries(${PROJECT_NAME} libnl-3.a libnl-genl-3.a)
# wpa
#target_link_libraries(${PROJECT_NAME} libwpa_client_64.a)
# curl
# target_link_libraries(${PROJECT_NAME} curl)
# rtmp
#target_link_libraries(${PROJECT_NAME} rtmp)
# rkisp_api
#target_link_libraries(${PROJECT_NAME} rkisp_api)
# rockchip_mpp
#target_link_libraries(${PROJECT_NAME} rockchip_mpp)
# rkcamera
#target_link_libraries(${PROJECT_NAME} rkcamera)
# rksound
#target_link_libraries(${PROJECT_NAME} rksound)
# portaudio
#target_link_libraries(${PROJECT_NAME} libportaudio.a)
# alsa
#target_link_libraries(${PROJECT_NAME} asound)