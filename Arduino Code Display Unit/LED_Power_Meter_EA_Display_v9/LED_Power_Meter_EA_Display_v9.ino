/********************************************************
  /****** LED Power Meter Display CODE ********************
  /****** by Matt Little **********************************
  /****** Date: 13/3/2017 ************************************
  /****** info@re-innovation.co.uk ************************
  /****** www.re-innovation.co.uk *************************
  /********************************************************

  See www.re-innovation.co.uk for information and
  construction details

  This is code for the Embedded Adventures LED display unit:
  https://www.embeddedadventures.com/datasheets/PLT-1001_hw_v4_doc_v3.pdf

  This unit takes values of Time and Power and displays them on a large LED matrix
  Time Mode:
  The data is sent "aXXT0001-----" where XX is aa reference (does not matter which)
  The time is a three digit value of 100's milli-seconds (eg 0.1, 1.2, 10.5 etc)
  Want the display to take serial data and update the LED display
  Want a display to show 3 - 2 - 1 GO then show

  /*************Details of Code*****************************

   The PCB/schematic files are available from: www.re-innovation.co.uk

  This code is for an Arduino Nano

  It reads serial data (on D0/D1, Tx/Rx).
  It parses that data and displays the power on a large Embedded Adventures serial LED display.

  Display connections:
  EA controller is on Tx = D12, Rx = D11

  Serial Input is on Tx = D0, Rx = D1

  Two switches are included on D5 and D9

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

  Voltage and Current
  “aXXVDDDIDDDD“
  This is the voltage and current output (V is 00.0 to 99.9V DC) (I is 00.00A to 99.99A). This will be an output approximately every second.

  Maximum graph power
  “aXXMGP????--“
  This is for adjusting the display bar graph height to show the max power displayed:
  Max power value = 32 LEDs lit up in a column. Where ???? is the power in watts.
  This is stored to EEPROM. Device ID does not matter

  Also calculates the energy in Wh to display

  Updates:
  06/06/2016 Code started - Matt Little
  13/3/2017 update of code for timer - Matt Little
  16/3/2017 Added new functions for game style unit - Matt Little
  25/10/2019 Changed to show energy (no timer functions in this version) - Matt Little

**********************************************************************************************************/
/************ External Libraries*****************************/
// General configuration
#include "config.h"

#include <stdlib.h>
#include <avr/pgmspace.h>  // Library for putting data into program memory
#include <EEPROM.h>        // For writing values to the EEPROM
#include <avr/eeprom.h>    // For writing values to EEPROM
#include <SoftwareSerial.h>
#include <Arduino.h>
#include <MutilaDebug.h>
#include <Millis.h>
#include <DebouncedButton.h>
/************User variables and hardware allocation**********************************************/

// ********** Hardware information********************************************
// These constants won't change.  They're used to give names to the pins used:
// Set up the software serial line to put data to the display
SoftwareSerial mySerial(12, 11); // RX, TX

//*********END of hardware information****************************************

// Useful variables
char deviceID[3]; // A buffer to hold the device ID

unsigned int graphDisplay[64];  // A buffer to hold the graphical bar graph display
int maxGraphPower;  // Power in Watts which is max power to display (=16 on output)
String maxGraphPowerStr;

String outputString;    // This is the holder for the data as a string. Start as blank

// ****** Serial Data Read***********
// Variables for the serial data read
char inByte;         // incoming serial char
String str_buffer = "";  // This is the holder for the string which we will display
#define MAX_STRING 25     // Sets the maximum length of string probably could be lower
char stringBuffer[MAX_STRING];  // A buffer to hold the string when pulled from program memory

// Varibales for writing to EEPROM
int hiByte;      // These are used to store longer variables into EERPRPROM
int loByte;
long int number;

int averagecounter = 1000;  // The number of samples to average over
int displaycounter = 20;   // When to update the display
int displaycountermax = 20;  // This is when to display the data ( x 10mS)

long int oldTime = 0;  // This is for outputting the data every 1S or so

int averageCounter = 100;  // This counts the number of times an average has been taken.
unsigned long int time = 0;  // This holds the averaging

int displayOffset = 0;  // This is for putting the power data in the middle of the display.
int displayOffsetWh = 0;  // This is for putting the power data in the middle of the display.

// Commands for writing to serial port (put into const to save RAM)

int powerAve = 0;  // DC power in
String powerAveStr; // Holds the power value as a string

float energyWs = 0;  // DC power in
float energyWh = 0;  // DC power in
char energyStr[5] = ""; // Holds the power value as a string

int timeValue = 0;  // Holds the time in 100's milliseconds. Aldo holds the Count Down value
String timeValueStr;

int displayMode = 1; // Display Mode 0: Power, 1: Time

const String hello = "HELLO";
const String paint = "paint\r";
const String clearLED = "clear\r";

// Sort out reset button
DebouncedButton ResetButton(RESET_BUTTON_PIN, true);


boolean checkStringOK(String dataString)
{
  boolean fail = true; // Holds the check
  // Check the string is all numbers, not characters
  for (int n = 0; n < dataString.length(); n++)
  {
    if (isDigit(dataString[n]) == false)
    {
      fail = false;
    }
  }
  return (fail);
}


//****************INITIALISE ROUTINE******************************
void setup()
{
  Serial.begin(9600);    // Set up a serial output for data display and changing parameters

  Serial.flush();

  // set the data rate for the SoftwareSerial port
  mySerial.begin(115200);

  // Initialize button objects
  ResetButton.begin();

  // Read in the values from EEPROM
  // Read the device ID
  deviceID[0] = char(EEPROM.read(0));
  deviceID[1] = char(EEPROM.read(1));
  Serial.print("Device ID is: ");
  Serial.print(deviceID[0]);
  Serial.println(deviceID[1]);

  // Read in the max Graph Power
  hiByte = EEPROM.read(9);
  loByte = EEPROM.read(10);
  maxGraphPower = (hiByte << 8) + loByte;
  Serial.print("Max Graph Power: ");
  Serial.println(maxGraphPower);

  for (int z = 0; z < 64; z++)
  {
    // Ensure the graphDisplay buffer is clear
    graphDisplay[z] = 0;
  }

  // Clear the display
  mySerial.print(clearLED);
  mySerial.print(clearLED);
  delay(20);
  mySerial.print(paint);
  delay(20);
  mySerial.print("font 6\r");
  mySerial.print("text 2 3 24 ");
  mySerial.print('"');
  mySerial.print(hello);  // Start message
  mySerial.print('"');
  mySerial.print("\r");
  // Then we display it all
  mySerial.print(paint);
  delay(500); // Show this message for a little bit at least

}

void loop()
{
  // This is the main loop.

  //*********Here we check to see if any serial data has been seen***********
  getData();  // Check the serial port for data
  // Check Buttons
  ResetButton.update();

  if (ResetButton.pushed() == true)
  {
    energyWs = 0;  // Reset the energy value
    //Serial.println("Pressed");
  }

  //Decide which mode depending up Time or Power data being sent
  switch (displayMode)
  
  {
    case 0:
      // ********** POWER MODE *********************************
      // ***********Display data********************************
      // Here we keep a count of the milliseconds
      // If more than 1000mS has passed then update the display
      // Output data on serial:
      if ((millis()) >= (oldTime + DISPLAYUPDATEMS))
      {



        // We enter this point every 1000mS (approx, it might be out by a bit (+/-5mS))

        // Sort out the graphDisplay with the power values
        // First move all the power values along
        // Then fit the new powerAve into the buffer
        for (int z = 1; z < 64; z++)
        {
          // Shift all the graph display values along
          graphDisplay[z - 1] = graphDisplay[z];
        }
        graphDisplay[63] = powerAve;

        // Output things to the Embedded Adventures display unit (serial)
        // More info can be found here: http://www.embeddedadventures.com/datasheets/PLT-1001_hw_v4_doc_v3.pdf

        // Here we calculate the offset for the power text so that it stays to one side
        // The display is 64 px wide.
        // Each number is 10+1(space) px wide
        // So if power is 0-9W, Offset is 38
        // So if power is 10-99W, Offset is 27
        // So if power is 100-999W, Offset is 16
        // So if power is 1000-9999W, Offset is 5

        if (powerAve >= 0 && powerAve < 10)
        {
          displayOffset = 38;
        }
        else if (powerAve >= 10 && powerAve < 20)
        {
          displayOffset = 29;
        }
        else if (powerAve >= 20 && powerAve < 100)
        {
          displayOffset = 27;
        }
        else if (powerAve >= 100 && powerAve < 200)
        {
          displayOffset = 18;
        }
        else if (powerAve >= 200 && powerAve < 1000)
        {
          displayOffset = 16;
        }
        else if (powerAve >= 1000 && powerAve < 2000)
        {
          displayOffset = 7;
        }
        else if (powerAve >= 2000 && powerAve < 10000)
        {
          displayOffset = 5;
        }

        // Clear the LED display (just in case)
        mySerial.print(clearLED);
        // Then also want to display a scrolling bar graph of power along the bottom
        // This takes the values in graphDisplay and creates a line from 0 to value all along the bottom

        // To draw a line we need to write the command:
        //  "line colour x1 y1 x2 y2" for every data point

        // Power values is converted into a graph.
        // The power value is also displayed on top of the graph.

        for (int y = 0; y < 64; y++)
        {
          if (graphDisplay[y] > 0)
          {
            mySerial.print("line 1 ");
            mySerial.print(y);
            mySerial.print(" 31 ");
            //mySerial.print(graphDisplay[y]);
            mySerial.print(y);
            mySerial.print(' ');
            // This is where we draw the bar graph section:
            // We have maxGraphPower = 32 LEDs lit.
            // So to convert from power reading to LEDs to light we use the calculation:
            // (power now / maxGraphPower) *32
            int LEDtoLight = (((float)graphDisplay[y] / (float)maxGraphPower) * 32);
            mySerial.print(31 - LEDtoLight);
            mySerial.print('\r');
            mySerial.print('\r');
          }
        }
        // Write the power
        mySerial.print("font 5\r");
        mySerial.print("text 2 ");
        mySerial.print(displayOffset);  // This ensure the numbers are in the middle of the display
        mySerial.print(" 16 ");
        mySerial.print('"');
        // Want to display the power here:
        mySerial.print(powerAve);
        mySerial.print('w');
        mySerial.print('"');
        mySerial.print('\r');
        delay(2);



        // Sort out the energy value
        // This starts at zero and calculated by adding on the power multiplied by time segment
        energyWs = energyWs + (((float)powerAve / 10.0) * ((millis() - (float)oldTime)) / 1000.0); // This is in Ws
        energyWh = energyWs / 3600.0;
        //        Serial.print("energyWs:");
        //        Serial.print(energyWs);
        //        Serial.print(" energyWh:");
        //        Serial.println(energyWh);

        // Here we calculate the offset for the power text so that it stays to one side
        // The display is 64 px wide.
        // Each number is 10+1(space) px wide, except for 1 which is 8+1 wide!!!
        // So if energyWh is 0.00 -9.99 , Offset is 16
        // So if power is 9.99 - 99.99, change decimal place Offset is 16
        // So if power is 99.99 - 999.9 , change decimal place Offset is 16
        // So if power is 999.9 to 9999.9, change decimal place Offset is 16


        if (energyWh >= 0 && energyWh < 10)
        {
          displayOffsetWh = 16;
          dtostrf(energyWh, 3, 2, energyStr );
        }
        else if (energyWh >= 10 && energyWh < 20)
        {
          displayOffsetWh = 16;
          dtostrf(energyWh, 3, 1, energyStr );
        }
        else if (energyWh >= 20 && energyWh < 100)
        {
          displayOffsetWh = 16;
          dtostrf(energyWh, 3, 1, energyStr );
        }
        else if (energyWh >= 100 && energyWh < 200)
        {
          displayOffsetWh = 16;
          dtostrf(energyWh, 3, 0, energyStr );
        }
        else if (energyWh >= 200 && energyWh < 1000)
        {
          displayOffsetWh = 16;
          dtostrf(energyWh, 3, 0, energyStr );
        }
        else if (energyWh >= 1000 && energyWh < 2000)
        {
          displayOffsetWh = 16;
          dtostrf(energyWh, 4, 0, energyStr );
        }
        else if (energyWh >= 2000 && energyWh < 10000)
        {
          dtostrf(energyWh, 4, 0, energyStr );
          displayOffsetWh = 16;
        }


        //        Serial.print("offset: ");
        //        Serial.println(displayOffsetWh);
        delay(2);
        //Write the energy
        mySerial.print("font 4\r");
        mySerial.print("text 2 ");
        mySerial.print(displayOffsetWh);  // This ensure the numbers are in the middle of the display
        mySerial.print(" 27 ");
        mySerial.print('"');
        // Want to display the power here:
        mySerial.print(energyStr);
        mySerial.print("Wh");
        mySerial.print('"');
        mySerial.print('\r');
        delay(2);
        // Then we display it all
        mySerial.print(paint);
        oldTime = millis();  // Save the time ready for the next output display
      }
      break;

    case 1:
      // *********** TIME MODE *********************************
      // In this mode we have the following commands:
      // aXXCD?--------
      // Where ? = 3,2,1,0 if ? = 0 then we show GO!

      // Nothing really needs to happen here

      break;

    default:
      Serial.println("Bad display mode"); // Error
      break;

  }

  delay(100);   // Short pause to stop everything running too quick
}


// This routine pulls the string stored in program memory so we can use it
// It is temporaily stored in the stringBuffer
char* getString(const char* str) {
  strcpy_P(stringBuffer, (char*)str);
  return stringBuffer;
}

// **********************GET DATA SUBROUTINE*****************************************
// This sub-routine picks up and serial string sent to the device and sorts out a power string if there is one
// All values are global, hence nothing is sent/returned

void getData()
{
  // **********GET DATA*******************************************
  // We want to find the bit of interesting data in the serial data stream
  // As mentioned above, we are using LLAP for the data.
  // All the data arrives as serial commands via the serial interface.
  // All data is in format aXXDDDDDDDDD where XX is the device ID
  while (Serial.available())
  {
    inByte = (char)Serial.read(); // Read whatever is happening on the serial port
    str_buffer += inByte;
    if ((inByte == '-') || (inByte == '\r') || (inByte == '\n'))
    {
      break;
    }
  }
  if (str_buffer[0] == 'a' && ((str_buffer[str_buffer.length() - 1] == '-') || (str_buffer[str_buffer.length() - 1] == '\r') || (str_buffer[str_buffer.length() - 1] == '\n')))
  {
    Serial.println(str_buffer);  // TEST - print the str_buffer data (if it has arrived)
    sortData();
    str_buffer = ""; // Reset the buffer to be filled again
  }
  else
  {
    str_buffer = ""; // Reset the buffer to be filled again
    //Serial.println("Not OK");
  }
  Serial.flush();
}

// **********************SORT DATA SUBROUTINE*****************************************
// This sub-routine takes the read-in data string (12 char, starting with a) and does what is required with it
// The str-buffer is global so we do not need to send it to the routine

void sortData()
{
  // We first want to check if the device ID matches.
  // If it does not then we disregard the command (as it was not meant for this device
  if (str_buffer.substring(1, 3) == deviceID)
  {
    // If yes then we can do further checks on ths data
    // This is where we do all of the checks on the incomming serial command:

    //Serial.println("ID OK");  // TEST - got into this routine

    // Change device ID:
    // Device ID
    // “aXXCHIDXXE--“
    // Where the last two values (XX) are the new device ID (from AA to ZZ).
    if (str_buffer.substring(3, 7) == "CHID")
    {
      // First check if the NEW device ID is within the allowable range (AA-ZZ)
      // to do this we can convert to an int and check if it is within the OK levels
      // A -> int is 65, Z -. int is 90
      // So our new device ID as an int must be between 65 and 90 for it to be valid
      if (65 <= int(str_buffer[7]) && int(str_buffer[7]) <= 90 && 65 <= int(str_buffer[8]) && int(str_buffer[8]) <= 90)
      { // If is all OK then write the data
        // Change device ID
        Serial.print("CHID: ");
        Serial.println(str_buffer.substring(7, 9)); // This will change the device ID
        deviceID[0] = str_buffer[7];
        deviceID[1] = str_buffer[8];
        // Also want to store this into EEPROM
        EEPROM.write(0, deviceID[0]);    // Do this seperately
        EEPROM.write(1, deviceID[1]);
      }
      else
      {
        Serial.println("Invalid ID");
      }
    }
  }


  //  Power Value
  //  “aXXP????-“
  //  Where the value ???? is the power in Watts
  if (str_buffer.substring(3, 4) == "P")
  {
    // Update the power value if its valid
    powerAveStr = str_buffer.substring(4, 8);

    if (checkStringOK(powerAveStr) == true)
    {
      powerAve = powerAveStr.toInt();
      displayMode = 0;  // Display mode 0: Power 1: Time
    }
    else
    {
      Serial.println("Invalid P");
    }
  }

  //  Maximum Graph Power
  //  To change this we use the command:
  //  “aXXMGPDDDD--“
  //  Where the value DDDD is the maximum graph power in watts
  // This corresponds to 32 red LEDs being lit.
  else if (str_buffer.substring(3, 6) == "MGP")
  {
    maxGraphPowerStr = str_buffer.substring(6, 10);
    maxGraphPower = maxGraphPowerStr.toInt();
    // DO NOT CHANGE if outside of max/min values or rubbish data
    if (maxGraphPower >= 0 && maxGraphPower <= 3000)
    {
      // Change max Graph Power
      Serial.print("MGP: ");
      Serial.println(maxGraphPower);

      // Also want to store this into EEPOM
      EEPROM.write(9, maxGraphPower >> 8);    // Do this seperately
      EEPROM.write(10, maxGraphPower & 0xff);
    }
    else
    {
      Serial.println("Invalid MGP");
    }
  }
}
