#! /bin/bash

echo '-------------------------------'
echo 'Downloading and Installing Dependency..'
echo '-------------------------------'
DIR="$(pwd)"
git clone https://github.com/WiringPi/WiringPi ${DIR}/WiringPi
cd ${DIR}/WiringPi/wiringPi
make 
sudo make install
cd ..
rm -rf WiringPi
echo '-------------------------------'
echo 'Start Building RPI Audio Looper'
echo '-------------------------------'
cd ${DIR}/src
make
sudo make install