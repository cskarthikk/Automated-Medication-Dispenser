🏥 Automated Medication Dispenser

A smart medication dispenser with biometric security, real-time scheduling, and IoT missed-dose alerts using ESP32, Flask, and Telegram.

🚀 Features
🔐 Biometric Security

Fingerprint verification required for dispensing.

🕒 Smart Scheduling

Web dashboard for adding/editing medicine timings for each compartment.

✋ IR Confirmation

Dispenses pills only when a hand is detected.

⚙️ Servo-Based Mechanism

Controlled pill release with adjustable compartments.

📡 Missed Dose Alerts

Automatic Telegram notifications if the user does not respond.

🌐 Web App + API

Clean frontend UI and Flask backend used by ESP32.

📂 Project Structure
hardware/
│── Dispenser.ino
│── schematic.jpeg
│── circuit.jpeg
│── working_demo.html

web-app/
│── index.html
│── styles.css
│── app.js
│── server.py
│── schedules.json

🔧 Hardware Overview

ESP32

Fingerprint Sensor (R307/AS608)

RTC DS3231

IR Proximity Sensor

Servo Motor

LEDs + Buzzer

🌐 Running the Web Scheduler

Install Flask:

pip install flask


Run the server:

python server.py


Open in browser:

http://<your-ip>:5000


ESP32 fetches schedules:

GET /list?format=esp32

📡 Telegram Alerts

A missed dose triggers an automatic notification to the caregiver via Telegram Bot API.

🎥 Demo

Add photos/videos inside hardware/working_demo.html or demo_media/.
