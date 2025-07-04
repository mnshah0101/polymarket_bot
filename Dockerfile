# Multi-stage build for polymarket_bot
FROM ubuntu:22.04 as builder

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CMAKE_BUILD_TYPE=Release

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory and build
RUN mkdir -p build && cd build \
    && cmake .. \
    && make -j$(nproc) \
    && make install

# Runtime stage
FROM ubuntu:22.04 as runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libcurl4 \
    libsqlite3-0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 polymarket && \
    mkdir -p /app/bin /app/config /app/data && \
    chown -R polymarket:polymarket /app

# Copy built executable and config files
COPY --from=builder /usr/local/bin/polymarket_bot /app/bin/
COPY --from=builder /usr/local/etc/polymarket_bot/ /app/config/

# Set working directory
WORKDIR /app

# Switch to non-root user
USER polymarket

# Set environment variables
ENV PATH="/app/bin:${PATH}"
ENV CONFIG_PATH="/app/config"

# Create volume for persistent data
VOLUME ["/app/data"]

# Expose any necessary ports (if needed)
# EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD polymarket_bot --health-check || exit 1

# Default command
CMD ["polymarket_bot"]
