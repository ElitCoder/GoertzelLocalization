#!/bin/bash

cd dependencies/
unar libssh-0.7.5.tar.xz
cd libssh-0.7.5/
mkdir build
cd build/
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DWITH_STATIC_LIB=ON ..
make -j 9
sudo make install
cd ../../

cd nessh/
make -j 9
sudo make install