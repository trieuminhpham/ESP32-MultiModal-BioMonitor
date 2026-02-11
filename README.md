# Multi-modal Vital Signs Monitoring System (ESP32)

An advanced physiological monitoring system powered by **ESP32**, capable of capturing and synchronizing **ECG, PPG, PCG, and SPG** signals in real-time. The system features a custom **FreeRTOS** architecture for multi-core processing and a Python-based PC tool for live visualization and "pre-trigger" recording.

## 📌 Project Overview
This project explores the correlation between different vital signs by synchronizing them on a unified time axis.
* **MCU:** ESP32 (Dual-core).
* **OS:** FreeRTOS.
* **Key Innovation:** Implementing **Speckle Plethysmography (SPG)** by hacking the ADNS3080 optical flow sensor to dump raw pixel data via SPI.

## 🌟 Key Features
1.  **Hardware-Level Sensor Hack:**
    * Directly accesses **ADNS3080 registers** via SPI to capture 30x30 raw pixel arrays.
    * Computes average brightness on-chip to detect blood flow variations (SPG).
    * Uses custom PWM (40kHz) via `LEDC` to drive the sensor's LED for optimal illumination.
2.  **Multi-Core Task Scheduling:**
    * **Core 0 (High Priority):** Dedicated to critical biosignals (ECG via AD8232 & PPG via MAX30102).
    * **Core 1 (Application):** Handles SPI transactions (ADNS3080), High-res ADC (ADS1115), and Serial/UDP communication to prevent blocking.
3.  **Smart Data Logging (Python Tool):**
    * **Circular Buffer:** The PC tool continuously buffers the last **5 seconds** of data in RAM.
    * **Pre-Trigger Capture:** When you hit "Save", it writes the previous 5 seconds *before* the button press to the CSV file, ensuring no critical events are missed.
    * **Live Waveforms:** Real-time plotting of ECG, SPG, and PPG signals.

## 🔌 Hardware Pinout
Based on the firmware configuration:
<img width="1874" height="1484" alt="image" src="https://github.com/user-attachments/assets/f8598d67-d56a-4a1a-bfcf-5e6114dc4ca5" />

| Sensor | Interface | ESP32 Pin | Note |
| :--- | :--- | :--- | :--- |
| **I2C Bus** | Common | **SDA: 21, SCL: 22** | Shared by MAX30102 & ADS1115 |
| **ADNS3080** | SPI | **MOSI: 23, MISO: 19, SCK: 18, CS: 17** | RST: 16 |
| **ADNS LED** | PWM | **GPIO 4** | 40kHz Drive |
| **AD8232** | Analog | **GPIO 34** (ADC1_CH6) | ECG Output |
| **ADS1115** | I2C | **Addr: GND** | Connects to Piezo sensor |

## 🏗️ Firmware Architecture (FreeRTOS)
The system utilizes both cores of the ESP32 to balance high-speed sampling and heavy SPI transactions.

| Task Name | Core | Priority | Function |
| :--- | :---: | :---: | :--- |
| `ecg_task` | 0 | 5 | Samples AD8232 ADC at 100Hz. |
| `max_task` | 0 | 5 | Polling MAX30102 FIFO for Red/IR data. |
| `adns_task` | 1 | 3 | Captures frame & calculates brightness (Heavy SPI). |
| `ads_task` | 1 | 3 | Samples Piezo via external 16-bit ADC. |
| `report_task`| 1 | 2 | Formats CSV string & prints to Serial (50Hz). |

## 💻 PC Visualization Tool
The project includes a Python script (`monitor.py`) for data acquisition.

### Prerequisites
```bash
pip install pyserial matplotlib
