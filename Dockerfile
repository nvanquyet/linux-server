# Build stage
FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libmysqlclient-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory and build the project
RUN mkdir -p build && \
    cd build && \
    rm -rf CMakeCache.txt CMakeFiles && \
    cmake .. && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    libmysqlclient21 \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy the built executable
COPY --from=builder /app/build/src/chat_app /app/chat_app

# Copy configuration file
COPY --from=builder /app/config.properties /app/config.properties

# Expose port
EXPOSE 1609

# Run the chat client
ENTRYPOINT ["/app/chat_app"]
