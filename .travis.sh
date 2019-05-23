#!/usr/bin/env bash

git submodule update --init --recursive

if [ "${TRAVIS_OS_NAME}" == "osx" ]; then
  echo "Installing boost"
  wget https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.bz2
  tar xf boost_1_70_0.tar.bz2
  pushd boost_1_70_0
  ./bootstrap.sh
  sudo ./b2 install
  popd
fi

echo "Installing secp256k1"
git clone --depth=1 https://github.com/bitcoin-core/secp256k1.git
pushd secp256k1
./autogen.sh
./configure --with-bignum=no
make -j2
sudo make install
popd

export INSTALL_PREFIX=/usr
if [ "${TRAVIS_OS_NAME}" == "osx" ]; then
  export INSTALL_PREFIX=/usr/local
  export OPENSSL_ROOT_DIR=/user/local/opt/openssl
fi

wget https://github.com/mongodb/mongo-c-driver/releases/download/1.13.0/mongo-c-driver-1.13.0.tar.gz
tar -xzf mongo-c-driver-1.13.0.tar.gz
mkdir mongo-c-driver-1.13.0/build
pushd mongo-c-driver-1.13.0/build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_BSON=ON \
      -DENABLE_SSL=OPENSSL \
      -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
      -DENABLE_STATIC=ON \
      -DENABLE_ICU=OFF \
      -DENABLE_TESTS=OFF \
      -DENABLE_EXAMPLES=OFF \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
      -DENABLE_SNAPPY=OFF ..
make -j16
sudo make install
popd

wget https://github.com/mongodb/mongo-cxx-driver/archive/r3.4.0.tar.gz
tar -xzf r3.4.0.tar.gz
mkdir mongo-cxx-driver-r3.4.0/build
pushd mongo-cxx-driver-r3.4.0/build
if [ "${TRAVIS_OS_NAME}" != "osx" ]; then
  sed -i 's/add_subdirectory(test)//' ../src/mongocxx/CMakeLists.txt ../src/bsoncxx/CMakeLists.txt
  sed -i 's/add_subdirectory(examples EXCLUDE_FROM_ALL)//' ../CMakeLists.txt
  sed -i 's/add_subdirectory(benchmark EXCLUDE_FROM_ALL)//' ../CMakeLists.txt
fi
cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
make -j2
sudo make install
popd
