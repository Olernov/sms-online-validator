project(sms-online)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 -Wall")
include_directories(~/projects/utils/src/utils)
include_directories(~/projects/utils/src/utils/pspacket)
include_directories(/usr/lib/oracle/12.1/client64/include)

aux_source_directory(. SRC_LIST)
aux_source_directory(/home/oleg/projects/utils/src/utils/pspacket SRC_LIST)
link_directories(/usr/lib/oracle/11.2/client64/lib)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries (${PROJECT_NAME} libclntsh.so)
