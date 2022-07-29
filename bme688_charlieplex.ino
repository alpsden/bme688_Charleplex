#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <Adafruit_IS31FL3731.h>
#include <Wire.h>
#include "bsec.h"

#define LED_PIN 13
#define BME688_ADDRESS 0X77
#define MODE_BUTTON 10
// Number of display modes
#define MODES 3

// BSEC instance
Bsec iaqSensor;
String output;

// Charlie plex led instance
Adafruit_IS31FL3731 matrix = Adafruit_IS31FL3731();

// display modes method definition
void dataDisplayMode(uint8_t mode);

// Helper Functions for BSEC and BME688
void checkIaqSensorStatus(void);
void errLeds(void);

#define INTERVAL_MESSAGE_SCROLL 50 // refresh rate for scrolling text

unsigned long refreshTimer = 0;
unsigned long debounce = 0;

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  // BME688 init
  iaqSensor.begin(BME688_ADDRESS, Wire);
  checkIaqSensorStatus();
  matrix.setTextWrap(false); // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(50);

  bsec_virtual_sensor_t sensorList[6] = {
      BSEC_OUTPUT_IAQ,
      BSEC_OUTPUT_STATIC_IAQ,
      BSEC_OUTPUT_CO2_EQUIVALENT,
      BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
      BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  // Led matrix init
  if (!matrix.begin())
  {
    Serial.println("matrix not found");
  }

  pinMode(MODE_BUTTON, INPUT_PULLUP);
}

// Global  variables to control text show and value type
bool showText = true;
bool showVal = false;
int8_t oldVal = 0;
int8_t frameMoveCounter = 0;

void loop()
{
  debounce = millis();
  matrix.setRotation(0);
  if (iaqSensor.run())
  { // If new data is available
  }
  else
  {
    checkIaqSensorStatus();
  }

  static uint8_t select = 0;
  dataDisplayMode(select);

  // if button pressed
  if (digitalRead(MODE_BUTTON) == LOW)
  {
    select++; // change display mode

    showText = true;
    showVal = false;

    refreshTimer = millis(); // set to most recent millis() value for accurate comparison and delay vals

    oldVal = -10; // clear oldVal variable to load the value to be displayed

    frameMoveCounter = 5;
    if (select > MODES)
    {
      select = 0; // round robin
    }

    Serial.println(String(select));
    while (1)
    {
      if (millis() >= debounce + 100) // debounce timer
      {
        if (digitalRead(10) == HIGH)
        {
          break;
        }
      }
    }
  }
}

// Constructing standard frames for different modes to be displayed
void dataDisplayconstruct(int16_t scrollTextLen, String scrollString, int16_t outputval)
{
  if (millis() >= refreshTimer + INTERVAL_MESSAGE_SCROLL)
  {
    refreshTimer += INTERVAL_MESSAGE_SCROLL;
    if (frameMoveCounter > scrollTextLen && showText)
    {
      matrix.clear();
      matrix.setCursor(frameMoveCounter, 1);
      String outStr = scrollString;
      matrix.print(outStr);
      frameMoveCounter--;
    }
    else
    {
      frameMoveCounter = 0;
      showText = false;
      showVal = true;
    }
    if (showVal)
    {
      uint16_t newVal = outputval;
      if (newVal != oldVal) // to avoid flickering due to refresh rate
      {
        oldVal = newVal;
        matrix.clear();
        if (newVal >= 100) // adjusting positioning of the value based on the sensor value
        {
          matrix.setCursor(0, 1);
        }
        else
        {
          matrix.setCursor(3, 1);
        }
        matrix.print(outputval);
      }
    }
  }
}

// data display function
void dataDisplayMode(uint8_t mode)
{
  switch (mode)
  {
  case 0:
    if (millis() >= refreshTimer + INTERVAL_MESSAGE_SCROLL)
    {
      refreshTimer += INTERVAL_MESSAGE_SCROLL;
      if (frameMoveCounter > -127)
      {
        matrix.clear();
        matrix.setCursor(frameMoveCounter, 1);
        String outStr = "TEMP:" + String((int)round(iaqSensor.temperature)) + " " + "IAQ:" + String((int)round(iaqSensor.iaq)) + " " + "HUM:" + String((int)round(iaqSensor.humidity));
        matrix.print(outStr);
        frameMoveCounter--;
      }
      else
      {
        frameMoveCounter = 17;
      }
    }
    break;

  case 1:
    dataDisplayconstruct(-30, "TEMP", (int16_t)round(iaqSensor.temperature));
    break;

  case 2:
    dataDisplayconstruct(-50, "HUMIDITY", (int16_t)(iaqSensor.humidity));
    break;

  case 3:
    dataDisplayconstruct(-20, "IAQ", (int16_t)(iaqSensor.iaq));
    break;

  default:
    break;
  }
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK)
  {
    if (iaqSensor.status < BSEC_OK)
    {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK)
  {
    if (iaqSensor.bme680Status < BME680_OK)
    {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    }
    else
    {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
}
