# Use an official Ubuntu base image with a compiler
FROM ubuntu:20.04

# Install essential packages and add GCC 13 PPA
RUN apt-get update && apt-get install -y --fix-missing \
    software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y --fix-missing \
    gcc-13 \
    g++-13 \
    gcc-13-plugin-dev \
    build-essential \
    libboost-all-dev \
    graphviz \
    libgraphviz-dev \
    wget \
    nano \
    && rm -rf /var/lib/apt/lists/*


RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.1/cmake-3.27.1-Linux-x86_64.sh \
    && chmod +x cmake-3.27.1-Linux-x86_64.sh \
    && ./cmake-3.27.1-Linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-3.27.1-Linux-x86_64.sh


RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100


RUN gcc --version && g++ --version && cmake --version

ENV Graphviz_DIR=/usr/lib/x86_64-linux-gnu/graphviz

# Copy the project into the container
COPY . /usr/src/myapp

# Set the working directory to the project's root
WORKDIR /usr/src/myapp


RUN rm -rf build CMakeCache.txt && mkdir -p build && cd build \
    && cmake ..  \
    && cmake --build .


EXPOSE 9999


CMD ["/bin/bash"]
