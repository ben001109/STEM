#!/bin/bash

echo "更新軟件包列表..."
sudo apt-get update

echo "安裝 Python3..."
sudo apt-get install -y python3

echo "安裝 pip3..."
sudo apt-get install -y python3-pip

echo "安裝 RPi.GPIO..."
sudo apt-get install -y python3-rpi.gpio

echo "安裝 spidev..."
pip3 install spidev

echo "安裝 SX127x..."
pip3 install SX127x

echo "所有依賴項安裝完成。"
