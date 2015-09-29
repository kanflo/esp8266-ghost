// Copyright (c) 2015 Johan Kanflo (github.com/kanflo)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#if 1
 #define DEBUG(x)
#else
 #define DEBUG(x) x
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIXEL_PIN            6

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      16

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

String input;

void setPixelColor(unsigned char r, unsigned char g, unsigned char b)
{
  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
  delay(10);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
    WheelPos -= 85;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}


void setup()
{
  Serial.begin(115200);
  Serial.println("\nNeopixel!\n");
  pixels.begin();

#if 1
  setPixelColor(0, 0, 0);
#else
  setPixelColor(255, 0, 0);
  delay(500);
  setPixelColor(0, 255, 0);
  delay(500);
  setPixelColor(0, 0, 255);
  delay(500);
  setPixelColor(0, 0, 0);
#endif
}

void stopProgram()
{
  setPixelColor(0, 0, 0);
}

uint16_t p0_j;
uint32_t p2_j;
uint8_t head, tail;

void startProgram(long programIndex)
{
  switch(programIndex) {
    case 0:
    case 1:
      p0_j = 0;
      break;
    case 2:
      tail = 0;
      head = NUMPIXELS/4;
      setPixelColor(0, 0, 0);
      for(int i = tail; i < tail; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
      }
      pixels.show();
    case 3:
      setPixelColor(0, 0, 0);
      p0_j = p2_j = 0;
      break;
  }
}

void runProgram(long programIndex)
{
  switch(programIndex) {
    case 0:
    {
      uint16_t i;
      for(i = 0; i < pixels.numPixels(); i++) {
        pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + p0_j) & 255));
      }
      pixels.show();
      delay(50);
      p0_j++;
      if (p0_j >= 256*5) {
        p0_j = 0;
      }
    }
      break;
    case 1:
    {
      uint16_t i;
      for(i = 0; i < pixels.numPixels(); i++) {
        pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + p0_j) & 255));
      }
      pixels.show();
      delay(1);
      p0_j++;
      if (p0_j >= 256*5) {
        p0_j = 0;
      }
    }
      break;
    case 2:
    {
      p2_j++;
      if (p2_j > 1000) {
        p2_j = 0;
        pixels.setPixelColor(tail, pixels.Color(0, 0, 0));
        pixels.setPixelColor(head, pixels.Color(0, 0, 255));
        pixels.show();
        tail = (tail + 1) % NUMPIXELS;
        head = (head + 1) % NUMPIXELS;
      }
    }
      break;
    case 3:
    {
      p2_j++;
      if (p2_j > 30000) {
        for(int i=0; i<3; i++) {
          setPixelColor(0, 0, 255);
          pixels.show();
          setPixelColor(0, 0, 0);
          pixels.show();
          delay(40);
        }
        p2_j = 0;
      }
    }
      break;
  }
}


/*

  Messages are formatted as:
 
  #RRGGBB   Set all LEDs to RRGGBB (eg #FF0000 for red)
  :nnRRGGBB Set LED nn to RRGGBB (eg #08FF0000 sets LED 08 to red)
  !         Set all LEDs changd by the : command
  pnn       Start program nn
  *         Stop program

  The driver will respond with the single character '!' when it has received a valid command
  and '?' if it receives an invalid command.

 */

void reportOk(void)
{
  Serial.print("!");
}

void reportError(void)
{
  Serial.print("?");
}

#define MODE_NONE          0
#define MODE_ALL_LEDS      1
#define MODE_SINGLE_LED    2
#define MODE_PROGRAM_START 3
#define MODE_PROGRAM_RUN   4
#define MODE_PROGRAM_STOP  5

int currentMode = MODE_NONE;

void loop()
{
  static long currentProgram = 0;
  if (!Serial.available()) {
    if (currentMode == MODE_PROGRAM_RUN) {
      runProgram(currentProgram);
    }
  } else {
    char c = Serial.read();
    switch(c) {
      case '#':
        currentMode = MODE_ALL_LEDS;
        DEBUG(Serial.println(">ALL");)
        input = "";
        break;
      case ':':
        currentMode = MODE_SINGLE_LED;
        DEBUG(Serial.println(">SINGLE");)
        input = "";
        break;
      case '!':
        pixels.show();
        input = "";
        break;
      case 'p':
        currentMode = MODE_PROGRAM_START;
        DEBUG(Serial.println(">PROGRAM");)
        input = "";
        break;
      case '*':
        currentMode = MODE_PROGRAM_STOP;
        stopProgram();
        input = "";
        reportOk();
        currentMode = MODE_NONE;
        break;
      default:
        input += c; 
        switch(currentMode) {
          case MODE_ALL_LEDS:
            if (input.length() == 6) {
              DEBUG(Serial.print("Parsing:");)
              DEBUG(Serial.println(input);)
              long rgb = (long) strtol( &input[0], NULL, 16);
              int r = (rgb >> 16) & 0xff;
              int g = (rgb >> 8) & 0xff;
              int b = rgb & 0xff;
              DEBUG(Serial.print("ALL pixels:");)
              DEBUG(Serial.print(r);)
              DEBUG(Serial.print(" ");)
              DEBUG(Serial.print(g);)
              DEBUG(Serial.print(" ");)
              DEBUG(Serial.println(b);)
              setPixelColor(r, g, b);
              input = "";
              currentMode = MODE_NONE;
              reportOk();
            }
            break;
          case MODE_SINGLE_LED:
            if (input.length() == 8) {
              DEBUG(Serial.print("Parsing:");)
              DEBUG(Serial.println(input);)
              long irgb = (long) strtol( &input[0], NULL, 16);
              int i = (irgb >> 24) & 0xff;
              int r = (irgb >> 16) & 0xff;
              int g = (irgb >> 8) & 0xff;
              int b = irgb & 0xff;
              if (i < NUMPIXELS) {
                DEBUG(Serial.print("Single pixel:");)
                DEBUG(Serial.print(i);)
                DEBUG(Serial.print(" color:");)
                DEBUG(Serial.print(r);)
                DEBUG(Serial.print(" ");)
                DEBUG(Serial.print(g);)
                DEBUG(Serial.print(" ");)
                DEBUG(Serial.println(b);)
                pixels.setPixelColor(i, pixels.Color(r, g, b));
                reportOk();
                currentMode = MODE_NONE;
                input = "";
              } else {
                reportError();
              }
            }
            break;
          case MODE_PROGRAM_START:
            if (input.length() == 2) {
              currentProgram = (long) strtol( &input[0], NULL, 16);
              DEBUG(Serial.print("Program start:");)
              DEBUG(Serial.println(currentProgram);)
              startProgram(currentProgram);
              currentMode = MODE_PROGRAM_RUN;
              reportOk();
              input = "";
            }
            break;
          default:
            input = "";
            reportError();
        }
    }
  }
}
