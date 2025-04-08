## 📘 Database Setup Instructions

### ✅ 1. Create and Use Database

```sql
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '12332145';
FLUSH PRIVILEGES;
CREATE DATABASE IF NOT EXISTS linux;
USE linux;
```

---

### ✅ 2. Create Tables

#### 🔹 users

```sql
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(100) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    online BOOLEAN DEFAULT FALSE,
    last_attendance_at DATETIME
);
```

---

#### 🔹 groups 

```sql
CREATE TABLE `groups` (
    group_id INT AUTO_INCREMENT PRIMARY KEY,
    group_name VARCHAR(100) NOT NULL,
    created_by INT NOT NULL,
    created_at DATETIME NOT NULL,
    password VARCHAR(100),
    FOREIGN KEY (created_by) REFERENCES users(id)
);
```

---

#### 🔹 group_members

```sql
CREATE TABLE group_members (
    id INT AUTO_INCREMENT PRIMARY KEY,
    group_id INT NOT NULL,
    user_id INT NOT NULL,
    joined_at DATETIME NOT NULL,
    role VARCHAR(50),
    FOREIGN KEY (group_id) REFERENCES `groups`(group_id),
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

---

#### 🔹 messages

```sql
CREATE TABLE messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sender_id INT NOT NULL,
    receiver_id INT DEFAULT NULL,
    group_id INT DEFAULT NULL,
    message_content TEXT NOT NULL,
    timestamp DATETIME NOT NULL,
    FOREIGN KEY (sender_id) REFERENCES users(id),
    FOREIGN KEY (receiver_id) REFERENCES users(id),
    FOREIGN KEY (group_id) REFERENCES `groups`(group_id)
);
```

---
