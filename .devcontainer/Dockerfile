FROM ubuntu:20.04
# Set timezone. Otherwise cmake asks for this on installation
ENV TZ=Europe/Berlin
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

WORKDIR /printer
# Install required packages
RUN apt-get update
RUN apt-get install -y sudo expect git-lfs

# Required Setup from https://github.com/abhiTronix/raspberry-pi-cross-compilers/wiki/Cross-Compiler-CMake-Usage-Guide-with-rsynced-Raspberry-Pi-32-bit-OS#cross-compiler-cmake-usage-guide-with-rsynced-raspberry-pi-32-bit-os
RUN apt-get install -y build-essential cmake unzip gfortran
RUN apt-get install -y gcc git bison python gperf pkg-config gdb-multiarch wget rsync
RUN apt-get install -y g++ gperf flex texinfo gawk bison openssl pigz libncurses-dev autoconf automake tar figlet

# Export toolchain paths
ENV PATH="/printer/.devcontainer/cross-pi-gcc-10.2.0-0/bin:${PATH}"
ENV LD_LIBRARY_PATH="/printer/.devcontainer/cross-pi-gcc-10.2.0-0/lib:${LD_LIBRARY_PATH}"

# Make the container keep running
CMD ["tail", "-f", "/dev/null"]

