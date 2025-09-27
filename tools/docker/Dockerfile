FROM d3lta12/ubuntu-build-tools:latest

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt update
RUN apt install --no-install-recommends -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    debootstrap

# Setup sysroot
RUN debootstrap --arch=arm64 noble /opt/sysroot-arm64 http://ports.ubuntu.com/ubuntu-ports
RUN chroot /opt/sysroot-arm64 /bin/bash -c \
    "echo \"deb http://ports.ubuntu.com/ubuntu-ports noble main universe restricted multiverse\" > \
    /etc/apt/sources.list"
RUN chroot /opt/sysroot-arm64 /bin/bash -c \
    "apt update && apt upgrade -y && apt autoremove -y"
RUN chroot /opt/sysroot-arm64 /bin/bash -c \
    "apt install --no-install-recommends -y \
    libsdl2-dev \
    python3-catkin-pkg \
    python3-empy \
    python3-nose \
    python3-setuptools"
RUN chroot /opt/sysroot-arm64 /bin/bash -c "apt clean"
RUN chroot /opt/sysroot-arm64 /bin/bash -c "rm -rf /tmp/* /var/tmp/* /var/lib/apt/lists/*"

# Cleanup
WORKDIR /root
RUN apt clean
RUN rm -rf /tmp/* /var/tmp/* /var/lib/apt/lists/*