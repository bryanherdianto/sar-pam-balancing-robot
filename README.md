# SaR-PaM (Self-Balancing Robot with Path Memorization)

![Project Banner](https://i.imgur.com/aur2KEg.jpeg)  

<p align="center">
 <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" />
 <img src="https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white" />
 <img src="https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white" />
 <img src="https://img.shields.io/badge/React-20232A?style=for-the-badge&logo=react&logoColor=61DAFB" />
 <img src="https://img.shields.io/badge/TypeScript-007ACC?style=for-the-badge&logo=typescript&logoColor=white" />
 <img src="https://img.shields.io/badge/Vite-646CFF?style=for-the-badge&logo=vite&logoColor=white" />
</p>

## Introduction

SaR-PaM (Self-Balancing Robot with Path Memorization) is a two-wheeled robot based on the **ESP32 DevKit V1** designed to maintain real-time balance using the **MPU6050 sensor** and **PID algorithm**. This project implements a **Dual-Core FreeRTOS** architecture, where the balancing process is separated from the communication process to ensure maximum stability.

The standout feature of this robot is its command-based **Path Memorization**, which allows the robot to record a sequence of movement instructions from the user and automatically replay that path. Control is performed through a **ReactJS** web interface that communicates via low-latency **WebSocket**.

## Implementation

### Hardware Components

- **Microcontroller:** ESP32 DevKit V1 (Type-C)
- **IMU Sensor:** MPU6050 (6-Axis Accelerometer & Gyroscope)
- **Motor Driver:** L298N Dual H-Bridge
- **Actuators:** 2x DC Gearbox Motors (Yellow TT Motors)
- **Power Source:** 2x Li-Ion Batteries
- **Input:** Push Button (GPIO 0) for *Wake-up* from Sleep mode.

### Software Architecture

The software system is built using the **Arduino IDE** with **FreeRTOS** implementation, dividing the workload across two processor cores simultaneously:

1. **Motion Control Task (Core 1 - High Priority)**
    - Fully responsible for the PID loop.
    - Reads MPU6050 sensor data using *External Interrupt* (INT) for precise data synchronization.
    - Calculates PID output and drives the motors directly.
    - Detects falling conditions (*Failsafe*); if an extreme tilt angle is detected (>10 seconds), the system triggers *Light Sleep*.

2. **Network & Logic Task (Core 0 - Low Priority)**
    - Handles WiFi connection and the WebSocket server.
    - Receives JSON instructions from the client (ReactJS) and forwards them to Core 1 via **FreeRTOS Queue**.
    - Manages *Path Memorization* logic (Recording and Replay).

3. **Website Remote Controller (Frontend)**
    - **Command Transmission:** Sends JSON-formatted instruction packets to the ESP32 via WebSocket.
    - **Connection Management:** Monitors network connection status in real-time.
    - **Input Handling:** Translates user interactions (Button/Touch) into navigation control signals and *Record/Play* features.

4. **Path Memorization Module (Shared Resource)**
    - **Command Logging:** Records the sequence of instructions ("FORWARD", "LEFT", etc.) into a `std::vector` protected by a **Mutex**.
    - **Step-based Replay:** Uses a **Software Timer** (500ms interval) to re-execute stored commands without blocking the main loop.

## Testing and Evaluation

### Video Simulation

You can see the simulation of the robot balancing from this link here: <https://youtube.com/shorts/Nn4jgWTpkQI>

### Testing Performed

Pengujian dilakukan untuk memvalidasi integrasi hardware dan software:

1. **Balancing Stability Test**
    Testing the robot's ability to stand still and respond to external disturbances (light pushes).
2. **Communication Latency Test**
    Measuring the robot's responsiveness to commands from the Website Controller via WebSocket.
3. **Path Replay Accuracy**
    Verifying if the robot can correctly repeat the recorded movement sequence (Forward -> Turn -> Backward).
4. **Power Management (Safety) Test**
    Ensuring motors automatically shut off and the ESP32 enters *Light Sleep* mode when the robot falls, and can be woken up again using the BOOT button.

### Evaluation Summary

- **Stability:** The robot is able to maintain balance responsively thanks to *Core* separation (PID on Core 1 is not interrupted by WiFi activity on Core 0).
- **Remote Control:** WebSocket communication proved reliable for sending real-time navigation commands without *jitter* on the motors.
- **Memory Feature:** The *Command-Based Recording* mechanism successfully stores and replays movement logic, although distance accuracy depends on floor surface conditions (as it does not use encoders).
- **Power Efficiency:** The *Auto-Sleep* feature works effectively to prevent battery waste and motor overheating when the robot accidentally falls.

## Conclusion

The SaR-PaM project successfully built a stable and interactive self-balancing robot platform. The **FreeRTOS** implementation proved crucial in handling multitasking between time-sensitive balancing algorithms and heavy network communication.

The **Path Memorization** feature provides unique added value, allowing the robot to perform semi-automatic navigation based on command history. Overall, the system has met the *acceptance criteria*. Further development is suggested to use **Stepper Motors** to increase path replay precision and a **Custom PCB** for more even weight distribution.
