#!/bin/bash

cd ../ && ./create.sh && cd Server/
mv speaker_ips ../DistanceCalculation/bin/data/
cd ../DistanceCalculation/bin/ && ./DistanceCalculation -p 2 -er 0 -f data/speaker_ips -tf testTone.wav -t GOERTZEL -ws TRUE && cd ../../Server/
cd ../Localization3D/ && ./Localization3D 3 2 < live_localization.txt && cd ../Server/
mv ../DistanceCalculation/bin/server_results.txt results/
mv ../Localization3D/server_placements_results.txt results/