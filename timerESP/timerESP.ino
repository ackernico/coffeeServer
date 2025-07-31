#define LED_CLOCKWISE 12
#define LED_COUNTERCLOCKWISE 2
#define FUSISFET 27
#define IN_AUX 14

unsigned long diffTime;

int input;
int tries = 0;

bool isStuck = false;

void setup() 
{
  Serial.begin(115200);
  
  pinMode(LED_CLOCKWISE, OUTPUT);
  pinMode(LED_COUNTERCLOCKWISE, OUTPUT);
  pinMode(IN_AUX, INPUT_PULLUP);

  digitalWrite(LED_CLOCKWISE, HIGH);
  digitalWrite(LED_COUNTERCLOCKWISE, LOW);
  digitalWrite(FUSISFET, HIGH);
}

void loop() 
{
  input = digitalRead(IN_AUX);

  if(tries >= 3)
  {
    digitalWrite(FUSISFET, LOW);
    Serial.println("Motor travado gravemente! Conferir broca.");
  }

  if(input == LOW && !isStuck && tries < 3)
  {
    digitalWrite(LED_CLOCKWISE, LOW);
    digitalWrite(LED_COUNTERCLOCKWISE, HIGH);
    Serial.println("Motor travado! Invertendo rotação...");

    tries++;

    diffTime = millis();
    isStuck = true;
  }

  if(isStuck && (millis() - diffTime >= 2000))
  {
    digitalWrite(LED_COUNTERCLOCKWISE, LOW);
    digitalWrite(LED_CLOCKWISE, HIGH);
    Serial.print("Motor destravado! Tentativas: ");
    Serial.println(3-tries);

    isStuck = false;
  }
}
