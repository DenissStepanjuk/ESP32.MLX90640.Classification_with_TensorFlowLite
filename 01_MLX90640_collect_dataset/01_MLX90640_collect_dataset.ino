/**
  Подготовил: Степанюк Денис Борисович
**/

// Библиотека для работы с wifi подключением (standard library).
#include <WiFi.h>  
// Библиотека для создания и запуска веб-сервера, обработки HTTP-запросов, формирования и отправки HTTP-ответов клиенту.
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
// Библиотека позволяет устанавливать постоянное соединение между сервером и клиентом, что делает возможным двустороннюю передачу данных в реальном времени.
#include <WebSocketsServer.h> 
// Библиотека используется для работы с JSON (JavaScript Object Notation) данными.
#include <ArduinoJson.h>
#include <Arduino.h>
// Библиотека для работы с шиной I2C.
#include <Wire.h>
// Библиотека предоставляет функции для преобразования изображений между различными форматами.
#include "img_converters.h"
// Документ хранящий код HTML странички.
#include "HomePage.h"
//В документе реализованы функции для работы с матрицой температур.
#include <functions.h>

// MLX_Library: https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example
// Высокоуровневые функции для работы с термокамерой (чтение кадров, расчёт температуры).
#include "MLX_Library/MLX90640_API.h"
#include "MLX_Library/MLX90640_API.cpp"
// Низкоуровневый драйвер для обмена по шине I²C.
#include "MLX_Library/MLX90640_I2C_Driver.h"
#include "MLX_Library/MLX90640_I2C_Driver.cpp"

// 7-битный адрес для MLX90640 по шине I2C.
const byte MLX90640_address = 0x33; // Default 7-bit unshifted address
// Сдвиг по умолчанию для MLX90640 на открытом воздухе
#define TA_SHIFT 8 // Default shift for MLX90640 in open air

// Буфер хранящий матрицу температур (32x24 = 768).
static float mlx90640To[768]; // Buffer for temperatures 
// Буфер для матрицы отмасштабированных температур.
float buffer_SCALE[768];
// Буфер для RGB матрицы.
uint8_t buffer_RGB[768 * 3];
// Cтруктура для хранения десятков коэффициентов (калибровочных данных), которые библиотека использует для вычисления температуры каждого пикселя.
paramsMLX90640 mlx90640;

// Функция проверки, подключен ли датчик по шине I2C.
bool isConnected() {
  Wire.beginTransmission(MLX90640_address);
  if (Wire.endTransmission() == 0) {
    return true;
  }
  return false;
}


// Данные WiFi сети.
const char* ssid = "WiFi_name";
const char* password = "WiFi_password";

// Экземпляр вебсервера ссылается на 80 порт.
AsyncWebServer server(80);
// Экземпляр WebSocket-сервера ссылается на 81 порт.
WebSocketsServer webSocket = WebSocketsServer(81);


// Переменная отображает состояние кнопки отвечающей за скачивание теплового снимка.
bool dataset = false;

/** В документе реализлвана функция webSocketEvent для обработки данных 
полученых через соеденение сокетов. **/
#include "socketConnection.h"


// Добавляем глобальную переменную для таймера
unsigned long previousMillis = 0;
const unsigned long frameInterval = 500; // Интервал между кадрами в миллисекундах



void setup() {
  // Последовательный порт (serial port).
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for serial

  // Инициализация интерфейса I²C (библиотека Wire) для связи с MLX90640.
  Wire.begin(8, 9); // Default ESP32 pins: SDA=8, SCL=9 (try these; or change to your pins, e.g., 8,9 or 20,21)
  // Устанавливает частоту I²C на 1 МГц (1000000 Гц).
  Wire.setClock(1000000);
  // Проверяет, обнаружен ли сенсор по адресу MLX90640 в I²C-шине.
  if (!isConnected()) {
    Serial.println("MLX90640 not detected at default I2C address. Check wiring/power/pull-ups. Freezing.");
    while (1) delay(10);
  }
  Serial.println("MLX90640 online!");

  // Переменная для хранения кода возврата функций библиотеки (Dump and extract parameters (done once)).
  int status;
  // Массив из 832 элементов, в котором будут сохранены EEPROM-данные сенсора (калибровочные параметры).
  uint16_t eeMLX90640[832];
  // Считывает содержимое EEPROM датчика (калибровочные параметры) и сохраняет их в массив eeMLX90640.
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  // Проверка успешности чтения EEPROM.
  if (status != 0) {
    Serial.println("Failed to load system parameters");
    while (1) delay(10);
  }

  // Распаковывает калибровочные данные из EEPROM (массив eeMLX90640) и сохраняет их в структуру mlx90640.
  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  // Проверка успешности распаковки EEPROM.
  if (status != 0) {
    Serial.print("Parameter extraction failed: ");
    Serial.println(status);
    while (1) delay(10);
  }

  // Устанавливает рабочий режим датчика (Enable temperature readout)
  MLX90640_I2CWrite(MLX90640_address, 0x800D, 6401); // Write to register 0x800D

  // Устанавливает частоту обновления кадров (скорость измерений) (Set refresh rate (0x04 = 4Hz; adjust as needed)).
  MLX90640_SetRefreshRate(MLX90640_address, 0x05); // Options: 0x02=1Hz, 0x03=2Hz, 0x05=8Hz (may need 800kHz clock)

  Serial.println("MLX90640 initialized!");


  // Подключение к WiFi.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Вывод IP адреса в последовательный порт.
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());


  /** Инициализация вебсервера (загружаем HTML страничку на WebServer, делаем её корневой "/"):
        + "/" - корневая папка, 
        + HTTP_GET - HTTP-метод GET для запроса данных с сервера
        + [](AsyncWebServerRequest *request) {} - лямбда-функция
        + AsyncWebServerRequest *request - указатель на объект, 
          который содержит всю информацию о запросе, поступившем на сервер.**/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // отправить (200 - http код ответа, метод отправки по html, HTML страничка)
    request -> send(200, "text\html", getHTML());
  });

  // Запуск вебсокета.
  webSocket.begin();
  // При приёме данных от клиента контролером через соеденение вебсокетов передать данные функцие webSocketEvent для дальнейшей обработки.
  // Функция webSocketEvent реализована в документе "receiveData.h".
  webSocket.onEvent(webSocketEvent);

  // Запуск вебсервера.
  server.begin();

}

void loop() {
  /** Метод webSocket.loop() обеспечивает:
    - Поддержание активного соединения с клиентами.
    - Обработку входящих данных от клиентов.
    - Обработку новых подключений и отключений клиентов.
    - Отправку данных клиентам, если это необходимо.**/
  webSocket.loop();

  // Проверяем, пора ли обновлять кадр
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= frameInterval) {
    previousMillis = currentMillis;

    // Проверка доступности датчика по I2C
    Wire.beginTransmission(MLX90640_address); // начинает передачу по I2C.
    byte error = Wire.endTransmission(); // завершает передачу и возвращает код ошибки (0 = OK).
    // Вывод ошибки и пропуск текущего кадра, если I2C не доступен.
    if (error != 0) {
      Serial.println("MLX90640 не отвечает по I2C!");
      return; // Пропускаем кадр
    }

    // MLX90640 делит изображение на 2 подкадра, каждый содержит половину пикселей (чередуются).
    // Цикл for выполняется 2 раза, чтобы получить оба подкадра, cчитывает один подкадр (subpage) с сенсора за раз (Read both subpages for a full frame).
    for (byte x = 0; x < 2; x++) {
      uint16_t mlx90640Frame[834];
      int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
      // Если при чтении подкадра произошла ошибка — сообщение и выход из цикла.
      if (status < 0) {
        Serial.print("GetFrame Error: ");
        Serial.println(status);
        return;
      }

      // Вычисляет напряжение питания (Vdd), измеренное внутри датчика, используя данные кадра и параметры калибровки.
      float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
      // Вычисляет температуру сенсора (ambient temperature, Ta) по данным из кадра и параметрам калибровки.
      float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
      // Отражённая температура.
      float tr = Ta - TA_SHIFT; // Reflected temperature
      // Коэффициент излучения объекта (0.95 = типичная для кожи/органики);
      float emissivity = 0.95;
      // Вычисляет температуру каждого из 768 пикселей (To — temperature output array).Библиотека применяет уравнения теплового излучения, используя калибровочные коэффициенты.
      MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
    }


    // Фильтр недействительных значений температур. Если температура выходит за диапазон [-40 °C, 300 °C], считается ошибочной.
    for (int i = 0; i < 768; i++) {
      if ((mlx90640To[i] <= -40) || (mlx90640To[i] >= 300)) {
        mlx90640To[i] = (i > 0) ? mlx90640To[i - 1] : mlx90640To[i + 1];
      }
    }

    // Функция возвращает буфер из отмасштабированных значеий температур.
    TEMP2SCALE(mlx90640To, buffer_SCALE, 768);
    // Функция возвращает RGB матрицу.
    SCALE2RGB(buffer_SCALE, buffer_RGB, 768);

    // Инициализируем переменную для хранение размера буфера содержащего JPEG изображение.
    size_t len_img;
    // Инициализировать буфер под хранение JPEG изображения.
    uint8_t *buf_img;
    // Ширина изображения.
    int input_width = 32;
    // Высота изображения.
    int input_height = 24;

    // Преобразуем RGB матрицу в JPEG.
    bool ok_JPEG = fmt2jpg(buffer_RGB, 768 * 3, input_width, input_height, PIXFORMAT_RGB888, 80, &buf_img, &len_img);
    if(ok_JPEG){
      // Обозначить на вебстраничке что следующим будет принято изображение.
      sendJson(jsonString, doc_tx, "change_img_type", 0);
      // Отправить изображение на вебстраничку.
      webSocket.broadcastBIN(buf_img, len_img);
    }

    if (dataset) {
      // Обозначить на вебстраничке что следующим будет принят файл.
      sendJson(jsonString, doc_tx, "change_img_type", 1);

      // Буфер для текстовых данных
      String txtData = "";
      txtData.reserve(8192); // резервируем память (ускоряет работу)

      for (int i = 0; i < 768; i++) {
        txtData += String(mlx90640To[i], 5); // 5 знаков после запятой
        if ((i + 1) % 32 == 0)
          txtData += "\n";   // перенос строки после каждых 32 значений
        else
          txtData += ", ";   // разделитель между элементами
      }

      // Преобразуем String в байты
      const char* dataPtr = txtData.c_str();
      size_t dataLen = txtData.length();

      // Отправляем как бинарный файл (текстовый по содержимому)
      webSocket.broadcastBIN((uint8_t*)dataPtr, dataLen);

      dataset = false;
    }

    // Освобождаем буфер.
    if (buf_img) free(buf_img);

    Serial.println("------------------------------");
  }
}
