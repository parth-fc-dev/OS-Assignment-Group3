#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);
int actualValue = 0;
void setup()
{
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop()
{
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0)*2.5/100;
  // print out the value you read:
  if(sensorValue < actualValue -1 || sensorValue > actualValue + 2  ){
    actualValue = sensorValue;
    if(actualValue > 100){
      actualValue = 100;
    }
    char buffer[5];
    snprintf(buffer, sizeof(buffer), "%03d", actualValue); // Format the value with leading zeros
    Serial.println(buffer); // Print the formatted value
  }
  // int actualValue = sensorValue * 2.5;
  delay(100); // delay in between reads for stability
}