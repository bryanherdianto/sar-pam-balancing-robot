# SaR-PaM (Self-Balancing Robot with Path Memorization)

**Real-Time System and Internet of Things Final Project**  
Department of Electrical Engineering, Universitas Indonesia  

---

## Introduction

SaR-PaM (Self-Balancing Robot with Path Memorization) adalah robot dua roda berbasis **ESP32** yang dirancang untuk menjaga keseimbangan secara real-time menggunakan **sensor IMU** dan **algoritma PID**. Selain itu, robot ini dibangun untuk mampu merekam jalur pergerakan serta mengulang kembali jalur tersebut (path memorization).

Sistem juga mendukung dua metode komunikasi:

- **WiFi (WebSocket)** — kontrol jarak jauh dan telemetry
- **Bluetooth** — kontrol jarak dekat dengan latensi rendah

Keseluruhan proses berjalan dengan arsitektur **FreeRTOS multitasking**, sehingga balancing, komunikasi, dan path memorization dapat bekerja paralel tanpa saling mengganggu.

---

## Implementation

### Hardware Components

- ESP32 microcontroller  
- IMU sensor (gyroscope + accelerometer)  
- Dual DC motor with motor driver  
- Power management module  
- Battery pack (Li-ion / Li-Po)  

### Software Architecture

Software dikembangkan di **Arduino IDE**, memanfaatkan **FreeRTOS** untuk menjalankan beberapa task paralel:

- **IMU Processing & PID Balancing**  
  Membaca sudut, menerapkan filter, dan menjalankan kontrol PID.

- **Path Memorization Module**  
  Mencatat pergerakan motor dan waktu tempuh untuk replay.

- **WebSocket Control Module (WiFi)**  
  Kendali remote + pengiriman telemetry.

- **Bluetooth Control Module**  
  Kendali lokal dengan latensi rendah.

- **Motor Control Module**  
  PWM untuk kedua motor, berjalan pada interval tetap menggunakan software timer.

- **Telemetry System**  
  Mengirim informasi seperti sudut IMU, output PID, status baterai, dan mode operasi.

---

## Testing and Evaluation

### Testing Performed

Pengujian dilakukan pada beberapa aspek utama:

1. **Balancing Test**  
   Menilai stabilitas robot saat maju, mundur, dan berbelok.

2. **WiFi Control Test**  
   Mengukur delay respons, akurasi perintah, dan jangkauan.

3. **Bluetooth Control Test**  
   Mengukur latensi, stabilitas koneksi, serta kompatibilitas dengan balancing system.

4. **Mode Switching Test**  
   Verifikasi switching otomatis/manual antara WiFi ↔ Bluetooth.

5. **Power Consumption Test**  
   Mencatat perubahan konsumsi daya ketika modul komunikasi dimatikan.

### Evaluation Summary

- Robot berhasil menjaga **keseimbangan stabil** menggunakan IMU + PID.  
- Kontrol **WiFi** berjalan responsif dan mendukung telemetry real-time.  
- Kontrol **Bluetooth** memberikan latensi paling rendah untuk kendali dekat.  
- Switching mode **WiFi ↔ Bluetooth** berfungsi tanpa mengganggu balancing.  
- Sistem FreeRTOS bekerja deterministik dan responsif.

---

## Conclusion

Proyek SaR-PaM berhasil mengintegrasikan balancing system, path memorization, dan dual communication mode dalam satu platform robot berbasis ESP32. Melalui implementasi multitasking FreeRTOS, robot mampu menjalankan balancing, komunikasi, dan recording jalur secara paralel dengan stabil.

Fitur-fitur utama seperti kontrol melalui WiFi dan Bluetooth, replay jalur, dan mode switching telah berhasil diuji dan memenuhi acceptance criteria. Proyek ini dapat dikembangkan lebih lanjut menuju robot semi-otonom dengan penambahan fitur seperti obstacle detection atau mapping.

---

## References

1. "Module 1 - Introduction to SMP with RTOS," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-1-introduction-to-smp-with-rtos> (Accessed: Dec. 7, 2025).
2. "Module 2 - Task Management," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-2-task-management> (Accessed: Dec. 7, 2025).
3. "Module 3 - Memory Management & Queue," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-3-memory-management-queue> (Accessed: Dec. 7, 2025).
4. "Module 4 - Deadlock & Synchronization," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-4-deadlock-synchronization> (Accessed: Dec. 7, 2025).
5. "Module 5 - Software Timer," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-5-software-timer> (Accessed: Dec. 7, 2025).
6. "Module 6 - Bluetooth & BLE," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-6-bluetooth-ble> (Accessed: Dec. 7, 2025).
7. "Module 7 - MQTT, HTTP, WIFI," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-7-mqtt-http-wifi> (Accessed: Dec. 7, 2025).
8. "Module 8 - Power Management," digilabdte. [Online]. Available: <https://learn.digilabdte.com/books/internet-of-things/chapter/module-8-power-management> (Accessed: Dec. 7, 2025).
9. Instructables, “DIY ESP32 Wifi Self Balancing Robot - B-Robot ESP32 Arduino Programing,” Instructables, Sep. 19, 2021. [Online]. Available: <https://www.instructables.com/DIY-ESP32-Wifi-Self-Balancing-Robot-B-Robot-ESP32-/> (Accessed: Dec. 7, 2025).
10. "Bluetooth® Low Energy (Bluetooth LE)," espressif. [Online]. Available: <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/bluetooth/bt_le.html> (Accessed: Dec. 7, 2025).
11. "Bluetooth® Architecture," espressif. [Online]. Available: <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/bt-architecture/index.html> (Accessed: Dec. 7, 2025).
