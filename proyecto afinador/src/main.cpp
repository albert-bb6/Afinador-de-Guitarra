/********
PROYECTO PROCESADORES DIGITALES
AFINADOR DE GUITARRA
ALBERT BLAZQUEZ y POL RAMIREZ
*********/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <driver/i2s.h>
#include <math.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Pines I2S
#define I2S_WS   16
#define I2S_SD   18
#define I2S_SCK  17

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE     16000
#define I2S_BUFFER_SIZE 1024

// WiFi
const char* ssid = "Iphone de Pol";
const char* password = "12000047";

// WebServer
WebServer server(80);

// Tabla de notas
const char* noteNames[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

// Tabla de cuerdas
struct GuitarString {
  const char* name;
  float frequency;
};

GuitarString guitarStrings[] = {
  {"E2 (6ª)", 82.41},
  {"A2 (5ª)", 110.00},
  {"D3 (4ª)", 146.83},
  {"G3 (3ª)", 196.00},
  {"B3 (2ª)", 246.94},
  {"E4 (1ª)", 329.63}
};

// Variables globales
float currentFrequency = 0.0;
String currentNote = "??";
String currentString = "Ninguna";

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = I2S_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
}

float detectFrequency(int32_t* samples, int sampleCount, int sampleRate) {
  int maxLag = sampleRate / 80;
  int minLag = sampleRate / 400;

  float bestLag = -1;
  float bestValue = 0;

  for (int lag = minLag; lag <= maxLag; lag++) {
    double sum = 0;
    for (int i = 0; i < sampleCount - lag; i++) {
      float s1 = (float)(samples[i] >> 14);
      float s2 = (float)(samples[i + lag] >> 14);
      sum += s1 * s2;
    }
    if (sum > bestValue) {
      bestValue = sum;
      bestLag = lag;
    }
  }

  return (bestLag > 0) ? (float)sampleRate / bestLag : 0.0;
}

String frequencyToNote(float freq) {
  if (freq <= 0) return "??";
  int noteNumber = round(12.0 * log2(freq / 440.0) + 69);
  int octave = noteNumber / 12 - 1;
  int noteIndex = noteNumber % 12;
  return String(noteNames[noteIndex]) + String(octave);
}

String closestGuitarString(float freq) {
  if (freq <= 0) return "Ninguna";
  float minDiff = 1e6;
  const char* closest = "Ninguna";
  for (int i = 0; i < 6; i++) {
    float diff = fabs(freq - guitarStrings[i].frequency);
    if (diff < minDiff) {
      minDiff = diff;
      closest = guitarStrings[i].name;
    }
  }
  return String(closest);
}

void updateDisplay() {
  u8g2.clearBuffer();

  // Mostrar frecuencia en el centro
  u8g2.setFont(u8g2_font_logisoso32_tr);
  char freqBuf[16];
  snprintf(freqBuf, sizeof(freqBuf), "%.1fHz", currentFrequency);
  int freqWidth = u8g2.getStrWidth(freqBuf);
  u8g2.drawStr((128 - freqWidth) / 2, 40, freqBuf);

  // Mostrar cuerda debajo
  u8g2.setFont(u8g2_font_6x13_tr);
  int stringWidth = u8g2.getStrWidth(currentString.c_str());
  u8g2.drawStr((128 - stringWidth) / 2, 55, currentString.c_str());

  // Mostrar barra de afinación
  float targetFreq = 0.0;
  for (int i = 0; i < 6; i++) {
    if (currentString == guitarStrings[i].name) {
      targetFreq = guitarStrings[i].frequency;
      break;
    }
  }
  if (targetFreq > 0) {
    float diff = currentFrequency - targetFreq;
    int centerX = 64;
    int centerY = 60;
    int barLength = 30;
    int offset = (int)(diff * 4); // sensibilidad visual
    offset = constrain(offset, -barLength, barLength);
    u8g2.drawFrame(centerX - barLength, centerY, barLength * 2, 4);
    u8g2.drawBox(centerX + offset - 2, centerY + 1, 4, 2);
  }

  u8g2.sendBuffer();
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="es">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>Afinador de Guitarra</title>
    <style>
      body {
        background: linear-gradient(135deg, #667eea, #764ba2);
        color: #fff;
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        min-height: 100vh;
        margin: 0;
        padding: 1rem;
        text-align: center;
      }
      h1 {
        font-size: 2.5rem;
        margin-bottom: 0.2rem;
        text-shadow: 2px 2px 5px rgba(0,0,0,0.3);
      }
      p {
        font-size: 1.5rem;
        margin: 0.4rem 0;
        padding: 0.2rem 1rem;
        background: rgba(255, 255, 255, 0.15);
        border-radius: 12px;
        box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        width: 300px;
        max-width: 90vw;
        user-select: none;
      }
      .note {
        font-weight: 700;
        font-size: 2rem;
        color: #ffdd57;
        margin-top: 0.5rem;
        text-shadow: 1px 1px 2px #444;
      }
      footer {
        margin-top: 2rem;
        font-size: 0.9rem;
        opacity: 0.6;
      }
      @media (max-width: 400px) {
        h1 { font-size: 2rem; }
        p { font-size: 1.2rem; width: 90vw; }
      }
    </style>
  </head>
  <body>
    <h1>🎸 Afinador de Guitarra</h1>


    <!-- AQUI VA EL MENSAJE NUEVO -->
    <p style="font-size: 1rem; background: rgba(0,0,0,0.2); color: #ffdd57;">
      Pulsa el <b>botón físico</b> para <b>iniciar</b> o <b>detener</b> la afinación de la cuerda.
    </p>


    <p>Frecuencia: <span id="frequency">--.--</span> Hz</p>
    <p>Nota: <span class="note" id="note">??</span></p>
    <p>Cuerda más cercana: <span id="string">Ninguna</span></p>
    <footer>Actualización en tiempo real</footer>


    <script>
      async function fetchData() {
        try {
          const response = await fetch('/data');
          if (!response.ok) throw new Error('Error en respuesta');
          const data = await response.json();


          document.getElementById('frequency').textContent = data.frequency.toFixed(2);
          document.getElementById('note').textContent = data.note;
          document.getElementById('string').textContent = data.string;
        } catch (e) {
          console.error('Error al obtener datos:', e);
        }
      }


      setInterval(fetchData, 1000);
      fetchData();
    </script>
  </body>
  </html>
  )rawliteral";


  server.send(200, "text/html", html);
}


void handleData() {
  String json = "{";
  json += "\"frequency\":" + String(currentFrequency, 2) + ",";
  json += "\"note\":\"" + currentNote + "\",";
  json += "\"string\":\"" + currentString + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("🎸 Afinador de guitarra listo...");

  setupI2S();

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Wire.begin(1, 2); // SDA=1, SCL=2
  u8g2.begin();
  updateDisplay();
}

void loop() {
  const int samples = 1024;
  int32_t buffer[samples];
  size_t bytes_read;

  esp_err_t result = i2s_read(I2S_PORT, (char*)buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

  if (result == ESP_OK && bytes_read > 0) {
    float frequency = detectFrequency(buffer, samples, SAMPLE_RATE);
    String note = frequencyToNote(frequency);
    String stringName = closestGuitarString(frequency);

    currentFrequency = frequency;
    currentNote = note;
    currentString = stringName;

    Serial.printf("Frecuencia: %.2f Hz | Nota: %s | Cuerda: %s\n",
                  frequency, note.c_str(), stringName.c_str());

    updateDisplay();
  } else {
    Serial.println("Error al leer del I2S.");
  }

  server.handleClient();
  delay(200);
}