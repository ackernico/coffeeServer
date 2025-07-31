#define OUTPIN 35
#define OFFSET 3102

float motorVoltage = 3.6;
float currentValue;
int adcValue;

int currentFilter()
{
  long allCurrents = 0;
  for(int i = 0; i < 1000; i++)
  {
    allCurrents += analogRead(OUTPIN);
    delayMicroseconds(100);
  }
  allCurrents = allCurrents / 1000;
  return allCurrents;
}

float calcCurrent(int filteredSignal)
{
  float aux = ((filteredSignal - OFFSET) * 3.3) / (4095 * 0.1) + 1.98;
  return aux;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  adcValue = currentFilter();
  currentValue = calcCurrent(adcValue);

  Serial.printf("ADC Value: %i | Current value: %.2f A\n", adcValue, currentValue);
  delay(500);
}