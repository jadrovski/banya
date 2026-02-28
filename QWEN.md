# Sauna Controller - Project Context

## Project Overview

This is an **ESP32-based Sauna Controller** project built with PlatformIO and the Arduino framework. It provides comprehensive monitoring and control of sauna environmental conditions with visual feedback through an RGB LED strip and a 20x4 LCD display.

### Hardware Components

| Component | Description | Interface |
|-----------|-------------|-----------|
| ESP32 DOIT DevKit v1 | Main microcontroller | - |
| LCD 2004 I2C | 20x4 character display | I2C (0x27) |
| BME280 | Temperature, humidity, pressure sensor | I2C (0x76) |
| DS18B20 | Digital temperature sensors (up to 2) | OneWire (GPIO 5) |
| RGB LED Strip | Visual feedback via PWM | GPIO 25/26/27 |

### Pin Configuration

```
I2C SDA: GPIO 21
I2C SCL: GPIO 22
OneWire: GPIO 5
LED Red:   GPIO 25 (LEDC Channel 0)
LED Green: GPIO 26 (LEDC Channel 1)
LED Blue:  GPIO 27 (LEDC Channel 2)
```

## Architecture

### Source Structure

```
src/
├── main.cpp              # Application entry point, sensor reading, LCD display
├── LEDStrip.h/.cpp       # PWM-based RGB LED control with gamma correction
├── LEDController.h/.cpp  # LED effects controller (fade, blink, pulse, rainbow)
└── color/
    ├── Color.h           # Abstract base class for color models
    ├── RGB.h/.cpp        # RGB color model with conversions
    ├── HSV.h/.cpp        # HSV color model
    ├── HSL.h/.cpp        # HSL color model
    ├── TemperatureColor.h/.cpp  # Color from temperature (Kelvin/Celsius)
    └── SaunaColors.h     # Preset colors for sauna states
```

### Key Classes

- **`LEDStrip`**: Low-level PWM control for RGB LEDs with gamma correction (2.2), brightness control, and basic effects
- **`SaunaLEDStrip`**: Extends `LEDStrip` with sauna-specific modes (temperature, humidity, comfort, safety, relax)
- **`LEDStripController`**: Manages animated effects (fade, blink, pulse, rainbow) with timing
- **Color Models**: `RGB`, `HSV`, `HSL`, `TemperatureColor` - all convertible via the `Color` base class

### Operating Modes

The system supports 7 modes controlled via serial commands:

| Mode | Command | Description |
|------|---------|-------------|
| Auto | `A` | Automatic mode selection based on conditions |
| Temperature | `T` | Color reflects temperature |
| Humidity | `H` | Color reflects humidity |
| Comfort | `C` | Color based on heat index |
| Safety | `S` | Warning colors for dangerous conditions |
| Relax | `R` | Slow color cycling for relaxation |
| Manual | `M` | Direct brightness/color control |

## Building and Running

### Prerequisites

- PlatformIO Core (install via `pip install platformio`)
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

# Run tests (if implemented)
pio test
```

### Dependencies (from `platformio.ini`)

- `iakop/LiquidCrystal_I2C_ESP32` @ ^1.1.6 - LCD display driver
- `milesburton/DallasTemperature` @ ^3.11.0 - DS18B20 temperature sensors
- `paulstoffregen/OneWire` @ ^2.3.7 - OneWire protocol
- `adafruit/Adafruit BME280 Library` @ ^2.3.0 - Environmental sensor

## Development Conventions

### Code Style

- **File naming**: PascalCase for classes (e.g., `LEDController.h`, `LEDStrip.cpp`)
- **Namespaces**: `SaunaColors` namespace for color presets
- **Includes**: Use quotes for local headers, angle brackets for libraries
- **Guard macros**: `#ifndef SAUNA_<NAME>_H` format for header guards

### Color System

The project uses a flexible color model system:
- All colors inherit from abstract `Color` base class
- Conversion methods: `toRGB()`, `toHSV()`, `toHSL()`
- Factory methods for creating colors from different models
- Gamma correction applied via lookup table (2.2 gamma)

### Serial Interface

Interactive commands available via serial monitor:

```
Modes:      A=Auto, T=Temp, H=Humidity, C=Comfort, S=Safety, R=Relax, M=Manual
Brightness: 0-9 (0=off, 9=max)
Colors:     W=White, E=Red, F=Green, V=Blue
Effects:    P=Pulse, Q=Rainbow, B=Blink, X=Stop, O=Off
Info:       I=Status
```

### Testing

- Unit tests go in `test/` directory using PlatformIO Test Runner
- Test framework: Unity (default for PlatformIO)
- See: https://docs.platformio.org/en/latest/advanced/unit-testing/

## Safety Considerations

- Temperature threshold: >90°C triggers safety mode
- Humidity threshold: >70% triggers warning
- Critical: >95°C or >80% humidity shows danger indicator
- Sensor update interval: 2 seconds (configurable via `SENSOR_UPDATE_INTERVAL`)

## LCD Display Layout

```
Row 0: T1: [temp1]°C          (DS18B20 Sensor 1)
Row 1: T2: [temp2]°C          (DS18B20 Sensor 2)
Row 2: T3: [temp]°C, P: [pressure]mmHg  (BME280)
Row 3: HU: [humidity]%        (BME280)
```

## Future Enhancements

- Implement unit tests for color conversion algorithms
- Add WiFi connectivity for remote monitoring
- Implement OTA updates
- Add EEPROM storage for configuration persistence
- Implement PID control for temperature regulation
