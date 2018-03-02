#!/bin/bash

unar libssh-0.7.5.tar.xz
cd libssh-0.7.5/
mkdir build
cd build/
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
make -j 9
sudo make install
cd ../../
exit