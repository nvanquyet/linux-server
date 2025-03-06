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

# Improved MySQL detection
# Try using pkg-config first (more reliable on many systems)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(MYSQL QUIET mysqlclient)
endif()

# If pkg-config failed, try the manual approach
if(NOT MYSQL_FOUND)
    # Try to find MySQL using find_path and find_library
    find_path(MYSQL_INCLUDE_DIR
        NAMES mysql.h
        PATHS /usr/include/mysql /usr/local/include/mysql /usr/mysql/include/mysql
    )
    
    find_library(MYSQL_LIBRARIES
        NAMES mysqlclient
        PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64 /usr/mysql/lib
    )
    
    if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
        set(MYSQL_FOUND TRUE)
        message(STATUS "Found MySQL: ${MYSQL_LIBRARIES}, ${MYSQL_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "MySQL library not found. Please install MySQL development package.")
    endif()
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