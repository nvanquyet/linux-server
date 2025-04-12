# Linux Server

A multi-threaded, secure C server implementation with session management, encryption, and message exchange capabilities.

## Architecture Overview

The server is built with a focus on secure communication, scalability, and reliability, implementing:

- Session-based client management
- Diffie-Hellman key exchange for secure communications
- AES-256 message encryption
- Multi-threaded message processing
- Thread-safe operations with proper locking mechanisms

## Core Components

### Session Management

Sessions handle client connections with the following key features:

#### Security Features

- **Diffie-Hellman Key Exchange**: Implemented in `trade_key()`, `send_dh_params()`, and `send_public_key()` functions
- **AES-256 Encryption**: All messages are encrypted/decrypted after initial key exchange
- **Secure Authentication**: Login validation with proper error handling

#### Multi-Threading Architecture

Each session operates with dedicated threads:

- **Sender Thread**: Handles outgoing message queue and transmission
- **Collector Thread**: Manages incoming message reception and processing
- **Thread Safety**: Implemented using mutex and rwlocks for critical sections

#### Message Exchange Protocol

- **Message Structure**: Command-based protocol with size prefixing
- **Encryption Flow**: IV (Initialization Vector) transmission, size information, and encrypted payload
- **Command Processing**: Unified message processing pipeline

## Key Classes and Interfaces

### Session

The `Session` class manages client connections with the following responsibilities:
- Connection establishment and maintenance
- Message encryption/decryption
- Authentication and user state management
- Message queuing and transmission

```c
Session* createSession(int socket, int id);
void destroySession(Session* session);
void session_send_message(Session* session, Message* message);
int session_login(Session *self, Message *msg, char *errorMessage, size_t errorSize);
```

### ServerManager

Thread-safe singleton that manages active sessions and users:
- User registration and tracking
- IP address management
- Connection limits enforcement
- Thread synchronization

### Controller

Handles application-level message processing:
- Command routing
- Business logic execution
- Response generation

## Message Flow

1. Client connects to server and gets assigned a session
2. Diffie-Hellman key exchange establishes secure channel
3. Client authenticates through login or registers a new account
4. Encrypted messages are exchanged through dedicated sender/collector threads
5. Messages are processed by controller handlers
6. Session is terminated when client disconnects or timeout occurs

## Threading Model

- **Per-Session Threads**: Each session maintains sender and collector threads
- **Thread Synchronization**: Mutex for message queues and rwlocks for shared resources
- **Thread Cleanup**: Proper shutdown sequence to avoid resource leaks

## Security Considerations

- Encrypted communication prevents eavesdropping
- Session validation prevents unauthorized access
- Resource limits prevent DoS attacks
- Proper error handling prevents information leakage

## Building and Running

### Prerequisites

- C compiler (GCC or Clang)
- Docker
- Docker Compose

### Database Setup with Docker

The system uses MySQL/MariaDB for data persistence. The database is automatically set up when running with Docker Compose.

The database schema includes:
- Users table: Stores user credentials and online status
- Groups table: Manages chat groups
- Group members table: Tracks users within groups
- Messages table: Stores all communication data

All tables are properly indexed for performance optimization.

### Running with Docker

1. Start the server with database using Docker Compose:
   ```bash
   docker compose up -d
   ```

2. The Docker setup will:
   - Start a MySQL/MariaDB container
   - Initialize the database using schema from `database/database.sql`
   - Configure the database with credentials defined in environment variables
   - Start the server container connected to the database

3. View logs:
   ```bash
   docker compose logs -f
   ```

4. Stop all services:
   ```bash
   docker compose down
   ```

### Building from Source (for development)

```bash
mkdir build && cd build
cmake ..
make
```

## Database Configuration

The server connects to the database using configuration from the environment or config file:
- Host: Typically `db` in Docker environment
- User: As defined in Docker Compose file
- Password: As defined in Docker Compose file
- Database name: `linux` (default)
- Port: 3306 (default MySQL port)

## Contributing

Contributions to the Linux Server project are welcome. Here's how you can contribute:

1. Fork the repository
2. Create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. Commit your changes:
   ```bash
   git commit -am 'Add some feature'
   ```
4. Push to the branch:
   ```bash
   git push origin feature/your-feature-name
   ```
5. Submit a pull request

Please ensure your code adheres to our coding standards and includes appropriate tests.

## Contact

For questions, issues, or suggestions:

- **Email**: nguyenducduy160903@gmail.com

