# src/CMakeLists.txt
# Danh sách source files
file(GLOB_RECURSE SOURCES "*.c")

# Tạo executable
add_executable(chat_app ${SOURCES}
        ../include/group.h
        models/group.c
        ../include/json_utils.h
        network/json_utils.c
        ../include/group_member.h
        models/group_member.c
        ../include/db_message.h
        models/db_message.c)

# Liên kết thư viện
target_link_libraries(chat_app 
    ${OPENSSL_LIBRARIES}
    ${MYSQL_LIBRARIES}
    pthread
)