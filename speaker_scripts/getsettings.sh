#!/bin/bash

MIC_VOLUME=$(amixer -c1 sget 'Capture' | awk '/Left:/{print $4}')
LINE_VOLUME=$(amixer -c1 sget 'Headphone' | awk '/Left:/{print $4}')

rm speakersettings
echo "volume: " "$LINE_VOLUME" >> speakersettings
echo "capture: " "$MIC_VOLUME" >> speakersettings