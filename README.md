CREATE TABLE users (
id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(255) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL, 
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE groups (~~~~
group_id INT AUTO_INCREMENT PRIMARY KEY,
group_name VARCHAR(50) NOT NULL,
created_at BIGINT NOT NULL,
created_by INT NOT NULL,
FOREIGN KEY (created_by) REFERENCES users(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


CREATE TABLE group_members (
group_member_id INT AUTO_INCREMENT PRIMARY KEY,
group_id INT NOT NULL,
user_id INT NOT NULL,
joined_at BIGINT NOT NULL,
role TINYINT DEFAULT 0,
FOREIGN KEY (group_id) REFERENCES groups(group_id) ON DELETE CASCADE,
FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
