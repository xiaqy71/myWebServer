cmake_minimum_required(VERSION 3.20)

project(Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TIMER_DIR ${SRC_DIR}/timer)
set(BUFFER_DIR ${SRC_DIR}/buffer)
set(LOG_DIR ${SRC_DIR}/log)
set(CONFIG_DIR ${SRC_DIR}/config)
set(POOL_DIR ${SRC_DIR}/pool)
set(HTTP_DIR ${SRC_DIR}/http)

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${TIMER_DIR}/heaptimer.cpp
    ${BUFFER_DIR}/buffer.cpp
    ${LOG_DIR}/log.cpp
    ${CONFIG_DIR}/configMgr.cpp
    ${POOL_DIR}/sqlconnpool.cpp
    ${HTTP_DIR}/httprequest.cpp
    ${HTTP_DIR}/httpresponse.cpp
    ${HTTP_DIR}/httpconn.cpp
)

include_directories(${TIMER_DIR} ${BUFFER_DIR} ${LOG_DIR} ${CONFIG_DIR} ${POOL_DIR} ${HTTP_DIR} /usr/include/mysql)
link_directories(/usr/lib64/mysql)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} mysqlclient)

# 拷贝 config.ini 文件到构建目录，存在则覆盖
configure_file(${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_BINARY_DIR}/config.ini COPYONLY)

# 测试
cmake_policy(SET CMP0135 NEW)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)
# test gtest
add_executable(hello_test test/hello_test.cpp)
target_link_libraries(hello_test GTest::gtest_main)
gtest_discover_tests(hello_test)

# test log
add_executable(log_test test/log_test.cpp ${LOG_DIR}/log.cpp ${BUFFER_DIR}/buffer.cpp)
target_link_libraries(log_test GTest::gtest_main)
gtest_discover_tests(log_test)

# test config
add_executable(config_test test/config_test.cpp ${CONFIG_DIR}/configMgr.cpp)
target_link_libraries(config_test GTest::gtest_main)
gtest_discover_tests(config_test)