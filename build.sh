#!/bin/bash

#To Revover when chip type is unknown on esp32 device
#mos flash esp32 --esp-erase-chip

#mos build --clean --local --verbose --platform=esp32
mos build --local --verbose --platform=esp32
