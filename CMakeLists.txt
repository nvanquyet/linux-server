# Phiên bản CMake tối thiểu
cmake_minimum_required(VERSION 3.10)

# Tên project
project(MyChatProject C)

# Thiết lập tiêu chuẩn C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Các option build
option(BUILD_TESTS "Build test suite" OFF)
option(BUILD_DEBUG "Build with debug symbols" OFF)

# Thư viện bắt buộc
find_package(OpenSSL REQUIRED)
find_package(MySQL)
if(NOT MYSQL_FOUND)
    # Thử với cách khác
    find_library(MYSQL_LIBRARIES 
        NAMES mysqlclient
        PATHS /usr/lib /usr/local/lib
    )
    include_directories(/usr/include/mysql)
endif()

# Thêm thư mục include
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${MYSQL_INCLUDE_DIR}
)

# Thêm subdirectory
add_subdirectory(src)

# Nếu muốn build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Cấu hình debug
if(BUILD_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    add_definitions(-DDEBUG)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()