/*
   Description of virtual pins:
   V1, V4: measured distance in inches
   V2: countdown timer; range is timeOut..0
   V3: isDoorOpen (0 or 255 for android LED action)
   V4: uptime in minutes
*/

const int pinEcho = 10;
const int pinTrig = 11;

const int distanceMax = 24; //distance in inches beyond which the door is deemed open
const unsigned int timeOut = 3 * 60; //timeout in seconds
float distance = 0;
unsigned long timeDoorOpen = 0;
bool isDoorOpen = false;
bool isEmailSent = false;
bool LEDstate = LOW;

#include <Adafruit_SleepyDog.h>
#include <SimpleTimer.h>
SimpleTimer timer;

//#define BLYNK_DEBUG
#define BLYNK_PRINT Serial // Enables Serial Monitor
#include <BlynkSimpleMKR1000.h>
char auth[] = "165c2230546d422fbe05d94588578891";

#include <WiFi101.h>
char ssid[] = "David";     //  your network SSID (name)
char pass[] = "engineer";  // your network password


void setup()
{
  Serial.begin(9600);
  delay(1000);
//  unsigned long serial_time = millis();
//  while (!Serial) {
//    if (serial_time + 3000 < millis()) break;
//  }
  Serial.println();
  Serial.println();


  int countdownMS = Watchdog.enable();
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
  Serial.println();

  pinMode(pinEcho, INPUT);
  pinMode(pinTrig, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Blynk.begin()...");
  Blynk.begin(auth, ssid, pass);

  timer.setInterval(4000L, onTimer);
}


void loop()
{

  Watchdog.reset();
  timer.run();
  Blynk.run(); // All the Blynk Magic happens here...
  Watchdog.reset();

  LEDstate = !LEDstate;
  digitalWrite(LED_BUILTIN, LEDstate);

}



void onTimer()
{
  // uptime in minutes
  Blynk.virtualWrite(V4, millis()/1000/60);
  
  distance = microsecondsToInches(ping_median(10));

  Serial.println();
  Serial.print(distance);
  Serial.print("in, ");
  Serial.println();
  Serial.println();


  if (distance < distanceMax)
  {
    if (!isDoorOpen)
    {
      timeDoorOpen = millis() / 1000; //first time through, take note of the first time the door was open
    }
    isDoorOpen = true;
  }
  else
  {
    isDoorOpen = false;
    isEmailSent = false;
    timeDoorOpen = 0;
    Blynk.virtualWrite(V2, -1);//countdown timer
  }

  if (isDoorOpen && !isEmailSent)
  {
    if ((timeDoorOpen + timeOut) <= (millis() / 1000))
    {
      Serial.println("sending email");
      Blynk.email("7802977177@txt.windmobile.ca", "Blynk!", "garage door is open");
      isEmailSent = true;
    }
    Serial.println((timeDoorOpen + timeOut - (millis() / 1000)));
    Blynk.virtualWrite(V2, (timeDoorOpen + timeOut - (millis() / 1000)));
  }

  Blynk.virtualWrite(V1, distance);
  //Blynk.virtualWrite(V4, distance);
  Blynk.virtualWrite(V3, isDoorOpen * 255);
}

unsigned long doPing()
{

  Serial.print("doPing() ");

  pinMode(pinTrig, OUTPUT);
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(5);
  digitalWrite(pinTrig, LOW);

  pinMode(pinEcho, INPUT);
  return pulseIn(pinEcho, HIGH);

}

const int NO_ECHO = 0;
const int PING_MEDIAN_DELAY = 2 * 29000; //microseconds

unsigned long ping_median(uint8_t it) {

    
  unsigned int uS[it], last;
  uint8_t j, i = 0;
  unsigned long t;
  uS[0] = NO_ECHO;

  while (i < it) {
    t = micros();  // Start ping timestamp.
    last = doPing(); // Send ping.

    if (last != NO_ECHO) {       // Ping in range, include as part of median.
      if (i > 0) {               // Don't start sort till second ping.
        for (j = i; j > 0 && uS[j - 1] < last; j--) // Insertion sort loop.
          uS[j] = uS[j - 1];     // Shift ping array to correct position for sort insertion.
      } else j = 0;              // First ping is sort starting point.
      uS[j] = last;              // Add last ping to array in sorted position.
      i++;                       // Move to next ping.
    } else it--;                 // Ping out of range, skip and don't include as part of median.

    if (i < it && micros() - t < PING_MEDIAN_DELAY)
      delay((PING_MEDIAN_DELAY + t - micros()) / 1000); // Millisecond delay between pings.

  }
  return (uS[it >> 1]); // Return the ping distance median.
}

float microsecondsToInches(float microseconds) {
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74.0 / 2;
}
