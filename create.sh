make clean
make -j 9

cd bin/
./GoertzelLocalization 4 cap99.wav 200000 cap38.wav 400000 cap168.wav 600000 cap250.wav 800000
cd ../