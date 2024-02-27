#! /bin/bash
echo '-------------------------------'
echo 'Downloading and Installing Dependency..'
echo '-------------------------------'
git clone https://github.com/WiringPi/WiringPi/tree/final_official_2.50
cd WiringPi
make 
sudo make install
cd ..
rm -rf WiringPi
echo '-------------------------------'
echo 'Start Building RPI Audio Looper'
echo '-------------------------------'
cd src
make
sudo make install