/*****************************************************************************************************
  lora Programs for Arduino - Copyright of the author Stuart Robinson - 16/03/20

  This program is supplied as is, it is up to the user of the program to decide if the program is
  suitable for the intended purpose and free from errors.
*******************************************************************************************************/

#define Program_Version "V1.0"

#include <SPI.h>
#include <SX128XLT.h>
#include "Settings.h"

SX128XLT LT;


#ifdef ENABLEOLED
#include <U8x8lib.h>                                        //https://github.com/olikraus/u8g2 
U8X8_SSD1306_128X64_NONAME_HW_I2C disp(U8X8_PIN_NONE);      //standard 0.96" SSD1306
//U8X8_SH1106_128X64_NONAME_HW_I2C disp(U8X8_PIN_NONE);     //1.3" OLED often sold as 1.3" SSD1306
#endif



uint16_t rangeing_errors, rangeings_valid, rangeing_results;
uint16_t IrqStatus;
uint32_t endwaitmS, startrangingmS, range_result_sum, range_result_average;
//uint32_t total_ranging_OK;
float distance, distance_sum, distance_average;
bool ranging_error;
int32_t range_result;


void loop()
{
  uint8_t index;
  distance_sum = 0;
  range_result_sum = 0;
  rangeing_results = 0;                           //count of valid results in each loop

  for (index = 1; index <= rangeingcount; index++)
  {

    startrangingmS = millis();

    LT.transmitRanging(RangingAddress, TXtimeoutmS, RangingTXPower, WAIT_TX);

    IrqStatus = LT.readIrqStatus();

    if (IrqStatus & IRQ_RANGING_MASTER_RESULT_VALID)
    {
      rangeing_results++;
      rangeings_valid++;
      digitalWrite(LED1, HIGH);
      Serial.print("Valid");
      range_result = LT.getRangingResultRegValue(RANGING_RESULT_RAW);
      Serial.print(",Register,");
      Serial.print(range_result);

      if (range_result > 800000)
      {
        range_result = 0;
      }
      range_result_sum = range_result_sum + range_result;

      distance = LT.getRangingDistance(RANGING_RESULT_RAW, range_result, distance_adjustment);
      distance_sum = distance_sum + distance;

      Serial.print(",Distance,");
      Serial.print(distance, 1);
      //Serial.print("m");
      digitalWrite(LED1, LOW);
    }
    else
    {
      rangeing_errors++;
      distance = 0;
      range_result = 0;
      Serial.print("NotValid");
      Serial.print(",Irq,");
      Serial.print(IrqStatus, HEX);
    }
    delay(packet_delaymS);

    if (index == rangeingcount)
    {
      range_result_average = (range_result_sum / rangeing_results);

      if (rangeing_results == 0)
      {
        distance_average = 0;
      }
      else
      {
        distance_average = (distance_sum / rangeing_results);
      }
      
      Serial.print(",TotalValid,");
      Serial.print(rangeings_valid);
      Serial.print(",TotalErrors,");
      Serial.print(rangeing_errors);
      Serial.print(F(",AverageRAWResult,"));
      Serial.print(range_result_average);
      Serial.print(F(",AverageDistance,"));
      Serial.print(distance_average, 1);

#ifdef ENABLEDISPLAY
      display_screen1();
#endif
      //Serial.println();
      delay(2000);

    }
    Serial.println();
  }

}

#ifdef ENABLEDISPLAY
void display_screen1()
{
  disp.clear();
  disp.setCursor(0, 0);
  disp.print("Distance ");
  disp.print(distance_average, 1);
  disp.print("m");
  disp.setCursor(0, 2);
  disp.print("OK,");
  disp.print(rangeings_valid);
  disp.print(",Err,");
  disp.print(rangeing_errors);
}
#endif



void led_Flash(uint16_t flashes, uint16_t delaymS)
{
  uint16_t index;

  for (index = 1; index <= flashes; index++)
  {
    digitalWrite(LED1, HIGH);
    delay(delaymS);
    digitalWrite(LED1, LOW);
    delay(delaymS);
  }
}


void setup()
{
  pinMode(LED1, OUTPUT);                                   //setup pin as output for indicator LED
  led_Flash(4, 125);                                       //two quick LED flashes to indicate program start

  Serial.begin(9600);
  Serial.println();
  Serial.print(F(__TIME__));
  Serial.print(F(" "));
  Serial.println(F(__DATE__));
  Serial.println(F(Program_Version));
  Serial.println();
  Serial.println(F("54_Ranging_Master Starting"));

  SPI.begin();

  led_Flash(2, 125);

  if (LT.begin(NSS, NRESET, RFBUSY, DIO1, LORA_DEVICE))
  {
    Serial.println(F("Device found"));
    led_Flash(2, 125);
    delay(1000);
  }
  else
  {
    Serial.println(F("No device responding"));
    while (1)
    {
      led_Flash(50, 50);                                 //long fast flash indicates device error
    }
  }

  LT.setupRanging(Frequency, Offset, SpreadingFactor, Bandwidth, CodeRate, RangingAddress, RANGING_MASTER);

  //LT.setRangingCalibration(Calibration);               //override automatic lookup of calibration value from library table

#ifdef ENABLEDISPLAY
  Serial.println("Display Enabled");
  disp.begin();
  disp.setFont(u8x8_font_chroma48medium8_r);
  disp.setCursor(0, 0);
  disp.print("Ranging RAW Ready");
  disp.setCursor(0, 1);
  disp.print("Power ");
  disp.print(RangingTXPower);
  disp.print("dBm");
  disp.setCursor(0, 2);
  disp.print("Cal ");
  disp.print(Calibration);
  disp.setCursor(0, 3);
  disp.print("Adjust ");
  disp.print(distance_adjustment, 4);
#endif


  Serial.print(F("Address "));
  Serial.println(RangingAddress);
  Serial.print(F("CalibrationValue "));
  Serial.println(LT.getSetCalibrationValue());
  Serial.println(F("Ranging master RAW ready"));

  delay(2000);
}
