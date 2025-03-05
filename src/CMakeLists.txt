# src/CMakeLists.txt
# Danh sách source files
file(GLOB_RECURSE SOURCES "*.c")

# Tạo executable
add_executable(chat_app ${SOURCES})

# Liên kết thư viện
target_link_libraries(chat_app 
    ${OPENSSL_LIBRARIES}
    ${MYSQL_LIBRARIES}
    pthread


)