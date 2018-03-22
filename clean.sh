#!/bin/bash

# Clean deprecated programs
cd deprecated/
./clean.sh
cd ../

# Clean Server & Client
cd Server/
make clean
cd ../Client/
make clean
cd ../dependencies/
./clean.sh