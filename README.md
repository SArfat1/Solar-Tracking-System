# ☀️ Solar Tracking System using ESP32

An ESP32-based solar tracking system that compares light intensity from two LDR sensors and automatically adjusts a servo-mounted solar panel toward the stronger light source.

The project has been extended with a **local ESP32 web dashboard** so live sensor values and the servo angle can be viewed from a browser on the same Wi-Fi network.

## Features

- Dual-LDR light comparison
- Automatic servo-based tracking
- Moving-average sensor filtering
- Configurable tracking threshold
- Configurable servo step size
- Safe servo angle limits
- 16×2 I2C LCD monitoring
- Serial Monitor output
- ESP32 Wi-Fi connectivity
- Local browser dashboard
- JSON endpoint for live sensor data

## Hardware

Based on the supplied source code:

- ESP32 development board
- 2 × LDR sensors
- Servo motor
- 16×2 I2C LCD
- Solar panel / mechanical tracking setup
- Supporting circuit components and wiring

## Pin Configuration

| Component | ESP32 Pin |
|---|---:|
| Left LDR | GPIO 34 |
| Right LDR | GPIO 35 |
| Servo signal | GPIO 18 |
| I2C SDA | GPIO 21 |
| I2C SCL | GPIO 22 |

LCD address:

```text
0x3F
```

If the LCD module uses another address, update `LCD_ADDRESS` in the sketch.

## Tracking Logic

The ESP32 reads both LDR sensors and calculates:

```text
difference = left_LDR - right_LDR
```

If:

```text
difference > 250
```

the servo moves in one direction.

If:

```text
difference < -250
```

the servo moves in the opposite direction.

If the difference remains inside the threshold, the servo stays at its current position.

The servo is limited to:

```text
10° to 170°
```

This prevents the mechanism from continuously rotating beyond the configured range.

## Sensor Filtering

The improved version uses a moving average of 8 readings for each LDR.

This reduces sudden changes caused by sensor noise and produces more stable tracking behavior.

## LCD

The LCD displays:

- Left LDR reading
- Right LDR reading
- Current servo angle
- Light difference

## Wi-Fi Dashboard

The ESP32 can host a small local dashboard.

The dashboard displays:

- Left LDR
- Right LDR
- Light difference
- Servo angle
- Online/offline status
- Live sensor bars

### Setup

1. Open:

```text
src/secrets.example.h
```

2. Copy it to:

```text
src/secrets.h
```

3. Add your Wi-Fi credentials:

```cpp
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

4. Upload the sketch to the ESP32.
5. Open Serial Monitor at:

```text
115200
```

6. After connection, the ESP32 prints its local IP address.

Example:

```text
Wi-Fi connected. Dashboard: http://192.168.1.25
```

7. Open that address in a browser connected to the same Wi-Fi network.

### Security

`secrets.h` is excluded through `.gitignore`.

**Never commit your actual Wi-Fi password to GitHub.**

## Libraries

Install:

- `LiquidCrystal_I2C`
- `ESP32Servo`

These are used together with the ESP32/Arduino `Wire` and `WiFi` functionality.

## Project Demonstration

A demonstration video of the physical solar tracking prototype is included in:

```text
demo/solar_tracking_demo.mp4
```

The video shows the physical project setup and its operation. The repository also includes the source code used for the ESP32-based tracker.

## Project Structure

```text
Solar-Tracking-System/
│
├── README.md
├── .gitignore
│
├── src/
│   ├── solar_tracking_system.ino
│   └── secrets.example.h
│
├── docs/
│   └── project_summary.md
│
├── images/
│   └── ADD_PROJECT_IMAGES_HERE.txt
│
└── demo/
    └── ADD_PROJECT_VIDEO_HERE.txt
```

## Future Improvements

- Add solar-voltage/current sensors
- Calculate generated power
- Store tracking data
- Add historical charts
- Add remote cloud monitoring
- Add weather/environment sensors
- Extend the mechanism to two-axis tracking
- Add automatic calibration

## Accuracy Note

The original project source code supplied for this repository establishes the ESP32, two LDR inputs, servo, I2C LCD, pins, thresholds, and tracking logic.

The Wi-Fi dashboard is an **extension to the original project code**. It should only be presented as a tested feature after you upload the updated sketch to your actual ESP32 and verify it.

## License

This project is intended for educational and portfolio use.
