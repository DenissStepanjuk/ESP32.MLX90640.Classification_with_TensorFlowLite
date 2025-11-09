/** Функция берёт буфер температур и возвращает минимальное, среднее и максимальное значеия.
      const float* buffer - Буфер хранящий матрицу температур.
      size_t length - Размер буфера.
      float &minVal - Минимальное значеие температуры в буфере.
      float &avgVal - Среднее значеие температуры в буфере.
      float &maxVal - Максимальное значеие температуры в буфере.**/
void analyzeBuffer(const float* buffer, size_t length, float &minVal, float &avgVal, float &maxVal) {
  // Если буфер пустой, вернуть NaN.
  if (length == 0) {
    minVal = avgVal = maxVal = NAN;
    return;
  }

  // Минимальное значеие инициализируем первым элементом буфера.
  minVal = buffer[0];
  // Максимальное значеие инициализируем первым элементом буфера.
  maxVal = buffer[0];
  // Сумма всех температур.
  double sum = 0;

  // Проходим через все значение температур в буфере.
  for (size_t i = 0; i < length; i++) {
    // Текущее значение температуры в буфере.
    float val = buffer[i];
    // Обновить минимальное значеие.
    if (val < minVal) minVal = val;
    // Обновить максимальное значеие.
    if (val > maxVal) maxVal = val;
    // Считаем сумму всех температур.
    sum += val;
  }
  // Получаем среднее значеие.
  avgVal = sum / length;
}




/** Функция масштабирует все значения из буфера температур и возвращает буфер из отмасштабированных значеий.
      const float* buffer_TEMP - Буфер хранящий матрицу температур.
      float* buffer_SCALE - Буфер для матрицы отмасштабированных температур.
      size_t length - Размер буфера.**/
void TEMP2SCALE(const float* buffer_TEMP, float* buffer_SCALE, size_t length) {
  
  /** float minT - Минимальное значеие температуры в буфере.
      float avgT - Среднее значеие температуры в буфере.
      float maxT - Максимальное значеие температуры в буфере.**/
  float minT, avgT, maxT;

  // Функция берёт буфер температур и возвращает минимальное, среднее и максимальное значеия.
  analyzeBuffer(buffer_TEMP, length, minT, avgT, maxT);

  // Коэфицент для масштабирования температур меньше среднего значеия.
  float scaleMin = avgT - minT;
  // Коэфицент для масштабирования температур больше среднего значеия.
  float scaleMax = maxT - avgT;

  // Проходим через все значение температур в буфере и масштабируем.
  for (uint8_t h = 0; h < 24; h++) {
    for (uint8_t w = 0; w < 32; w++) {
      // Текущее значение температуры в буфере.
      float t = buffer_TEMP[h * 32 + w];

      // Масштабируем:
      // Если температура меньше среднего значеия.
      if(t < avgT){
        buffer_SCALE[h * 32 + w] = (t - avgT)/scaleMin;
      } else{ // Если температура больше среднего значеия.
        buffer_SCALE[h * 32 + w] = (t - avgT)/scaleMax;
      }
    }
  }
}




/** Функция берёт буфер из отмасштабированных значеий и возвращает RGB буфер.
      float* buffer_SCALE - Буфер для матрицы отмасштабированных температур.
      uint8_t* buffer_RGB - Буфер для RGB матрицы.
      size_t length - Размер буфера.**/
void SCALE2RGB(float* buffer_SCALE, uint8_t* buffer_RGB, size_t length) {

  // Проходим через все значения в буфере и преобразуем в RGB матрицу.
  for (uint8_t h = 0; h < 24; h++) {
    for (uint8_t w = 0; w < 32*3; w+=3) {
      // Текущее значение в буфере.
      float s = buffer_SCALE[h * 32 + w/3];

      // Формируем RGB пиксель.
      if(s >= 0){
        // синяя компонента
        buffer_RGB[h * 32 * 3 + w] = 0;
        // зелёная компонента
        buffer_RGB[h * 32 * 3 + w + 1] = 0;
        // красная компонента
        buffer_RGB[h * 32 * 3 + w + 2] = s * 255;
      } else{
        // синяя компонента
        buffer_RGB[h * 32 * 3 + w] = s * 255;
        // зелёная компонента
        buffer_RGB[h * 32 * 3 + w + 1] = 0;
        // красная компонента
        buffer_RGB[h * 32 * 3 + w + 2] = 0;
      }
    }
  }
}
  







/** Функция возвращает наименование категории захваченой датчика.
  int kCategoryCount - Кол-во классов предсказываемых моделью.
  int8_t probabilities [] - Массив для хранения вероятностей для всех классов.
  char* kCategoryLabels[] - Массив наименований категорий, которые модель может классифицировать.   **/
String getPrediction(int kCategoryCount, int8_t probabilities [], const char* kCategoryLabels[]){
  // Индекс для категории с наибольшей вероятностью.
  int idx = 0;
  // Максимальная вероятность.
  int8_t maxProbability = probabilities[idx];

  // Пройдём через все сущности "категории" для которых модель возвращает вероятность.
  for (int i = 1; i < kCategoryCount; i++) {
    // Если вероятность для текущей сущности "категории" больше максимальная вероятности назначеной ранее,
    if(probabilities[i] > maxProbability){
      // Обновим максимальную вероятность.
      maxProbability = probabilities[i];
      // Обновим индекс для категории с наибольшей вероятностью.
      idx = i;
    }
  }
  // Вернём наименование категории с наибольшей вероятностью.
  return String(kCategoryLabels[idx]);
}




/** Функция возвращает строку на базе которой будет построена таблица с распределением вероятностей для каждого предсказываемого класса (категории).
  int kCategoryCount - Кол-во классов предсказываемых моделью.
  int8_t probabilities [] - Массив для хранения вероятностей для всех классов.
  char* kCategoryLabels[] - Массив наименований категорий, которые модель может классифицировать.   **/
String getProbabilitiesTable(int kCategoryCount, int8_t probabilities [], const char* kCategoryLabels[]){
  // Строка содержащая наименование категорий.
  String row_1 = "";
  // Строка содержащая распределение вероятностей.
  String row_2 = "";
  // Заполним строки соответствующими значениями.
  for(int i = 0; i < kCategoryCount; i++){
    if(i != kCategoryCount-1){
      row_1 += String(kCategoryLabels[i]) +  ", ";
      row_2 += String(probabilities [i]) +  ", ";
    } else {
      row_1 += String(kCategoryLabels[i]) +  "\n";
      row_2 += String(probabilities [i]);
    }
  }

  // Создадим строку на базе которой будет построена таблица с распределением вероятностей для каждого предсказываемого класса (категории).
  String table = row_1 + row_2;
  // Вернём строку на базе которой будет построена таблица.
  return table;
}

