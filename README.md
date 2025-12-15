# SaR-PaM (Self-Balancing Robot with Path Memorization)

**Real-Time System and Internet of Things Final Project**  
Department of Electrical Engineering, Universitas Indonesia  

---

## Introduction

SaR-PaM (Self-Balancing Robot with Path Memorization) adalah robot dua roda berbasis **ESP32 DevKit V1** yang dirancang untuk menjaga keseimbangan secara real-time menggunakan **sensor MPU6050** dan **algoritma PID**. Proyek ini menerapkan arsitektur **Dual-Core FreeRTOS**, di mana proses penyeimbangan (balancing) dipisahkan dari proses komunikasi untuk menjamin stabilitas maksimal.

Fitur unggulan dari robot ini adalah **Path Memorization** berbasis perintah, di mana robot dapat merekam urutan instruksi gerak dari pengguna dan memutar ulang (replay) jalur tersebut secara otomasis. Pengendalian dilakukan melalui antarmuka website berbasis **ReactJS** yang berkomunikasi via **WebSocket** dengan latensi rendah.

---

## Implementation

### Hardware Components

- **Microcontroller:** ESP32 DevKit V1 (Type-C)
- **IMU Sensor:** MPU6050 (6-Axis Accelerometer & Gyroscope)
- **Motor Driver:** L298N Dual H-Bridge
- **Actuators:** 2x DC Gearbox Motors (Yellow TT Motors)
- **Power Source:** 2x Li-Ion Batteries
- **Input:** Push Button (GPIO 0) untuk *Wake-up* dari mode Sleep.

### Software Architecture

Sistem perangkat lunak dibangun menggunakan **Arduino IDE** dengan penerapan **FreeRTOS** yang membagi beban kerja ke dalam dua *core* prosesor secara simultan:

1.  **Motion Control Task (Core 1 - High Priority)**
    * Bertanggung jawab penuh atas loop PID.
    * Membaca data sensor MPU6050 menggunakan *External Interrupt* (INT) untuk sinkronisasi data presisi.
    * Menghitung output PID dan menggerakkan motor secara langsung.
    * Mendeteksi kondisi jatuh (*Failsafe*); jika sudut miring ekstrem (>10 detik), sistem memicu *Light Sleep*.

2.  **Network & Logic Task (Core 0 - Low Priority)**
    * Menangani koneksi WiFi dan server WebSocket.
    * Menerima instruksi JSON dari klien (ReactJS) dan meneruskannya ke Core 1 melalui **FreeRTOS Queue**.
    * Mengelola logika *Path Memorization* (Perekaman dan Replay).

3.  **Website Remote Controller (Frontend)**
    * **Command Transmission:** Mengirimkan paket instruksi berformat JSON ke ESP32 via WebSocket.
    * **Connection Management:** Memantau status koneksi jaringan secara *real-time*.
    * **Input Handling:** Menerjemahkan interaksi pengguna (Button/Touch) menjadi sinyal kendali navigasi dan fitur *Record/Play*.

4.  **Path Memorization Module (Shared Resource)**
    * **Command Logging:** Mencatat urutan instruksi ("FORWARD", "LEFT", dll) ke dalam `std::vector` yang dilindungi oleh **Mutex**.
    * **Step-based Replay:** Menggunakan **Software Timer** (500ms interval) untuk mengeksekusi ulang perintah yang tersimpan tanpa memblokir loop utama.

---

## Testing and Evaluation

### Testing Performed

Pengujian dilakukan untuk memvalidasi integrasi hardware dan software:

1.  **Balancing Stability Test**
    * Menguji kemampuan robot berdiri diam dan menanggapi gangguan eksternal (dorongan ringan).
2.  **Communication Latency Test**
    * Mengukur responsivitas robot terhadap perintah dari Website Controller via WebSocket.
3.  **Path Replay Accuracy**
    * Memverifikasi apakah robot dapat mengulang urutan gerakan yang direkam (Maju -> Putar -> Mundur) dengan urutan yang benar.
4.  **Power Management (Safety) Test**
    * Memastikan motor mati otomatis dan ESP32 masuk ke mode *Light Sleep* saat robot terjatuh, serta dapat dibangunkan kembali dengan tombol BOOT.

### Evaluation Summary

* **Stabilitas:** Robot mampu menjaga keseimbangan dengan responsif berkat pemisahan *Core* (PID di Core 1 tidak terganggu oleh aktivitas WiFi di Core 0).
* **Kendali Jarak Jauh:** Komunikasi WebSocket terbukti handal untuk mengirimkan perintah navigasi *real-time* tanpa *jitter* pada motor.
* **Fitur Memori:** Mekanisme *Command-Based Recording* berhasil menyimpan dan memutar ulang logika pergerakan, meskipun akurasi jarak bergantung pada kondisi permukaan lantai (karena tidak menggunakan *encoder*).
* **Efisiensi Daya:** Fitur *Auto-Sleep* berfungsi efektif mencegah pemborosan baterai dan panas berlebih pada motor saat robot tidak sengaja terjatuh.

---

## Conclusion

Proyek SaR-PaM berhasil membangun platform robot penyeimbang yang stabil dan interaktif. Implementasi **FreeRTOS** terbukti krusial dalam menangani *multitasking* antara algoritma penyeimbang yang sensitif terhadap waktu dan komunikasi jaringan yang berat.

Fitur **Path Memorization** memberikan nilai tambah unik, memungkinkan robot untuk melakukan navigasi semi-otomatis berdasarkan riwayat perintah. Secara keseluruhan, sistem ini telah memenuhi *acceptance criteria*. Pengembangan selanjutnya disarankan untuk menggunakan **Stepper Motor** guna meningkatkan presisi replay jalur dan **PCB Custom** untuk distribusi bobot yang lebih merata.

---

## References

1.  "Module 1 - Introduction to SMP with RTOS," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-1-introduction-to-smp-with-rtos](https://learn.digilabdte.com/books/internet-of-things/chapter/module-1-introduction-to-smp-with-rtos) (Accessed: Dec. 7, 2025).
2.  "Module 2 - Task Management," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-2-task-management](https://learn.digilabdte.com/books/internet-of-things/chapter/module-2-task-management) (Accessed: Dec. 7, 2025).
3.  "Module 3 - Memory Management & Queue," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-3-memory-management-queue](https://learn.digilabdte.com/books/internet-of-things/chapter/module-3-memory-management-queue) (Accessed: Dec. 7, 2025).
4.  "Module 4 - Deadlock & Synchronization," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-4-deadlock-synchronization](https://learn.digilabdte.com/books/internet-of-things/chapter/module-4-deadlock-synchronization) (Accessed: Dec. 7, 2025).
5.  "Module 5 - Software Timer," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-5-software-timer](https://learn.digilabdte.com/books/internet-of-things/chapter/module-5-software-timer) (Accessed: Dec. 7, 2025).
6.  "Module 7 - MQTT, HTTP, WIFI," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-7-mqtt-http-wifi](https://learn.digilabdte.com/books/internet-of-things/chapter/module-7-mqtt-http-wifi) (Accessed: Dec. 7, 2025).
7.  "Module 8 - Power Management," digilabdte. [Online]. Available: [https://learn.digilabdte.com/books/internet-of-things/chapter/module-8-power-management](https://learn.digilabdte.com/books/internet-of-things/chapter/module-8-power-management) (Accessed: Dec. 7, 2025).
8.  Instructables, “DIY ESP32 Wifi Self Balancing Robot - B-Robot ESP32 Arduino Programing,” Instructables, Sep. 19, 2021. [Online]. Available: [https://www.instructables.com/DIY-ESP32-Wifi-Self-Balancing-Robot-B-Robot-ESP32-/](https://www.instructables.com/DIY-ESP32-Wifi-Self-Balancing-Robot-B-Robot-ESP32-/) (Accessed: Dec. 7, 2025).
