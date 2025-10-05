# Use Ubuntu 22.04 as base
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libpq-dev \
    wget \
    unzip \
    pkg-config \
    curl

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh

# Install Drogon, bcrypt, nlohmann-json, jwt-cpp
RUN /opt/vcpkg/vcpkg install drogon nlohmann-json bcrypt jwt-cpp openssl

# Set environment for vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH=$VCPKG_ROOT:$PATH
ENV CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Copy project
WORKDIR /app
COPY . /app

# Build the app
RUN mkdir build && cd build \
    && cmake .. -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE \
    && cmake --build . --config Release

EXPOSE 8080

# Run the app
CMD ["./build/my_cppAuth"]
