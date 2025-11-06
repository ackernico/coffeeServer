#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define CLK_PIN 23
#define DT_PIN 19
#define SW_PIN 5

LiquidCrystal_I2C lcd(0x27, 20, 4);

int lastCLKState;
int counter = 0;
unsigned long lastTurnTime = 0;
unsigned long lastButtonTime = 0;
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);

  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rotary Encoder");

  lastCLKState = digitalRead(CLK_PIN);

  delay(1000);
  lcd.clear();
}

void loop() {
  int currentCLKState = digitalRead(CLK_PIN);
  int currentDTState  = digitalRead(DT_PIN);

  // Detecta mudança com um pequeno debounce (5 ms)
  if (currentCLKState != lastCLKState && (millis() - lastTurnTime > 5)) {
    if (currentDTState != currentCLKState) {
      counter++;  // Horário (CW)
      Serial.println("CW");
    } else {
      counter--;  // Anti-horário (CCW)
      Serial.println("CCW");
    }

    // Atualiza display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Contador:");
    lcd.setCursor(0, 1);
    lcd.print(counter);

    lastTurnTime = millis();
  }

  lastCLKState = currentCLKState;

  // Botão com debounce
  if (!digitalRead(SW_PIN) && millis() - lastButtonTime > 250) {
    lastButtonTime = millis();
    buttonPressed = !buttonPressed;

    lcd.setCursor(0, 2);
    lcd.print("Botao: ");
    lcd.print(buttonPressed ? "PRESS." : "SOLTO ");
    Serial.println(buttonPressed ? "Botao --pressionado" : "Botao solto");
  }
}