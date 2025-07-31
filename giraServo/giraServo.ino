#include <ESP32Servo.h>

Servo testServo;

      int position = 0; //Variable to store the servo position
const int servoPin = 18; //GPIO D18
const int button1Pin = 15;

void testaServo()
{
  for(int i=0 ; i<=180 ; i++)
  {
    testServo.write(i);
    Serial.println(i);
    delay(20);
  }

  for(int i=180 ; i>=0 ; i--)
  {
    testServo.write(i);
    Serial.println(i);
    delay(20);
  }
}

void setup() 
{
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  testServo.setPeriodHertz(50);
  testServo.attach(servoPin, 1000, 2000);
  testServo.write(0);

  pinMode(button1Pin, INPUT_PULLDOWN);
}

void loop() 
{
  if(digitalRead(button1Pin) == HIGH)
  {
    testServo.write(0);
    Serial.println("Abriu");
  }
  else if(digitalRead(button1Pin) == LOW)
  {
    testServo.write(180);
    Serial.println("Fechou");
  }
}
