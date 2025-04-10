-- Tạo database nếu chưa có
CREATE DATABASE IF NOT EXISTS linux;
USE linux;

-- Tạo bảng users
CREATE TABLE users (
                       id INT AUTO_INCREMENT PRIMARY KEY,
                       username VARCHAR(100) NOT NULL UNIQUE,
                       password VARCHAR(255) NOT NULL,
                       online BOOLEAN DEFAULT FALSE,
                       last_attendance_at DATETIME
);

-- Tạo bảng groups
CREATE TABLE `groups` (
                          group_id INT AUTO_INCREMENT PRIMARY KEY,
                          group_name VARCHAR(100) NOT NULL,
                          created_by INT NOT NULL,
                          created_at DATETIME NOT NULL,
                          password VARCHAR(100),
                          FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE
);

-- Tạo bảng group_members
CREATE TABLE group_members (
                               id INT AUTO_INCREMENT PRIMARY KEY,
                               group_id INT NOT NULL,
                               user_id INT NOT NULL,
                               joined_at DATETIME NOT NULL,
                               role VARCHAR(50),
                               FOREIGN KEY (group_id) REFERENCES `groups`(group_id) ON DELETE CASCADE,
                               FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Tạo bảng messages
CREATE TABLE messages (
                          id INT AUTO_INCREMENT PRIMARY KEY,
                          sender_id INT NOT NULL,
                          receiver_id INT DEFAULT NULL,
                          group_id INT DEFAULT NULL,
                          message_content TEXT NOT NULL,
                          timestamp DATETIME NOT NULL,
                          FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
                          FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE SET NULL,
                          FOREIGN KEY (group_id) REFERENCES `groups`(group_id) ON DELETE CASCADE
);
