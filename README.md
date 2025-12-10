# Automated Medication Dispenser with IoT Alerts

A smart medication dispenser with biometric security, real-time scheduling, and IoT missed-dose alerts using ESP32, Flask, and Telegram.

---

## Features

### Biometric Security  
Fingerprint verification required for dispensing.

### Smart Scheduling  
Web dashboard for adding/editing medicine timings for each compartment.

### IR Confirmation  
Dispenses pills only when a hand is detected.

### Servo-Based Mechanism  
Controlled pill release using a servo-driven sliding mechanism.

### Missed Dose Alerts  
Automatic Telegram notifications if the user does not respond.

### Web App + API  
Clean frontend UI and Flask backend used by ESP32 to fetch schedules.

---

## Project Structure

### **hardware/**
hardware/
├── Dispenser.ino
├── schematic.jpeg
├── circuit.jpeg
└── working_demo.html

markdown
Copy code

### **web-app/**
web-app/
├── index.html
├── styles.css
├── app.js
├── server.py
└── schedules.json

yaml
Copy code

---

## Hardware Overview

- ESP32  
- Fingerprint Sensor (R307/AS608)  
- RTC DS3231  
- IR Proximity Sensor  
- Servo Motor  
- LEDs & Buzzer  

The firmware (`Dispenser.ino`) handles alarms, fingerprint authentication, IR hand detection, servo dispensing, and missed-dose logic.

---

## Running the Web Scheduler

### Install Flask
pip install flask

shell
Copy code

### Start the server
python server.py

graphql
Copy code

### Open the web UI
http://<your-ip>:5000

shell
Copy code

### ESP32 API endpoint
GET /list?format=esp32

yaml
Copy code

---

## Telegram Alerts

A missed dose automatically triggers a Telegram message via the Telegram Bot API.

---

## Demo

Demo photos/videos are included under:

hardware/working_demo.html