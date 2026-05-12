#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

#define TOUCH_PIN 4
#define LED_PIN   2

#define I2S_WS   25
#define I2S_SD   33
#define I2S_SCK  26
#define I2S_PORT I2S_NUM_0

#define SD_CS 5

#define SAMPLE_RATE 16000
#define RECORD_TIME 10
#define BUFFER_SAMPLES 512

File audioFile;
bool isRecording = false;
uint32_t bytesWritten = 0;
bool lastTouchState = LOW;

// ================== WAV HEADER ==================
void writeWavHeader(File file, uint32_t sampleRate, uint32_t dataSize) {
  uint32_t fileSize = dataSize + 36;

  file.write((const uint8_t*)"RIFF", 4);
  file.write((uint8_t*)&fileSize, 4);
  file.write((const uint8_t*)"WAVE", 4);

  file.write((const uint8_t*)"fmt ", 4);
  uint32_t subChunk1Size = 16;
  uint16_t audioFormat = 1;
  uint16_t numChannels = 1;
  uint16_t bitsPerSample = 16;
  uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
  uint16_t blockAlign = numChannels * bitsPerSample / 8;

  file.write((uint8_t*)&subChunk1Size, 4);
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&numChannels, 2);
  file.write((uint8_t*)&sampleRate, 4);
  file.write((uint8_t*)&byteRate, 4);
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&bitsPerSample, 2);

  file.write((const uint8_t*)"data", 4);
  file.write((uint8_t*)&dataSize, 4);
}

// ================== START ==================
void startRecording() {
  SD.remove("/record.wav");
  audioFile = SD.open("/record.wav", FILE_WRITE);

  if (!audioFile) {
    Serial.println("❌ File open failed");
    return;
  }

  uint32_t totalBytes = SAMPLE_RATE * RECORD_TIME * 2;
  writeWavHeader(audioFile, SAMPLE_RATE, totalBytes);

  bytesWritten = 0;
  isRecording = true;

  digitalWrite(LED_PIN, HIGH);
  Serial.println("🎙 Recording started...");
}

// ================== STOP ==================
void stopRecording() {
  audioFile.close();
  isRecording = false;

  digitalWrite(LED_PIN, LOW);
  Serial.println("✅ Recording finished");
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD Card failed");
    while (1);
  }

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SAMPLES,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_set_clk(I2S_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);

  Serial.println("👉 Touch sensor to start recording...");
}

// ================== LOOP ==================
void loop() {
  int touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH && lastTouchState == LOW && !isRecording) {
    startRecording();

    while (digitalRead(TOUCH_PIN) == HIGH);
    delay(200);
  }

  lastTouchState = touchState;

  if (isRecording) {
    int32_t i2sBuffer[BUFFER_SAMPLES];
    int16_t pcmBuffer[BUFFER_SAMPLES];
    size_t bytesRead;

    i2s_read(I2S_PORT, i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);

    int samples = bytesRead / sizeof(int32_t);

    for (int i = 0; i < samples; i++) {
      pcmBuffer[i] = (int16_t)(i2sBuffer[i] >> 16);
    }

    audioFile.write((uint8_t*)pcmBuffer, samples * sizeof(int16_t));
    bytesWritten += samples * sizeof(int16_t);

    if (bytesWritten >= SAMPLE_RATE * RECORD_TIME * 2) {
      stopRecording();
    }
  }
}
