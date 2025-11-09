// Функция подготавливает и возвращает HTML страничку.

String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 + MLX90640 Thermal Camera</title>
  <style>
    body {
      background-color: #EEEEEE;
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 20px;
    }
    button {
      margin: 5px;
      padding: 10px 20px;
      border: none;
      background-color: #4CAF50;
      color: white;
      font-size: 16px;
      border-radius: 6px;
      cursor: pointer;
    }
    button:hover {
      background-color: #45a049;
    }
    img {
      border: 2px solid #555;
      border-radius: 10px;
      margin-top: 10px;
    }
    table {
      margin: 10px auto;
      border-collapse: collapse;
      background: white;
    }
    th, td {
      border: 1px solid #777;
      padding: 6px 10px;
    }
    th {
      background-color: #ddd;
    }
  </style>
</head>

<body>
  <h2>ESP32 + MLX90640 Thermal Camera</h2>

  <div><b>INFRARED IMAGE:</b></div>
  <img id="imgINFRARED" width="320" height="240">

  <div><b>Prediction: </b></div>
  <div id="new-prediction-container"><b>Wait...</b></div>

  <div><b>Probability table: </b></div>
  <div id='table-container-probability'></div>

  <p>
    <button type="button" onclick="location.reload();">REFRESH PAGE</button>
  </p>

  <script>
    function createTableFromString(dataString, containerId) {
      const lines = dataString.trim().split('\n').filter(line => line.trim().length > 0);
      const headers = lines[0].split(',').map(h => h.trim());
      const rows = lines.slice(1).map(line => line.split(',').map(cell => cell.trim()));

      const table = document.createElement('table');

      const thead = document.createElement('thead');
      const headerRow = document.createElement('tr');
      headers.forEach(header => {
        const th = document.createElement('th');
        th.textContent = header;
        headerRow.appendChild(th);
      });
      thead.appendChild(headerRow);
      table.appendChild(thead);

      const tbody = document.createElement('tbody');
      rows.forEach(rowData => {
        const tr = document.createElement('tr');
        rowData.forEach(cellData => {
          const td = document.createElement('td');
          td.textContent = cellData;
          tr.appendChild(td);
        });
        tbody.appendChild(tr);
      });
      table.appendChild(tbody);

      const container = document.getElementById(containerId);
      container.innerHTML = '';
      container.appendChild(table);
    }

    var Socket;

    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.onmessage = function(event) {
        processCommand(event);
      };
    }

    function processCommand(event) {
      if (event.data instanceof Blob) {
        document.getElementById('imgINFRARED').src = URL.createObjectURL(event.data);
      } else {
        try {
          var obj = JSON.parse(event.data);
          var type = obj.type;
          if (type.localeCompare("table_string_probability") == 0) {
            createTableFromString(obj.value, 'table-container-probability');
          } else if (type.localeCompare("new_prediction") == 0) {
            document.getElementById("new-prediction-container").innerText = obj.value;
          }
        } catch (e) {
          console.error("Received data is neither Blob nor valid JSON:", event.data);
        }
      }
    }



    window.onload = init;
    window.onbeforeunload = function() {
      if (Socket) Socket.close();
    };
  </script>
</body>
</html>
)rawliteral";

  return html;
}