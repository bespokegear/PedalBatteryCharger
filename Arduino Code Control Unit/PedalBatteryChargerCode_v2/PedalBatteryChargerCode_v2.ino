/********************************************************
  /****** Energy Monitor Pedal Unit CODE ******************
  /****** by Matt Little **********************************
  /****** Date: 25/10/2019 ********************************
  /****** info@re-innovation.co.uk ************************
  /****** www.re-innovation.co.uk *************************
  /********************************************************

  /*************Details of Code*****************************

   The PCB/schematic files are available from: www.re-innovation.co.uk

   It also reads voltage from the input, with a potential divider and 5V1 Zener protection
   This does NOT have a current shunt

   Voltage is read on A0 - via 680k/47k voltage divider
   Voltage set-point is adjustable via serial code.

   Current is read on pin A2 through a ADS 50A hall effect sensor

  Power data is read on the serial port at 115200 baud
   ****protocol for straming data to serial****

  There are a number of functions which can be adjusted via software commands. The main protocol is based upon LLAP (Lightweight Local Automation Protocol), with some additional features.
  http://openmicros.org/index.php/articles/85-llap-lightweight-local-automation-protocol/101-llap-starter
  This uses a 12 character message in the format:
  “aXXDDDDDDDDD”
  Where a is the start signifier and XX is the device ID, from AA to ZZ.
  Everything else is in upper case.

  Commands this unit can process:
  Change device ID
  “aXXCHID??---“
  Where the XX is the old ID and ?? is the new ID. This is stored to EEPROM

  Power
  “aXXPDDDDD---“
  This is the power output (0000.0W to 9999.9W). This will be an output approximately every second.


  Updates:
  15/10/2019  Basic code written

**********************************************************************************************************/
/************ External Libraries*****************************/

#include <stdlib.h>
#include "config.h"

//*********END of hardware information****************************************

String outputString;    // This is the holder for the data as a string. Start as blank

unsigned long int currentAve = 0;  // To hold the average values
unsigned long int voltageAve = 0;

long int oldTime = millis();  // hold the time for output

float voltageFloat = 0;  // This holds the average calculation value
float currentFloat = 0;  // This holds the average calculation value
float powerFloat = 0;    // Holds power as float
unsigned int powerInt = 0;  // Holds the power as an int

float voltageConversion = (RESISTORTOP + RESISTORBOTTOM) / RESISTORBOTTOM; // This is the conversion of the voltage read

long int averageCounter = 0;  // This counts the number of times an average has been taken.

//********** This subroutine is used for outputting the power value
void displayPower()
{
  // **************DISPLAY POWER************************
  //  These values will be printed to output when required.
  //  Power
  //  “aXXPDDDDE---“
  //  This is the power output (0000W to 9999W). This will be an output approximately every second.
  //
  outputString = "a";
  outputString += DEVICEID;
  outputString += "P";
  // Here we add the power in W from 0000W to 9999W
  // powerave is in Wso we need to sort that out
  // If powerave is <1000 then add a zero, if <100 then add two zeros, if <10 the all zeros
  if (powerInt < 10)
  {
    outputString += "0000";
    outputString += String(powerInt);
  }
  else if (powerInt < 100)
  {
    outputString += "000";
    outputString += String(powerInt);
  }
  else if (powerInt < 1000)
  {
    outputString += "00";
    outputString += String(powerInt);
  }
  else if (powerInt < 10000)
  {
    outputString += "0";
    outputString += String(powerInt);
  }
  else
  {
    outputString += String(powerInt);
  }
  outputString += "----";
  Serial.println(outputString);
}


// Subroutine to read current and voltage
void readVI()
{
  // Firstly lets take an average of the voltage and current values:
  for (int i = 0; i < AVERAGECOUNTER; i++)
  {
    currentAve += analogRead(ANALOGCURRENT);
    voltageAve += analogRead(ANALOGVOLTAGE);
    delay(1);
  }
  // Then calculate the average values:
  currentFloat = (float)currentAve / (float)AVERAGECOUNTER;
  voltageFloat = (float)voltageAve / (float)AVERAGECOUNTER;

  currentAve = 0;  // Reset the average values
  voltageAve = 0;

  if (DEBUG == 1)
  {
    Serial.print("Current Reading: ");
    Serial.println(currentFloat);
    Serial.print("Voltage Reading: ");
    Serial.println(voltageFloat);
  }

  //  // *********Current conversion************************
  // Using an allegro ACS756 SCA-050B Hall-effect current sensor. 50A Unidirectional
  // Datasheet: https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=3&cad=rja&uact=8&ved=2ahUKEwjDr4_MrbrlAhWCUhUIHYuKBPoQFjACegQIARAC&url=https%3A%2F%2Fwww.allegromicro.com%2F~%2Fmedia%2FFiles%2FDatasheets%2FACS758-Datasheet.ashx&usg=AOvVaw0zG5YncHyc4UEWo88RnEEg
  // Output the currentint here FOR TESTING:
  // Power supply is 4.5V so:
  // Offset is 0.1 x Vcc which is 0.425 at 4.25V supply
  // Sensitivity is 60mV per A (at 5V) so (60/5)*4.25 = 51mV per amp at 4.25V

  // Convert currentFLoat into measured voltage:
  currentFloat  = (currentFloat * ANALOGREF) / 1024.0;
  // Convert currentFloat into actual current in amps
  currentFloat  = (((currentFloat - (VSUPPLY / 2)) * (CURRENTCONV * (VSUPPLY / 5.0))) * -1.0); // This does the ratiometric calculation of current
  if (currentFloat <= 0)
  {
    currentFloat = 0;
  }

  //currentFloat = 5.0; // DEBUG test line

  //*******Voltage Conversion**************************
  // Voltage is measured with a potential divider - set values in CONFIG
  voltageFloat = (voltageFloat * (float)ANALOGREF) / 1024.0;
  voltageFloat = (voltageFloat * voltageConversion); // Voltage in V
  if (voltageFloat <= 0)
  {
    voltageFloat = 0;
  }
  //*************POWER CACULATION********
  // This is not extremly accurate - as it only take the average values
  // But good enough for high powers and quick for this item
  powerFloat = voltageFloat * currentFloat;
  //powerAve = powerAve / 1000000; // Gets rid of sampling freq - need to include display update here?
  powerFloat = powerFloat * 10; //send data as 100's of mW
  powerInt = (int)powerFloat;

  if (DEBUG == 1)
  {
    Serial.print("Current Actual: ");
    Serial.println(currentFloat);
    Serial.print("Voltage Actual: ");
    Serial.println(voltageFloat);
    Serial.print("Power Actual: ");
    Serial.println(powerInt);
  }


}

//****************INITIALISE ROUTINE******************************
void setup()
{
  Serial.begin(9600);    // Set up a serial output for data display and changing parameters
  // declare correct inputs and outputs:
  pinMode(ANALOGCURRENT, INPUT);
  pinMode(ANALOGVOLTAGE, INPUT);
  analogReference(EXTERNAL);
  Serial.print("Voltage Conversion:");
  Serial.println(voltageConversion);
}

void loop()
{
  // This is the main loop.

  // Firstly lets take an average of the voltage and current values
  readVI();

  // ***********Display data********************************
  // Here we keep a count of the milliseconds
  // If more than DISPLAYUPDATEMS has passed then send out the data
  // Output data on serial:
  if ((millis()) >= (oldTime + DISPLAYUPDATEMS))
  {
    oldTime = millis();  // Save the time ready for the next output display
    // Output the voltage phrase:
    displayPower();
  }
}
