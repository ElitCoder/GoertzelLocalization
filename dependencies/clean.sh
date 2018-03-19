#/bin/bash

rm -rf libssh-0.7.5
rm -rf XADTemp*

cd nessh/
make clean
cd cli/
make clean
cd ../../