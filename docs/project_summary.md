# Project Summary

## Project
Solar Tracking System using ESP32

## Objective
Automatically adjust the solar panel orientation based on the relative light intensity detected by two LDR sensors.

## Original Core
- ESP32
- Two LDR sensors
- Servo motor
- 16×2 I2C LCD
- Arduino/C++

## Improvements Added
- Moving-average filtering
- Configurable deadband/threshold
- Servo safety limits
- Improved servo initialization
- Non-blocking timing with `millis()`
- Serial monitoring
- Optional ESP32 Wi-Fi dashboard
- Live JSON sensor endpoint
- Local browser monitoring

## Important
The Wi-Fi dashboard is an extension and should be tested on the physical ESP32 before being described as a completed deployed feature.
