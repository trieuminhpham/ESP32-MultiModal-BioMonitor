# Multi-modal Vital Signs Monitoring System (ESP32)

![Project Banner](docs/images/banner_setup.jpg)
*(Chèn ảnh chụp hệ thống thực tế của bạn ở đây)*

## Overview
This project implements a real-time, multi-modal physiological monitoring system using the **ESP32** microcontroller. It captures and synchronizes four distinct vital signs on a unified time axis:
* **ECG (Electrocardiogram):** Using AD8232.
* **PPG (Photoplethysmogram):** Using MAX30102 (SpO2/Heart Rate).
* **PCG (Phonocardiogram):** Using Piezoelectric sensor + ADS1115 (16-bit ADC).
* **SPG (Speckle Plethysmography):** A novel implementation using the **ADNS3080** optical flow sensor.

The system utilizes **FreeRTOS** for precise task scheduling and multi-sensor synchronization.

## Key Features (Kotowari Points)
* **Novel SPG Measurement:** Hacks the ADNS3080 optical mouse sensor to dump **raw 30x30 pixel arrays** directly from registers via SPI. It calculates average brightness frame-by-frame to detect blood flow variations (Speckle Plethysmography).
* **High-Precision Synchronization:** Uses FreeRTOS to manage heterogeneous sampling rates (e.g., high-speed audio sampling vs. lower-speed I2C sensors) with minimal latency.
* **Edge Processing:** Performs signal preprocessing (Averaging, Filtering) directly on the ESP32 before transmission.

## Hardware Architecture
| Component | Function | Interface |
|-----------|----------|-----------|
| **ESP32** | Main Controller | - |
| **ADNS3080**| SPG (Blood Flow) | SPI (Custom Driver) |
| **AD8232** | ECG (Heart Electrical)| Analog |
| **MAX30102**| PPG (Oxygen/Pulse) | I2C |
| **Piezo+ADS1115**| PCG (Heart Sound)| I2C (16-bit ADC) |

![System Block Diagram](docs/block_diagram.png)

## Software Design
The firmware is built on **FreeRTOS** with the following task distribution:
1.  **Task_SPG:** High-priority SPI transaction to dump pixel data & compute mean intensity.
2.  **Task_ECG_PPG:** Reads AD8232 and MAX30102.
3.  **Task_Audio:** High-frequency sampling for heart sounds via ADS1115.
4.  **Task_Comms:** Packets data and sends via Serial/UDP to PC.

## Results
**(Chèn ảnh chụp màn hình các đồ thị sóng tín hiệu bạn vẽ trên máy tính)**
*Figure 1: Synchronized waveforms of ECG, PPG, and SPG.*

## Getting Started
### Prerequisites
* PlatformIO (recommended) or Arduino IDE.
* ESP32 DevKit V1.

### Installation
1.  Clone the repo:
    ```bash
    git clone [https://github.com/yourusername/ESP32-MultiModal-BioMonitor.git](https://github.com/yourusername/ESP32-MultiModal-BioMonitor.git)
    ```
2.  Open in PlatformIO.
3.  Build and Upload to ESP32.

## License
[MIT](https://choosealicense.com/licenses/mit/)
