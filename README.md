# Afinador-de-Guitarra
🎸 Afinador de Guitarra con ESP32 y OLED
Este proyecto implementa un afinador digital de guitarra en tiempo real utilizando un ESP32, un micrófono I2S (INMP441) y una pantalla OLED I²C 128x64. Además, cuenta con una interfaz web para visualizar los datos desde cualquier dispositivo conectado a la red local.

🚀 Características
🎵 Detección de frecuencia mediante correlación cruzada.
🎚️ Identificación automática de la cuerda más cercana (E, A, D, G, B, e).
📺 Visualización en tiempo real en pantalla OLED:
Frecuencia detectada (Hz)
Nota correspondiente (Ej: A4, E2, etc.)
Cuerda de guitarra asociada
Barra de afinación visual
🌐 Interfaz web sencilla y responsiva para consulta desde el móvil o navegador.
📡 Conexión WiFi configurable.
🧠 Tecnologías usadas
ESP32 (Placa DevKit / S3)
Micrófono digital I2S (ej. INMP441)
Pantalla OLED I2C (SSD1306 128x64)
Lenguaje: C++ (Arduino)
Librerías clave:
U8g2 para la pantalla OLED
driver/i2s.h para captura de audio
WiFi.h y WebServer.h para servidor web
⚙️ Esquema de conexiones
Componente	ESP32 Pin
INMP441 (I2S)	WS → GPIO 16
SD → GPIO 18
SCK → GPIO 17
OLED (I2C)	SDA → GPIO 1
SCL → GPIO 2
🔍 ¿Cómo funciona?
El micrófono digital capta el sonido de la guitarra y lo envía al ESP32 vía I2S.
El microcontrolador analiza las muestras y calcula la frecuencia dominante.
Se determina la nota musical correspondiente y la cuerda más cercana.
Se actualiza la pantalla OLED y la página web con esta información.
🧪 Uso
Clona este repositorio y carga el código en tu ESP32 desde PlatformIO o el IDE de Arduino.
Conecta el dispositivo a la guitarra y a una fuente de alimentación.
Abre el monitor serie para ver la IP asignada.
Accede a la interfaz web desde tu navegador introduciendo la IP.
📸 Interfaz
OLED:
[ 110.0Hz ] [ A2 (5ª) ] [◀────■────▶] ← barra de ajuste de afinación

Web:
Frecuencia
Nota
Cuerda
Mensaje informativo
Actualización automática cada segundo
📌 Notas
Puedes ajustar la sensibilidad o el rango de detección modificando la función detectFrequency().
Para otras afinaciones (Drop D, etc.), puedes modificar la tabla guitarStrings[].
🧑‍💻 Autores
Pol Ramierez
Albert Blazquez
