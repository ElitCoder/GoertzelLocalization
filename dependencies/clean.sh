#/bin/bash

# libssh
rm -rf libssh-0.7.5
rm -rf XADTemp*

# libcurlpp
rm -rf curlpp-0.8.1/

# libnessh
cd nessh/
make clean
cd cli/
make clean
cd ../../