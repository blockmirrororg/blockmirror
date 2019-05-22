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

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz
tar -xzf mongo-c-driver-1.13.0.tar.gz
cd mongo-c-driver-1.13.0
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SNAPPY=OFF
make -j2
sudo make install
cd ../
cd ../

wget https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz
tar -xzf r3.4.0.tar.gz
cd mongo-cxx-driver-r3.4.0
sed -i '' 's/"maxAwaitTimeMS", count/"maxAwaitTimeMS", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp
cd build
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release ..
make -j2
sudo make install
cd ../
cd ../
