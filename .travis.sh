#!/usr/bin/env bash

git submodule update --init --recursive

# if [ -n "${LIBCXX_BUILD}" ]; then
#   # Checkout LLVM sources
#   git clone --depth=1 https://github.com/llvm-mirror/llvm.git llvm-source
#   git clone --depth=1 https://github.com/llvm-mirror/libcxx.git llvm-source/projects/libcxx
#   git clone --depth=1 https://github.com/llvm-mirror/libcxxabi.git llvm-source/projects/libcxxabi

#   # Build and install libc++ (Use unstable ABI for better sanitizer coverage)
#   mkdir llvm-build && cd llvm-build
#   cmake -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${COMPILER} \
#         -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr \
#         -DLIBCXX_ABI_UNSTABLE=ON \
#         -DLLVM_USE_SANITIZER=${LIBCXX_SANITIZER} \
#         -DLLVM_BUILD_32_BITS=OFF \
#         ../llvm-source
#   make cxx -j2
#   sudo make install-cxxabi install-cxx
#   cd ../
#   export EXTRA_CXX_FLAGS="-stdlib=libc++"
# fi

#if [ -n "${LIBCXX_BUILD}" -a "${TRAVIS_OS_NAME}" == "osx" ]; then
#  echo "Compiling boost"
#  wget https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2
#  tar xf boost_1_70_0.tar.bz2
#  cd boost_1_70_0
#  ./bootstrap.sh --with-toolset=clang
#  sudo ./b2 toolset=clang-3.8 cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" link=static install
#  cd ../
#fi

export EXTRA_C_FLAGS="${EXTRA_C_FLAGS} ${EXTRA_FLAGS}"
export EXTRA_CXX_FLAGS="${EXTRA_CXX_FLAGS} ${EXTRA_FLAGS}"

echo "Install secp256k1"
git clone --depth=1 https://github.com/bitcoin-core/secp256k1.git
cd secp256k1
./autogen.sh
./configure --with-bignum=no
make -j2
sudo make install
cd ../
