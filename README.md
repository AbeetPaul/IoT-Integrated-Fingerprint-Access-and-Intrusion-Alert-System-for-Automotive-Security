# IoT-Integrated Fingerprint Access and Intrusion Alert System for Automotive Security
• Designed a vehicle security system using Arduino Uno, GSM, fingerprint sensor, GPS, and ESP-32 camera. • Enabled fingerprint-based access, real-time GPS tracking, and live video monitoring. • Configured SMS alerts to notify users instantly during unauthorized access attempts.

**UPDATE #1**

Hardware Connections & Setup:

The fingerprint sensor is connected to UART1 (RX=2, TX=13).
(UART = Universal Asynchronous Receiver Transmitter — it’s how devices communicate serially. RX is receive pin, TX is transmit pin.)

The SIM800L GSM module is connected to UART2 (RX=15, TX=14).
(Used to send SMS via a cellular network)

The GPS module (NEO-6M) is connected to UART0 (RX=12; TX not used).
(GPS sends its location data to the ESP32 over serial communication.)

The ESP32-CAM’s camera module (AI-Thinker variant) is initialized using GPIO pins connected to data and control lines.
(These pins handle the flow of image data from the camera into the ESP32.)

***EXTREMELY IMPORTANT POINTS TO NOTE***

The ESP32-CAM doesn’t have a built-in USB interface, so to upload code, you'll need an external USB-to-Serial adapter (FTDI or CH340).
i.e 3.3V to 3.3V (5V also works fine for some models, check your datasheet)
GND to GND
TX to UOR
RX to UOT

**Directions to upload code onto ESP-32 CAM**

1.Connect the USB-to-Serial adapter as shown.
2.Short IO0 to GND to enter the 'bootloader mode' to upload the code.
3.In the Arduino IDE, select:
  Board: "AI Thinker ESP32-CAM"
  Upload Speed: 115200
  Port: Your FTDI COM port
4.Hit Upload in the Arduino IDE.
  Once you see Connecting....._____, release IO0 from GND
5.Press the reset button to restart the module after the code is uploaded for the results to get reflected.

**NOTE**

Disconnect the GPIO 12 peripheral during boot because, 
1.GPIO12 controls the flash voltage selection.
2.If it is pulled HIGH at boot (due to external components), it causes the ESP32 to boot incorrectly or not at all.
3.You may get a boot loop or failed startup.

Instead

1.Leave GPIO12 unconnected during upload and boot.
2.If your project uses GPIO12 (e.g., for a sensor or camera), connect it only after the ESP32 has booted successfully.

This is one of the major flaws of my project as of now and I am actively addressing this issue by integrating shift registers to offload the GPS module control to alternative GPIOs, thereby ensuring stable boot and operation.


**UPDATE #2**

Included a virtual circuit diagram to assist other users in understanding and replicating the hardware connections accurately.

<img width="3000" height="2852" alt="circuit_image" src="https://github.com/user-attachments/assets/f5c7df93-ea2e-4a74-bdd0-772621401d20" />

To ensure reliable operation, I have supplied the GSM module and NEO-6M GPS module with their own dedicated 5V power sources, as their peak current demands exceed the maximum current available from the ESP32-CAM’s onboard voltage regulator. The ESP32-CAM board, particularly when streaming video or using WiFi, can experience peak current draws of up to 500–600mA. However, GSM modules typically require extremely high current during transmission bursts—up to 2A, and in some cases, up to 3A may be necessary to accommodate brief transmission pulses. The NEO-6M GPS module is less demanding, with a typical operating current of 45–47mA, but startup currents and acquisition modes may briefly reach higher values (worst-case figures from the datasheet are about 67mA)

**NOTE**

When powering modules from separate supplies, always ensure that the ground (GND) line of each module's power source is connected to the ESP32-CAM’s GND pin. This common ground reference is essential for signal integrity and proper communication between devices.
