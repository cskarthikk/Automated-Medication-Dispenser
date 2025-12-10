# Automated-Medication-Dispenser
Automated medication dispenser with biometric security, real-time scheduling, and IoT missed-dose alerts using ESP32, Flask, and Telegram.
Features

Biometric Security – Fingerprint verification required for dispensing.

Smart Scheduling – Web dashboard for adding/editing medicine timings for each compartment.

IR Confirmation – Dispenses pills only when a hand is detected.

Servo-Based Mechanism – Controlled pill release with adjustable compartments.

Missed Dose Alerts – Automatic Telegram notifications if the user does not respond.

Web App + API – Clean frontend UI and Flask backend used by ESP32.

Project Structure
hardware/
  ├── Dispenser.ino
  ├── schematic.jpeg
  └── circuit.jpeg
  └── working_demo.html

web-app/
  ├── index.html
  ├── styles.css
  ├── app.js
  ├── server.py
  └── schedules.json


🔧 Hardware Overview

ESP32

Fingerprint Sensor (R307/AS608)

RTC DS3231

IR Proximity Sensor

Servo Motor

LEDs + Buzzer

Dispenser.ino handles alarms, authentication, IR detection, servo control, and missed-dose logic.

🌐 Running the Web Scheduler

Install Flask:

pip install flask


Run the server:

python server.py


Open the web UI in your browser:

http://<your-ip>:5000


ESP32 fetches schedules using:

GET /list?format=esp32

📡 Telegram Alerts

A missed dose triggers an automatic Telegram message using the Telegram Bot API.

