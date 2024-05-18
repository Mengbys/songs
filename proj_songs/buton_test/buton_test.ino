#include <PinChangeInterrupt.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptSettings.h>

int buttonPin=A0;

void setup() {
  // put your setup code here, to run once:
  pinMode(buttonPin,INPUT_PULLUP);
  attachPCINT(digitalPinToPCINT(buttonPin),buttonPress,FALLING);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

}

void buttonPress()
{
  Serial.println("Button is pressed!");
}
