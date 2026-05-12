#include "config.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Pins
#define TOUCH_PIN D2
#define VIB_PIN   D5
#define LED_PIN   D4

bool touched = false;
bool vibrated = false;

void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(VIB_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  int touchState = digitalRead(TOUCH_PIN);
  int vibState   = digitalRead(VIB_PIN);

  if (touchState == HIGH && !touched) {
    touched = true;
    blinkLED();
    Blynk.logEvent("touch_alert", "Touch detected!");
  }

  if (touchState == LOW) touched = false;

  if (vibState == HIGH && !vibrated) {
    vibrated = true;
    blinkLED();
    Blynk.logEvent("touch_alert", "Vibration detected!");
  }

  if (vibState == LOW) vibrated = false;

  delay(100);
}

void blinkLED() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(200);
    digitalWrite(LED_PIN, HIGH);
    delay(200);
  }
}
