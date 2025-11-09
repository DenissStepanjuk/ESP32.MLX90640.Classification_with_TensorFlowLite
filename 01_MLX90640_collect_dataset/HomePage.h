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
  </style>
</head>

<body>
  <h2>ESP32 + MLX90640 Thermal Camera</h2>

  <div><b>INFRARED IMAGE:</b></div>
  <img id="imgINFRARED" width="320" height="240">

  <p>
    <button type="button" id="BTN_dataset">DATASET</button>
    <button type="button" onclick="location.reload();">REFRESH PAGE</button>
  </p>

  <script>
    var Socket;
    var img_type = 0;

    document.getElementById('BTN_dataset').addEventListener('click', button_dataset);

    function init() {
      Socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      Socket.onmessage = function(event) {
        processCommand(event);
      };
    }

    function processCommand(event) {
      if (event.data instanceof Blob) {
        if (img_type == 0) {
          document.getElementById('imgINFRARED').src = URL.createObjectURL(event.data);
        } else if (img_type == 1) {
          const url = URL.createObjectURL(event.data);
          const a = document.createElement('a');
          a.href = url;
          a.download = 'INFRARED_MATRIX.txt';
          document.body.appendChild(a);
          a.click();
          document.body.removeChild(a);
          URL.revokeObjectURL(url);
        } else if (img_type == 2) {
          document.getElementById('imgBORDERS').src = URL.createObjectURL(event.data);
        }
      } else {
        try {
          var obj = JSON.parse(event.data);
          var type = obj.type;
          if (type === "change_img_type") {
            img_type = obj.value;
          }
        } catch (e) {
          console.error("Received data is neither Blob nor valid JSON:", event.data);
        }
      }
    }

    function button_dataset() {
      var btn_cpt = {type: 'dataset', value: true};
      Socket.send(JSON.stringify(btn_cpt));
    }

    window.onload = function(event) {
      init();
    };
  </script>
</body>
</html>
)rawliteral";

  return html;
}