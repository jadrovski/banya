"""
PlatformIO pre-build script to load WiFi credentials from wifi.ini
This script reads wifi.ini and sets build flags for WIFI_SSID and WIFI_PASSWORD
"""

import os
import configparser
from pathlib import Path

# Get the project directory
project_dir = Path(os.environ.get("PROJECT_DIR", "."))
wifi_ini = project_dir / "wifi.ini"

# Check if wifi.ini exists
if not wifi_ini.exists():
    print(f"Warning: {wifi_ini} not found!")
    print("Please create wifi.ini with your WiFi credentials.")
    print("See wifi.ini.example for template.")
    exit(1)

# Parse wifi.ini
config = configparser.ConfigParser()
config.read(wifi_ini)

# Get WiFi credentials
try:
    ssid = config.get("env:esp32doit-devkit-v1", "WIFI_SSID").strip('"')
    password = config.get("env:esp32doit-devkit-v1", "WIFI_PASSWORD").strip('"')
except (configparser.NoSectionError, configparser.NoOptionError) as e:
    print(f"Error reading wifi.ini: {e}")
    print("Make sure wifi.ini contains [env:esp32doit-devkit-v1] section")
    print("with WIFI_SSID and WIFI_PASSWORD options.")
    exit(1)

# Set build flags
env_flags = [
    f'-D WIFI_SSID=\\"{ssid}\\"',
    f'-D WIFI_PASSWORD=\\"{password}\\"',
]

# Append flags to BUILD_FLAGS
env = DefaultEnvironment()
env.Append(BUILD_FLAGS=env_flags)

print(f"WiFi credentials loaded from wifi.ini")
print(f"  SSID: {ssid}")
print(f"  Password: {'*' * len(password)}")
