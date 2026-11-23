# StretchSense BLE Data Acquisition

This repository contains a Nordic nRF52832 firmware example and a Python BLE client for the StretchSense sensor project. The firmware advertises a BLE peripheral named `StretchSense`, exposes a custom GATT notification characteristic, and sends two simulated 16-bit sensor channels every 100 ms.

## Hardware and Software

- MCU: Nordic nRF52832
- SDK: nRF Connect SDK v3.3.5
- Framework: Zephyr RTOS
- Client: Python with Bleak

## BLE Service

- Device name: `StretchSense`
- Sensor service UUID: `12345678-1234-5678-1234-56789abcdef0`
- Data characteristic UUID: `12345679-1234-5678-1234-56789abcdef0`
- Packet format: little-endian `uint16_t CH1 + uint16_t CH2`
- Packet size: 4 bytes
- Transmission interval: 100 ms

## Building the Firmware

Open the project with nRF Connect for VS Code and build it for the target board. The build entry point is [CMakeLists.txt](CMakeLists.txt), and BLE configuration is in [prj.conf](prj.conf).

From a configured Zephyr terminal, an equivalent build command is:

```powershell
west build -b nrf52dk/nrf52832
```

Flash the resulting firmware with nRF Connect for VS Code or nrfutil.

## Running the Python Client

Install the Bleak dependency and run the test script:

```powershell
python -m pip install bleak
python test_ble.py
```

The script scans for a BLE device named `StretchSense`, connects to it, enables notifications, and displays received sensor packets for 10 seconds.

## Packet Example

Each notification contains 4 bytes:

```text
Offset 0-1: CH1, little-endian uint16_t
Offset 2-3: CH2, little-endian uint16_t
```
