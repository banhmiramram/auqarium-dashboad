const express = require("express");
const mqtt = require("mqtt");
const http = require("http");
const { Server } = require("socket.io");

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
  cors: { origin: "*" },
});

// ====================== MQTT ======================
const MQTT_BROKER = "mqtt://test.mosquitto.org";
const mqttClient = mqtt.connect(MQTT_BROKER);

mqttClient.on("connect", () => {
  console.log("✅ Connected to MQTT broker:", MQTT_BROKER);

  // Subscribe nhận dữ liệu cảm biến từ ESP32
  mqttClient.subscribe("/aquarium/data", (err) => {
    if (!err) console.log("📡 Subscribed to /aquarium/data");
  });
});

// Khi nhận dữ liệu mới từ ESP32
mqttClient.on("message", (topic, message) => {
  try {
    if (topic === "/aquarium/data") {
      const data = JSON.parse(message.toString());
      io.emit("mqtt_data", data); // Gửi tới web
    }
  } catch (err) {
    console.error("Invalid MQTT message:", err.message);
  }
});

// ====================== SOCKET.IO ======================

// Khi web kết nối
io.on("connection", (socket) => {
  console.log("💻 Web client connected");

  // Khi web gửi lệnh điều khiển relay
  socket.on("control_relay", (state) => {
    console.log("⚙️ Relay control:", state);
    const payload = JSON.stringify({ state }); // {"state":"on"}
    mqttClient.publish("/aquarium/control/relay", payload);
  });

  // Khi web gửi lệnh điều khiển servo
  socket.on("control_servo", (angle) => {
    console.log("🌀 Servo control:", angle);
    const payload = JSON.stringify({ angle }); // {"angle":90}
    mqttClient.publish("/aquarium/control/servo", payload);
  });

  socket.on("disconnect", () => {
    console.log("❌ Web client disconnected");
  });
});

// ====================== EXPRESS STATIC ======================
app.use(express.static("public"));

// ====================== SERVER START ======================
const PORT = 3000;
server.listen(PORT, () => {
  console.log(`🌐 Web server running at http://localhost:${PORT}`);
});
