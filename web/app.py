from flask import Flask, render_template, request, jsonify
import requests
import time
from threading import Thread, Lock

app = Flask(__name__)

# ESP32 Configuration
ESP32_IP = "192.168.1.100"
ESP32_PORT = 80
ESP32_URL = f"http://{ESP32_IP}:{ESP32_PORT}"

# Telemetry data storage
telemetry_data = {
    "angle": 0,
    "output": 0,
    "setpoint": 190,
    "timestamp": 0,
    "connected": False,
}
telemetry_lock = Lock()

# Path memory storage
path_data = []
path_lock = Lock()


def update_esp32_url():
    """Update ESP32 URL from config"""
    global ESP32_URL
    ESP32_URL = f"http://{ESP32_IP}:{ESP32_PORT}"


@app.route("/")
def index():
    """Main landing page"""
    return render_template("index.html", esp32_ip=ESP32_IP)


@app.route("/control")
def control():
    """WiFi Control interface"""
    return render_template("control.html", esp32_ip=ESP32_IP)


@app.route("/path")
def path_memory():
    """Path Memory interface"""
    return render_template("path_memory.html", esp32_ip=ESP32_IP)


@app.route("/settings")
def settings():
    """Settings page"""
    return render_template("settings.html", esp32_ip=ESP32_IP)


@app.route("/api/config", methods=["GET", "POST"])
def api_config():
    """Get or set ESP32 IP configuration"""
    global ESP32_IP

    if request.method == "POST":
        data = request.json
        if "esp32_ip" in data:
            ESP32_IP = data["esp32_ip"]
            update_esp32_url()
            return jsonify({"status": "ok", "esp32_ip": ESP32_IP})
        return jsonify({"error": "Missing esp32_ip"}), 400

    return jsonify({"esp32_ip": ESP32_IP})


@app.route("/api/status")
def api_status():
    """Get ESP32 status"""
    try:
        response = requests.get(f"{ESP32_URL}/status", timeout=2)
        data = response.json()

        with telemetry_lock:
            telemetry_data["connected"] = True
            telemetry_data.update(data)

        return jsonify(data)
    except Exception as e:
        with telemetry_lock:
            telemetry_data["connected"] = False
        return jsonify({"error": str(e), "connected": False}), 503


@app.route("/api/command", methods=["POST"])
def api_command():
    """Send movement command to ESP32"""
    try:
        data = request.json
        command = data.get("command", "stop")
        speed = data.get("speed", 200)

        response = requests.post(
            f"{ESP32_URL}/command", json={"command": command, "speed": speed}, timeout=2
        )

        return jsonify(response.json())
    except Exception as e:
        return jsonify({"error": str(e)}), 503


@app.route("/api/telemetry")
def api_telemetry():
    """Get real-time telemetry from ESP32"""
    try:
        response = requests.get(f"{ESP32_URL}/telemetry", timeout=2)
        data = response.json()

        with telemetry_lock:
            telemetry_data.update(data)
            telemetry_data["connected"] = True
            telemetry_data["timestamp"] = time.time()

        return jsonify(data)
    except Exception as e:
        with telemetry_lock:
            telemetry_data["connected"] = False
        return jsonify({"error": str(e), "connected": False}), 503


@app.route("/api/pid", methods=["GET", "POST"])
def api_pid():
    """Get or set PID parameters"""
    try:
        if request.method == "POST":
            data = request.json
            response = requests.post(f"{ESP32_URL}/pid", json=data, timeout=2)
            return jsonify(response.json())
        else:
            response = requests.get(f"{ESP32_URL}/pid", timeout=2)
            return jsonify(response.json())
    except Exception as e:
        return jsonify({"error": str(e)}), 503


@app.route("/api/path", methods=["GET", "POST", "DELETE"])
def api_path():
    """Path memory operations"""
    global path_data

    if request.method == "GET":
        # Return stored path
        with path_lock:
            return jsonify(path_data)

    elif request.method == "POST":
        # Add path point or set entire path
        data = request.json

        with path_lock:
            if isinstance(data, list):
                # Replace entire path
                path_data = data
            else:
                # Add single point
                path_data.append(data)

        return jsonify({"status": "ok", "count": len(path_data)})

    elif request.method == "DELETE":
        # Clear path
        with path_lock:
            path_data = []
        return jsonify({"status": "ok"})


@app.route("/api/path/record", methods=["POST"])
def api_path_record():
    """Start/stop path recording on ESP32"""
    try:
        data = request.json
        action = data.get("action", "start")

        # This would communicate with ESP32 to start/stop recording
        if action == "start":
            with path_lock:
                global path_data
                path_data = []
            return jsonify({"status": "recording"})
        else:
            return jsonify({"status": "stopped", "count": len(path_data)})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/api/path/play", methods=["POST"])
def api_path_play():
    """Play back recorded path"""
    try:
        with path_lock:
            if not path_data:
                return jsonify({"error": "No path recorded"}), 400

        # Start playback thread
        def playback():
            with path_lock:
                points = list(path_data)

            for point in points:
                cmd = point.get("cmd", "stop")
                duration = point.get("duration", 100) / 1000.0  # Convert to seconds

                try:
                    requests.post(
                        f"{ESP32_URL}/command",
                        json={"command": cmd, "speed": 200},
                        timeout=2,
                    )
                except:
                    pass

                time.sleep(duration)

            # Stop at end
            try:
                requests.post(
                    f"{ESP32_URL}/command", json={"command": "stop"}, timeout=2
                )
            except:
                pass

        thread = Thread(target=playback)
        thread.daemon = True
        thread.start()

        return jsonify({"status": "playing", "count": len(path_data)})
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/api/path/stop", methods=["POST"])
def api_path_stop():
    """Stop path playback"""
    try:
        requests.post(f"{ESP32_URL}/command", json={"command": "stop"}, timeout=2)
        return jsonify({"status": "stopped"})
    except Exception as e:
        return jsonify({"error": str(e)}), 503


if __name__ == "__main__":
    print("=" * 50)
    print("ESP32 Self-Balancing Robot - Flask Server")
    print("=" * 50)
    print(f"ESP32 IP: {ESP32_IP}")
    print(f"Server running at: http://localhost:5000")
    print("=" * 50)
    print("\nEndpoints:")
    print("  /         - Home page")
    print("  /control  - WiFi control interface")
    print("  /path     - Path memory interface")
    print("  /settings - Configuration")
    print("=" * 50)

    app.run(host="0.0.0.0", port=5000, debug=True)
