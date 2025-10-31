const alertTableBody = document.querySelector("#alertTable tbody");

    const ESP_IP = "http://192.168.1.28";

    function addAlert(level, message) {
      // Xóa dòng "Chưa có cảnh báo nào"
      const emptyRow = alertTableBody.querySelector(".empty");
      if (emptyRow) emptyRow.remove();

      const row = document.createElement("tr");
      const time = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });

      row.innerHTML = `
        <td>${time}</td>
        <td class="level-${level}">${level.toUpperCase()}</td>
        <td>${message}</td>
      `;

      alertTableBody.prepend(row);

      // Giới hạn tối đa 10 dòng
      if (alertTableBody.rows.length > 10) {
        alertTableBody.deleteRow(10);
      }
    }

    // Khởi tạo biểu đồ lớn
    const ctx = document.getElementById('sensorChart').getContext('2d');
    const chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          { label: 'Temperature (°C)', borderColor: '#ff7043', data: [], fill: false, tension: 0.4 },
          { label: 'pH', borderColor: '#42a5f5', data: [], fill: false, tension: 0.4 },
          { label: 'TDS (ppm)', borderColor: '#ffca28', data: [], fill: false, tension: 0.4 },
          { label: 'Turbidity (NTU)', borderColor: '#ab47bc', data: [], fill: false, tension: 0.4 }
        ]
      },
      options: {
        responsive: true,
        animation: { duration: 800 },
        plugins: { 
          legend: { 
            labels: { color: '#ccc', font: { size: 20, weight: '600' }, padding: 20 } 
          } 
        },
        scales: {
          x: { 
            title: { display: true, text: 'Time', color: '#ccc', font: { size: 18, weight: 'bold' } }, 
            ticks: { color: '#999', font: { size: 20 } },
            grid: { color: '#333' }
          },
          y: { 
            title: { display: true, text: 'Data', color: '#ccc', font: { size: 18, weight: 'bold' } }, 
            ticks: { color: '#999', font: { size: 20 } },
            grid: { color: '#333' }
          }
        }
      }
    });

    // Hàm tạo biểu đồ mini
    function makeMiniChart(id, color) {
      const ctx = document.getElementById(id).getContext('2d');
      return new Chart(ctx, {
        type: 'line',
        data: { labels: [], datasets: [{ borderColor: color, data: [], fill: false, tension: 0.4, borderWidth: 2, pointRadius: 0 }] },
        options: {
          animation: false,
          plugins: { legend: { display: false } },
          scales: { x: { display: false }, y: { display: false } }
        }
      });
    }

    const mini = {
      temp: makeMiniChart('tempMini', '#ff7043'),
      ph: makeMiniChart('phMini', '#42a5f5'),
      tds: makeMiniChart('tdsMini', '#ffca28'),
      turb: makeMiniChart('turbMini', '#ab47bc')
    };

    function showTab(id, btn) {
      document.querySelectorAll('.content').forEach(c => c.classList.remove('active-tab'));
      document.getElementById(id).classList.add('active-tab');
      document.querySelectorAll('nav button').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
    }

    function sendServo(angle) {
        // IP Gửi góc servo đến server
      fetch(`${ESP_IP}/servo?angle=${angle}`).then(r => r.text()).then(console.log);
    }
    function sendRelay(state) {
        // IP Gửi trạng thái relay đến server
      fetch(`${ESP_IP}/relay?state=${state}`)
        .then(r => r.text())
        .then(t => {
          console.log("Relay:", state);
          document.getElementById("relayStatus").innerText = 
            "Status: " + (state === "on" ? "ON" : "OFF");
        })
        .catch(err => console.error("Relay error:", err));
    } 

    async function updateSensors() {
      try {
        // IP Fetch dữ liệu cảm biến từ server
        const res = await fetch(`${ESP_IP}/sensor`);
        const data = await res.json();

        const time = new Date().toLocaleTimeString();

        // Cập nhật giá trị hiển thị
        document.getElementById('tempVal').innerText = data.temp.toFixed(2) + ' °C';
        document.getElementById('phVal').innerText = data.ph.toFixed(2);
        document.getElementById('tdsVal').innerText = data.tds.toFixed(0) + ' ppm';
        document.getElementById('turbVal').innerText = data.turb.toFixed(2) + ' NTU';

        // Cập nhật biểu đồ chính
        chart.data.labels.push(time);
        chart.data.datasets[0].data.push(data.temp);
        chart.data.datasets[1].data.push(data.ph);
        chart.data.datasets[2].data.push(data.tds);
        chart.data.datasets[3].data.push(data.turb);

        if (chart.data.labels.length > 20) {
          chart.data.labels.shift();
          chart.data.datasets.forEach(ds => ds.data.shift());
        }
        chart.update();

        // Cập nhật biểu đồ mini
        const updateMini = (miniChart, value) => {
          miniChart.data.labels.push('');
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

      } catch (err) {
        console.error("Lỗi khi lấy dữ liệu:", err);
      }
    }

    setInterval(updateSensors, 5000);

    // === Hàm cập nhật bảng cảnh báo ===
  async function updateWarnings() {
    try {
    // IP Fetch cảnh báo từ server
      const res = await fetch(`${ESP_IP}/warning`);
      const data = await res.json();

      const tableBody = document.querySelector("#alertTable tbody");
      tableBody.innerHTML = ""; // Xóa dữ liệu cũ

      if (data.length === 0) {
        tableBody.innerHTML = '<tr class="empty"><td colspan="3">Chưa có cảnh báo nào</td></tr>';
        return;
      }

      data.forEach(w => {
        const row = document.createElement("tr");
        const levelColor = w.level === 2 ? "#ff4d4d" : (w.level === 1 ? "#ffb74d" : "#aaa");
        row.innerHTML = `
          <td>${w.time}</td>
          <td style="color:${levelColor};font-weight:600">${w.type}</td>
          <td>${w.message}</td>
        `;
        tableBody.appendChild(row);
      });
    } catch (err) {
      console.error("Lỗi khi lấy cảnh báo:", err);
    }
  }
  
  setInterval(updateWarnings, 5000);

    function updateClock() {
      const now = new Date();
      const h = String(now.getHours()).padStart(2, '0');
      const m = String(now.getMinutes()).padStart(2, '0');
      const d = String(now.getDate()).padStart(2, '0');
      const mo = String(now.getMonth() + 1).padStart(2, '0');
      const y = now.getFullYear();
      
      document.getElementById('clockText').innerHTML = `${h}:${m}`;
      document.getElementById('dateText').textContent = `${d}/${mo}/${y}`;
    }

    setInterval(updateClock, 1000);
    updateClock();