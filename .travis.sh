#!/usr/bin/env bash


export INSTALL_PREFIX=/usr
if [ "${TRAVIS_OS_NAME}" == "osx" ]; then
  export INSTALL_PREFIX=/usr/local
  export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
  
  wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2
  tar -xjf boost_1_69_0.tar.bz2
  pushd boost_1_69_0
  ./bootstrap.sh --prefix=${INSTALL_PREFIX}

  while sleep 20; do echo "[ $SECONDS seconds, boost still building... ]"; done &
  ./b2 toolset=${C_COMPILER} link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-chrono --with-test --with-serialization -q -j2 >building.log 2>&1
  kill %1
  tail building.log

  export BOOST_ROOT=`pwd`;

  # while sleep 20; do echo "[ $SECONDS seconds, boost still installing... ]"; done &
  # sudo ./b2 install >building.log 2>&1
  # kill %1
  # tail building.log

  popd
fi

echo "OpenSSL: " ${OPENSSL_ROOT_DIR}

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
      -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${COMPILER} \
      -DENABLE_SNAPPY=OFF ..
make -j2
sudo make install
popd

git submodule update --init --recursive

echo "Installing secp256k1"
export CC=${C_COMPILER}
git clone --depth=1 https://github.com/bitcoin-core/secp256k1.git
pushd secp256k1
./autogen.sh
./configure --with-bignum=no
make -j2
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
      -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${COMPILER} \
      -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} ..
make -j2
sudo make install
popd
