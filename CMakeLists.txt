cmake_minimum_required(VERSION 3.12)

project(riscv_sim)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h")

add_executable(riscv_sim ${SOURCES})

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
add_definitions(-D_CRT_SECURE_NO_WARNINGS )      # for visual studio

include_directories(
    ${CMAKE_SOURCE_DIR}/win/SDL2/include
    ${CMAKE_SOURCE_DIR}/src                      # 因为 CMake 是以 src 作为根路径所以新建的 pkg 比如 device 之类的文件 include 需要声明包名
)

target_link_directories(
    riscv_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/win/SDL2/lib/x64
)

message(${CMAKE_SOURCE_DIR}/win/SDL2/lib/x64)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
target_link_libraries(riscv_sim PRIVATE SDL2 SDL2main Ws2_32)
else()
add_compile_options(-g)

find_package(SDL2 REQUIRED)
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
target_link_libraries(riscv_sim PRIVATE pthread SDL2)
endif()


