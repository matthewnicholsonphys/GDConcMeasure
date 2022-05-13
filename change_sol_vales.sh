#!/usr/bin/env bash

if grep -Fxq "0" /sys/class/gpio/gpio18/value && grep -Fxq "0" /sys/class/gpio/gpio15/value; then
    echo 1 > /sys/class/gpio/gpio18/value
    echo 1 > /sys/class/gpio/gpio15/value
    echo "valves opened"
elif grep -Fxq "1" /sys/class/gpio/gpio18/value && grep -Fxq "1" /sys/class/gpio/gpio15/value; then
    echo 0 > /sys/class/gpio/gpio18/value
    echo 0 > /sys/class/gpio/gpio15/value
    echo "valve closed"
else
    echo "valve mismatch"
fi  
    
