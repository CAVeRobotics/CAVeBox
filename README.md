# CAVeBox

## Build for aarch64

1. Build protobufs for both x86_64 and aarch64:

   `cmake -S . -B _build/x86_64 -DCMAKE_INSTALL_PREFIX=_build/x86_64/protobuf-install -DCMAKE_CXX_STANDARD=20 -G Ninja -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -DABSL_PROPAGATE_CXX_STD=ON && cmake --build _build/x86_64 -t install`

   `cmake -S . -B _build/aarch64 -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake -DCMAKE_INSTALL_PREFIX=_build/aarch64/protobuf-install -DCMAKE_CXX_STANDARD=20 -G Ninja -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -DABSL_PROPAGATE_CXX_STD=ON && cmake --build _build/aarch64 -t install`

   `protoc` for x86_64 is needed to compile the protobufs and the static libraries for aarch64 are needed for the aarch64 build.

2. Build CAVeTalk

    `./tools/nanopb/generate.sh && cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake && cmake --build build`

    Make sure `set(PROTOBUF_INSTALL_DIR ${EXTERNAL_DIR}/protobuf/_build/${CMAKE_HOST_SYSTEM_PROCESSOR}/protobuf-install)` is present in `CMakeLists.txt`.