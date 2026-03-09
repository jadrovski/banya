# Banya Controller - Project Context

## Project Overview

ESP32-based environmental monitoring and control system for banya (Russian sauna). Provides comprehensive sensor readings (temperature, humidity, pressure) with visual feedback through RGB LED strip and 20x4 LCD display. Features WiFi connectivity with web interface and touch sensor for navigation.

## Hardware Components

| Component | Description | Interface |
|-----------|-------------|-----------|
| ESP32 DOIT DevKit v1 | Main microcontroller | - |
| LCD 2004 I2C | 20x4 character display | I2C (0x27) |
| BME280 | Temperature, humidity, pressure sensor | I2C (0x76) |
| DS18B20 | Digital temperature sensors (up to 12) | OneWire (GPIO 5) |
| RGB LED Strip | Visual feedback via PWM | GPIO 25/26/27 |
| Touch Sensor | Page navigation | GPIO 15 (T3) |

## Pin Configuration

```
I2C SDA: GPIO 21
I2C SCL: GPIO 22
OneWire: GPIO 5
LED Red:   GPIO 25 (LEDC Channel 0)
LED Green: GPIO 26 (LEDC Channel 1)
LED Blue:  GPIO 27 (LEDC Channel 2)
Touch:     GPIO 15 (T3)
```

## Architecture

### Source Structure

```
src/
├── main.cpp              # Application entry point, sensor reading, LCD display
├── LEDStrip.h/.cpp       # PWM-based RGB LED control with gamma correction
├── LEDController.h/.cpp  # LED effects controller (fade, blink, pulse, rainbow)
├── hal/                  # Hardware Access Layer
│   ├── HAL.h             # Main HAL include file
│   ├── I2CDevice.h       # Base class for I2C devices
│   ├── LCD.h             # LCD 2004 I2C display
│   ├── BME280.h          # Environmental sensor
│   ├── DS18B20.h         # OneWire temperature sensors
│   ├── RGBLED.h          # RGB LED via PWM (LEDC)
│   ├── WiFi.h            # WiFi manager
│   ├── WebServer.h       # HTTP web server
│   ├── Touch.h           # Touch sensor
│   ├── PageManager.h     # Display page manager
│   ├── DisplayPage.h     # Base display page class
│   └── pages/            # Display page implementations
└── color/
    ├── Color.h           # Abstract base class for color models
    ├── RGB.h/.cpp        # RGB color model with conversions
    ├── HSV.h/.cpp        # HSV color model
    ├── HSL.h/.cpp        # HSL color model
    ├── TemperatureColor.h/.cpp  # Color from temperature (Kelvin/Celsius)
    └── BanyaColors.h     # Preset colors for banya states
```

### Key Classes

- **`LEDStrip`**: Low-level PWM control for RGB LEDs with gamma correction (2.2), brightness control, and basic effects
- **`BanyaLEDStrip`**: Extends `LEDStrip` with banya-specific modes (temperature, humidity, comfort, safety, relax)
- **`LEDStripController`**: Manages animated effects (fade, blink, pulse, rainbow) with timing
- **`HAL::LCD`**: LCD 2004 I2C display with custom character support
- **`HAL::BME280Sensor`**: BME280 environmental sensor wrapper
- **`HAL::DS18B20Manager`**: DS18B20 OneWire sensor manager (supports multiple sensors)
- **`HAL::RGBLED`**: RGB LED via ESP32 LEDC with gamma correction
- **`HAL::WiFiManager`**: WiFi connection manager with auto-reconnect
- **`HAL::BanyaWebServer`**: HTTP web server with JSON API
- **`HAL::TouchSensor`**: ESP32 touch sensor with tap/long-press detection
- **`HAL::PageManager`**: Multi-page display manager
- **Color Models**: `RGB`, `HSV`, `HSL`, `TemperatureColor` - all convertible via the `Color` base class

## Building and Running

### Prerequisites

- PlatformIO Core (`pip install platformio`)
- VS Code with PlatformIO IDE extension (recommended)

### Commands

```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor

# Build and upload
pio run --target upload

# Clean build artifacts
pio run --target clean
```

### WiFi Configuration

WiFi credentials are stored in NVS (Non-Volatile Storage) and configured via:

1. **AP Mode Setup**: Connect to the device's access point and configure via web interface
2. **Touch Sensor**: Long press (>5s) to enter WiFi setup mode

On first boot, the device automatically starts in AP mode for initial configuration.

### Dependencies (from `platformio.ini`)

- `iakop/LiquidCrystal_I2C_ESP32` @ ^1.1.6 - LCD display driver
- `milesburton/DallasTemperature` @ ^3.11.0 - DS18B20 temperature sensors
- `paulstoffregen/OneWire` @ ^2.3.7 - OneWire protocol
- `adafruit/Adafruit BME280 Library` @ ^2.3.0 - Environmental sensor

## Operating Modes

| Mode | Command | Description |
|------|---------|-------------|
| Auto | `A` | Automatic mode selection based on conditions |
| Temperature | `T` | Color reflects temperature |
| Humidity | `H` | Color reflects humidity |
| Comfort | `C` | Color based on heat index |
| Safety | `S` | Warning colors for dangerous conditions |
| Relax | `R` | Slow color cycling for relaxation |
| Manual | `M` | Direct brightness/color control |

## Serial Interface

Interactive commands via serial monitor (115200 baud):

```
Modes:      A=Auto, T=Temp, H=Humidity, C=Comfort, S=Safety, R=Relax, M=Manual
Brightness: 0-9 (0=off, 9=max)
Colors:     W=White, E=Red, F=Green, V=Blue
Effects:    P=Pulse, Q=Rainbow, B=Blink, X=Stop, O=Off
Pages:      > or . = Next, < or , = Prev
Info:       I=Status, D=HAL Diagnostics, L=WiFi Reconnect
```

## Touch Sensor Actions

| Action | Duration | Effect |
|--------|----------|--------|
| Tap | < 1s | Next page |
| Long Press | > 1s | WiFi reconnect |
| Very Long Press | > 5s | Device reboot |

## LCD Display Layout

```
Row 0: T1: [temp1]°C          (DS18B20 Sensor 1)
Row 1: T2: [temp2]°C          (DS18B20 Sensor 2)
Row 2: T3: [temp]°C, P: [pressure]mmHg  (BME280)
Row 3: HU: [humidity]%        (BME280)
```

## Safety Thresholds

| Condition | Threshold | Action |
|-----------|-----------|--------|
| High Temperature | > 90°C | Safety mode (yellow) |
| Critical Temperature | > 95°C | Danger indicator (red) |
| High Humidity | > 70% | Warning |
| Critical Humidity | > 80% | Danger indicator |

## Web Interface

Access via `http://<device_ip>` after WiFi connection.

Features:
- Real-time sensor status (temperatures, humidity, pressure)
- WiFi status indicator
- Operating mode display
- Auto-refresh every 2 seconds

## Development Conventions

### Code Style

- **File naming**: PascalCase for classes (e.g., `LEDController.h`, `LEDStrip.cpp`)
- **Namespaces**: `HAL` for hardware layer, `BanyaColors` for color presets
- **Includes**: Quotes for local headers, angle brackets for libraries
- **Guard macros**: `#ifndef BANYA_<NAME>_H` format

### Color System

Flexible color model hierarchy:
- All colors inherit from abstract `Color` base class
- Conversion methods: `toRGB()`, `toHSV()`, `toHSL()`
- Gamma correction via lookup table (2.2 gamma)

### HAL Design

- Configuration structs for all hardware components
- Virtual `begin()` method for initialization
- `getInfo()` method for diagnostics
- Non-blocking `handleLoop()` for async operations

## Testing

- Unit tests go in `test/` directory using PlatformIO Test Runner
- Test framework: Unity (default for PlatformIO)
- Run tests: `pio test`

## Diagnostics

Use serial command `D` for full HAL diagnostics:

```
> D
=== HAL Diagnostics ===
LCD 20x4 @ 0x27 (SDA:21, SCL:22)
BME280 @ 0x76 (SDA:21, SCL:22) T:25.3°C H:45% P:1013hPa
DS18B20 Manager (Pin:5) Sensors:2/2 [0:25.5°C] [1:26.1°C]
RGB LED (R:25,G:26,B:27) Ch:0,1,2 Freq:1000Hz Res:8-bit Bright:50%
WiFi: Connected, IP: 192.168.1.100
Touch: Value=45, Baseline=50, Threshold=40
```

## Sensor Update Intervals

| Sensor | Interval |
|--------|----------|
| DS18B20 | 2000ms (configurable) |
| BME280 | On-demand (non-blocking) |
| LCD Display | Per page render |

## Git Configuration

- `.pio/` build directory is gitignored
- WiFi credentials stored in device NVS (not in version control)
