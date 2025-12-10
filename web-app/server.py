# server.py - FIXED for ESP32 compartment format
from flask import Flask, send_from_directory, jsonify, request
import json, os

APP_DIR = os.path.dirname(__file__)
DATA_FILE = os.path.join(APP_DIR, "schedules.json")

app = Flask(__name__, static_folder=APP_DIR)

# ensure schedules file exists
if not os.path.exists(DATA_FILE):
    with open(DATA_FILE, "w") as f:
        json.dump([], f)

@app.route("/")
def root():
    return send_from_directory(APP_DIR, "index.html")

@app.route("/<path:fn>")
def static_files(fn):
    return send_from_directory(APP_DIR, fn)

@app.route("/list", methods=["GET"])
def list_schedules():
    """
    Returns TWO formats:
    1. Array format for web UI: [{compartment, time, telegram}, ...]
    2. Object format for ESP32: {"A": {time, chat_id}, "B": {time, chat_id}}
    """
    with open(DATA_FILE, "r") as f:
        arr = json.load(f)
    
    # Check if request is from ESP32 or web browser
    user_agent = request.headers.get('User-Agent', '').lower()
    
    # If ESP32 is requesting, return object format
    if 'esp32' in user_agent or request.args.get('format') == 'esp32':
        result = {}
        for item in arr:
            compartment = item.get("compartment", "").upper()  # "A" or "B"
            if compartment in ["A", "B"]:
                result[compartment] = {
                    "time": item.get("time", ""),
                    "chat_id": item.get("telegram", "")
                }
        return jsonify(result)
    
    # Otherwise return array for web UI
    return jsonify(arr)

@app.route("/save", methods=["POST"])
def save_schedule():
    item = request.get_json(force=True)
    
    # Validate compartment
    compartment = item.get("compartment", "").upper()
    if compartment not in ["A", "B"]:
        return jsonify({"error": "Invalid compartment. Use A or B"}), 400
    
    with open(DATA_FILE, "r") as f:
        arr = json.load(f)
    
    # Remove any existing schedule for this compartment
    arr = [x for x in arr if x.get("compartment", "").upper() != compartment]
    
    # Add new schedule
    arr.append(item)
    
    with open(DATA_FILE, "w") as f:
        json.dump(arr, f, indent=2)
    
    return jsonify(arr)

@app.route("/delete", methods=["POST"])
def delete_schedule():
    req = request.get_json(force=True)
    idx = int(req.get("index", -1))
    with open(DATA_FILE, "r") as f:
        arr = json.load(f)
    if 0 <= idx < len(arr):
        arr.pop(idx)
    with open(DATA_FILE, "w") as f:
        json.dump(arr, f, indent=2)
    return jsonify(arr)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)