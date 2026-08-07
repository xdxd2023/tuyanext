cmake_minimum_required(VERSION 3.0.2)

project(ai)

add_definitions(-DRV1106_1103)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} $ENV{COMMON_DIR})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE})
message(STATUS "link: ${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}")
#Opencv 4
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
#Thread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(spdlog REQUIRED)
find_package(lcm REQUIRED)


include_directories($ENV{COMMON_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(include src src/AiSorce src/include)

aux_source_directory(src/AiSorce  SRC_FILES)
aux_source_directory(src/src SRC_FILES)

add_compile_options(-g -Wall -Wextra 
                    -Wuninitialized 
                    -Wmaybe-uninitialized
                    -fno-omit-frame-pointer)
add_compile_options(
                    -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
                    -DUAPI2
                    -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
                    )
add_compile_options(-O0)

if(${TYPE} STREQUAL "exe")
add_executable(${PROJECT_NAME} ${SRC_FILES})
add_definitions(-DTYPE_EXE)
else()
add_library(${PROJECT_NAME} ${SRC_FILES})
add_definitions(-DTYPE_LIB)
endif()

# add_compile_options(-g -Wall
#                     -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
#                     -DROCKIVA -DUAPI2
#                     -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
#                     )

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${CMAKE_CURRENT_SOURCE_DIR}/lib:/usr/lib")

# 设置优先链接静态库

if(${LIBC_TYPE} STREQUAL "uclibc")
    target_link_libraries(${PROJECT_NAME}
                        ${OpenCV_LIBS}
                        rknnmrt
                        Threads::Threads
                        rockiva
                        sample_comm
                        rockit
                        rockchip_mpp
                        rkaiq
                        pthread
                        rtsp
                        rga
                        )
elseif(${LIBC_TYPE} STREQUAL "glibc")
    target_link_libraries(${PROJECT_NAME}
                        ${OpenCV_LIBS}
                        rknnmrt
                        Threads::Threads
                        sample_comm
                        rockit
                        rockchip_mpp
                        rkaiq
                        pthread
                        rtsp
                        stdc++
                        func_none
                        c
                        m
                        rt
                        dl
                        rga
                        )
endif()

if(${TYPE} STREQUAL "exe")
# lcm
target_link_libraries(${PROJECT_NAME} lcm)
# spdlog
target_link_libraries(${PROJECT_NAME} spdlog::spdlog_header_only)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE                        
                            ${OpenCV_INCLUDE_DIRS}
                            ${EXAMPLE_DIR}/include
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_SOURCE_DIR}/utils
                            ${CMAKE_CURRENT_SOURCE_DIR}/common 
                            ${CMAKE_CURRENT_SOURCE_DIR}/common/isp3.x   
                            ${CMAKE_CURRENT_SOURCE_DIR}/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rknn
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/librga 
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/uAPI2
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/common
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/xcore
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/algos
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/iq_parser
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/iq_parser_v2
                            ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/smartIr
                          )

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
SET(MODEL_DIR "src/model")
file(GLOB MODEL_FILES "${MODEL_DIR}/*")  
install(FILES ${MODEL_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/model)
file(GLOB LIBS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}/*.so")
install(FILES ${LIBS_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)