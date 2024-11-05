cmake_minimum_required(VERSION 3.20)

project(Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TIMER_DIR ${SRC_DIR}/timer)
set(BUFFER_DIR ${SRC_DIR}/buffer)
set(LOG_DIR ${SRC_DIR}/log)

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${TIMER_DIR}/heaptimer.cpp
    ${BUFFER_DIR}/buffer.cpp
    ${LOG_DIR}/log.cpp
)

include_directories(${TIMER_DIR} ${BUFFER_DIR} ${LOG_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})