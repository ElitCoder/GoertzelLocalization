make clean
make -j 9

cd bin/
./GoertzelLocalization 4 172.25.12.99 172.25.9.38 172.25.12.168 172.25.13.250
cd ../