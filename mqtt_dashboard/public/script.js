// === MQTT realtime qua Socket.IO ===
const socket = io();

// === DOM elements ===
const alertTableBody = document.querySelector("#alertTable tbody");

// === Hàm thêm cảnh báo vào bảng ===
function addAlert(level, message) {
  if (!alertTableBody) return;

  const emptyRow = alertTableBody.querySelector(".empty");
  if (emptyRow) emptyRow.remove();

  const row = document.createElement("tr");
  const time = new Date().toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
  });

  row.innerHTML = `
    <td>${time}</td>
    <td class="level-${level}">${level.toUpperCase()}</td>
    <td>${message}</td>
  `;

  alertTableBody.prepend(row);
  if (alertTableBody.rows.length > 10) alertTableBody.deleteRow(10);
}

// === Khởi tạo biểu đồ lớn ===
const ctx = document.getElementById("sensorChart").getContext("2d");
const chart = new Chart(ctx, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "Temperature (°C)",
        borderColor: "#ff7043",
        data: [],
        fill: false,
        tension: 0.4,
      },
      {
        label: "pH",
        borderColor: "#42a5f5",
        data: [],
        fill: false,
        tension: 0.4,
      },
      {
        label: "TDS (ppm)",
        borderColor: "#ffca28",
        data: [],
        fill: false,
        tension: 0.4,
      },
      {
        label: "Turbidity (NTU)",
        borderColor: "#ab47bc",
        data: [],
        fill: false,
        tension: 0.4,
      },
    ],
  },
  options: {
    responsive: true,
    animation: { duration: 800 },
    plugins: {
      legend: {
        labels: {
          color: "#ccc",
          font: { size: 20, weight: "600" },
          padding: 20,
        },
      },
    },
    scales: {
      x: {
        title: {
          display: true,
          text: "Time",
          color: "#ccc",
          font: { size: 18, weight: "bold" },
        },
        ticks: { color: "#999", font: { size: 20 } },
        grid: { color: "#333" },
      },
      y: {
        title: {
          display: true,
          text: "Data",
          color: "#ccc",
          font: { size: 18, weight: "bold" },
        },
        ticks: { color: "#999", font: { size: 20 } },
        grid: { color: "#333" },
      },
    },
  },
});

// === Biểu đồ mini ===
function makeMiniChart(id, color) {
  const el = document.getElementById(id);
  if (!el) return null;

  const ctx = el.getContext("2d");
  return new Chart(ctx, {
    type: "line",
    data: {
      labels: [],
      datasets: [
        {
          borderColor: color,
          data: [],
          fill: false,
          tension: 0.4,
          borderWidth: 2,
          pointRadius: 0,
        },
      ],
    },
    options: {
      animation: false,
      plugins: { legend: { display: false } },
      scales: { x: { display: false }, y: { display: false } },
    },
  });
}

const mini = {
  temp: makeMiniChart("tempMini", "#ff7043"),
  ph: makeMiniChart("phMini", "#42a5f5"),
  tds: makeMiniChart("tdsMini", "#ffca28"),
  turb: makeMiniChart("turbMini", "#ab47bc"),
};

// === Tab chuyển đổi ===
function showTab(id, btn) {
  document
    .querySelectorAll(".content")
    .forEach((c) => c.classList.remove("active-tab"));
  document.getElementById(id).classList.add("active-tab");
  document
    .querySelectorAll("nav button")
    .forEach((b) => b.classList.remove("active"));
  btn.classList.add("active");
}

// === Gửi lệnh servo & relay QUA SOCKET.IO ===
function sendServo(angle) {
  socket.emit("control_servo", angle);
  console.log("📤 Gửi lệnh servo:", angle);
}

function sendRelay(state) {
  socket.emit("control_relay", state);
  console.log("📤 Gửi lệnh relay:", state);

  const relayStatus = document.getElementById("relayStatus");
  if (relayStatus) {
    relayStatus.innerText = "Status: " + (state === "on" ? "ON" : "OFF");
  }
}

// === Nhận dữ liệu MQTT realtime từ server Node ===
socket.on("mqtt_data", (data) => {
  try {
    const time = new Date().toLocaleTimeString();

    // Hiển thị dữ liệu lên web
    document.getElementById("tempVal").innerText =
      data.temp.toFixed(2) + " °C";
    document.getElementById("phVal").innerText = data.ph.toFixed(2);
    document.getElementById("tdsVal").innerText =
      data.tds.toFixed(0) + " ppm";
    document.getElementById("turbVal").innerText =
      data.turb.toFixed(2) + " NTU";

    // Cập nhật biểu đồ chính
    chart.data.labels.push(time);
    chart.data.datasets[0].data.push(data.temp);
    chart.data.datasets[1].data.push(data.ph);
    chart.data.datasets[2].data.push(data.tds);
    chart.data.datasets[3].data.push(data.turb);

    if (chart.data.labels.length > 20) {
      chart.data.labels.shift();
      chart.data.datasets.forEach((ds) => ds.data.shift());
    }
    chart.update();

    // Cập nhật mini chart
    const updateMini = (miniChart, value) => {
      if (!miniChart) return;
      miniChart.data.labels.push("");
      miniChart.data.datasets[0].data.push(value);
      if (miniChart.data.datasets[0].data.length > 20) {
        miniChart.data.datasets[0].data.shift();
        miniChart.data.labels.shift();
      }
      miniChart.update();
    };

    updateMini(mini.temp, data.temp);
    updateMini(mini.ph, data.ph);
    updateMini(mini.tds, data.tds);
    updateMini(mini.turb, data.turb);

    // ======== ALERT: pH ========
    if (data.ph < 6.0 || data.ph > 8.0) {
      addAlert("danger", `⚠️ pH NGUY HIỂM: ${data.ph.toFixed(2)}`);
    } else if (data.ph < 6.5 || data.ph > 7.5) {
      addAlert("warning", `pH bất thường: ${data.ph.toFixed(2)}`);
    }

    // ======== ALERT: Temperature ========
    if (data.temp < 22 || data.temp > 30) {
      addAlert("danger", `🔥 Nhiệt độ NGUY HIỂM: ${data.temp.toFixed(1)}°C`);
    } else if (data.temp < 24 || data.temp > 28) {
      addAlert("warning", `Nhiệt độ bất thường: ${data.temp.toFixed(1)}°C`);
    }

    // ======== ALERT: TDS ========
    if (data.tds < 50 || data.tds > 600) {
      addAlert("danger", `⚠️ TDS NGUY HIỂM: ${data.tds.toFixed(0)} ppm`);
    } else if (data.tds < 100 || data.tds > 400) {
      addAlert("warning", `TDS bất thường: ${data.tds.toFixed(0)} ppm`);
    }

    // ======== ALERT: Turbidity (NTU) ========
    if (data.turb > 15) {
      addAlert("danger", `💧 Độ đục NGUY HIỂM: ${data.turb.toFixed(1)} NTU`);
    } else if (data.turb > 5) {
      addAlert("warning", `Độ đục bất thường: ${data.turb.toFixed(1)} NTU`);
    }
  } catch (err) {
    console.error("Lỗi xử lý MQTT data:", err);
  }
});

// === Đồng hồ ===
function updateClock() {
  const now = new Date();
  const h = String(now.getHours()).padStart(2, "0");
  const m = String(now.getMinutes()).padStart(2, "0");
  const d = String(now.getDate()).padStart(2, "0");
  const mo = String(now.getMonth() + 1).padStart(2, "0");
  const y = now.getFullYear();

  const clockEl = document.getElementById("clockText");
  const dateEl = document.getElementById("dateText");

  if (clockEl) clockEl.innerHTML = `${h}:${m}`;
  if (dateEl) dateEl.textContent = `${d}/${mo}/${y}`;
}
setInterval(updateClock, 1000);
updateClock();

// === Biến & hàm mới cho servo hẹn giờ (tự động, không dùng nút) ===
let servoCloseTimeout = null; // timeout cho lần bật hiện tại
let servoScheduleTimer = null; // interval kiểm tra lịch
let servoScheduledTime = null; // Date giờ bật theo lịch


function openServoAuto(durationMs) {
  // Bật servo
  sendServo(180);

  // Nếu có timeout cũ → hủy
  if (servoCloseTimeout) clearTimeout(servoCloseTimeout);

  // Tự tắt sau durationMs
  servoCloseTimeout = setTimeout(() => {
    sendServo(0);
  }, durationMs);
}


// Đặt lịch bật servo theo đồng hồ web
function setServoSchedule() {
  const timeInput = document.getElementById("servoOnTime");
  const minInput  = document.getElementById("servoMin");
  const secInput  = document.getElementById("servoSec");
  const statusEl  = document.getElementById("servoScheduleStatus");

  if (!timeInput.value) {
    alert("Vui lòng chọn giờ bật.");
    return;
  }

  // Lấy thời gian bật servo
  const minutes = parseInt(minInput.value) || 0;
  const seconds = parseInt(secInput.value) || 0;
  const durationMs = (minutes * 60 + seconds) * 1000;
  if (durationMs <= 0) {
    alert("Thời gian bật servo không hợp lệ.");
    return;
  }

  // Lưu duration
  servoDurationMs = durationMs;

  // Tính giờ bật tiếp theo
  const [hh, mm] = timeInput.value.split(":").map(Number);
  const now = new Date();
  let target = new Date();

  target.setHours(hh, mm, 0, 0);
  if (target <= now) target.setDate(target.getDate() + 1);

  servoScheduledTime = target;

  // Tạo timer kiểm tra
  if (servoScheduleTimer) clearInterval(servoScheduleTimer);
  servoScheduleTimer = setInterval(checkServoSchedule, 1000);

  statusEl.textContent =
    `Đã đặt lịch ${timeInput.value}, bật trong ${minutes} phút ${seconds} giây.`;
}

let servoDurationMs = 30000; // fallback 30s

// Hàm check lịch, đến giờ thì bật servo + hỏi thời gian tắt
function checkServoSchedule() {
  if (!servoScheduledTime) return;

  const now = new Date();

  if (now >= servoScheduledTime) {
    // Bật servo và tự tắt sau thời gian nhập trong form
    openServoAuto(servoDurationMs);

    // Xóa lịch sau khi chạy
    clearInterval(servoScheduleTimer);
    servoScheduleTimer = null;
    servoScheduledTime = null;

    document.getElementById("servoScheduleStatus").textContent = "Chưa có lịch nào";
  }
}
