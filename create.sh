make clean
make -j 9

cd bin/
./GoertzelLocalization 4.16 4 cap99.wav cap38.wav cap168.wav cap250.wav
cd ../