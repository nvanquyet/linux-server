# src/CMakeLists.txt
# Danh sách source files
set(SOURCES 
    main.c
    database/database_connector.c
    # Thêm các file source khác
)

# Tạo executable
add_executable(chat_app ${SOURCES})

# Liên kết thư viện
target_link_libraries(chat_app 
    ${OPENSSL_LIBRARIES}
    ${MYSQL_LIBRARIES}
    pthread

)