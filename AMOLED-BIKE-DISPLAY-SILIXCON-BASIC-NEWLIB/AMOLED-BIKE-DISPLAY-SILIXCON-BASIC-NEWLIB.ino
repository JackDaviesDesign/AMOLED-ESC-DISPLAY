//#include <lvgl.h>

#include <ESP32-TWAI-CAN.hpp>  // https://github.com/handmade0octopus/ESP32-TWAI-CAN

#include "rm67162.h"
#include <TFT_eSPI.h>

#include "OrbitronBlack130.h"
#include "OrbitronBlack35.h"
#include "OrbitronBlack30.h"
#include "OrbitronBlack15.h"

// TaskHandle_t Task1;
// TaskHandle_t Task2;

// interval timer
unsigned long previousMillisrecieveCANData = 0;  // will store last time LED was updated
const long intervalrecieveCANData = 100;  // interval at which to run "recieveCANData" function

unsigned long previousMillisDraw = 0;  // will store last time LED was updated
const long intervalDraw = 100;  // interval at which to run "draw" functio

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

#define lineColor 0x0B91 // Color of the grid lines
#define lineAccColor 0x7E7C // Color of the grid line corner accents
#define speedTextColor 0xFFFF // Color of the grid line corner accents
#define boxBGColor 0xFFFF // Color of the grid line corner accents


// Change CAN_SPEED & Tx,Rx pins in setup below to match hardware. Silixcon default CAN speed is 1000KBPS / 1MBPS
#define CAN_TX		15
#define CAN_RX		14
#define CAN_SPEED		1000 // 1000 is 1MBPS

CanFrame rxFrame;


#define HALLSENSORPIN		40 


const uint8_t cellSeriesCount = 21; // Number of batt cells in series. Used to calc V per cell.
uint16_t SOCAngle; // Value used to drive the ring guage
uint16_t SOCGuageColor; // Colour change of guage
uint16_t powerGuage; // used for the power meter slider
uint16_t powerGuageColor;
bool canConnected;

// Incoming CAN Values from Silixcon

// lynxStatus
uint8_t currentPowerMap;
uint8_t driverStatusWord;
uint16_t driverLimitWord;
uint16_t driverErrorWord;

// lynxMotorStatus
int16_t motorCurrent; // Motor current [A] (q axis)  - can be neg during regen so int16_t
int16_t motorRpm; // Motor speed [rpm] - can be neg during reverse so int16_t
int16_t vehicleSpeed; // Vehicle speeed [km/h] - can be neg during reverse so int16_t
int16_t motorWatts; // Motor power [W] - can be neg during regen so int16_t

// lynxBatteryStatus
uint8_t mapType;  // Map type: 1 - Normal   2 - Restricted   3 - reverse   4 - boost   5 - Reserve map
uint8_t SOC; // SOC [%]
float battVoltage; // Battery voltage [0.01 V]
float battCurrent; // Battery current [0.02 A] - can be neg during regen

// ODO_trip
float TRIP; // TRIP [0.01 km ], distance counter. Can by reset from display or CAN command.
float ODO; // ODO [0.1 km], total distance counter

// relativeValues   -   Mostly used by the display for bargraphs
int16_t relMotorPhaseCurrent; // Relative motor phase current -32767 - 32767. (32767 = iref)
uint16_t relDriverTotLimit; // Driver total limit. 32767 = Full power  0 = Zero power (100% limitation)
int16_t relSpeed; // Relative speed (-323767 - 32767). Only works if parameter /maps/maxkph is set. The parameter is the full value.

// temps
uint16_t motorRThermistor; // Driver /driver/motor/RThermistor  Disconnected sensor = 0xFFFF
uint16_t ptcTemp; // Driver /driver/ptctemp
uint8_t drivetTemp; // Driver /drivet/temp

// Incoming CAN Values from Silixcon


// CAN IDs to filter - https://docs.silixcon.com/docs/fw/apps/esc/lynx/can/periodic_messages
// const uint32_t lynxStatus = 0x600; // Sent every 100ms
// const uint32_t lynxMotorStatus = 0x610; // Sent every 100ms
// const uint32_t lynxBatteryStatus = 0x618; // Sent every 100ms
// const uint32_t ODO_trip = 0x620; // Sent every 1s
// const uint32_t rangeEstimator = 0x625; // Sent every 800ms - Range estimator feature is not finished yet. It is not recommended to use it.
// const uint32_t relativeValues = 0x626; // Sent every 250ms
// const uint32_t temps = 0x628; // Sent every 800ms

void setup()
{

    if(ESP32Can.begin(ESP32Can.convertSpeed(CAN_SPEED), CAN_TX, CAN_RX, 10, 10)) { 
        Serial.println("CAN bus started!");
    } else {
        Serial.println("CAN bus failed!");
    }


  pinMode(38, OUTPUT); // enables the AMOLED screen
  digitalWrite(38, HIGH);
  
  pinMode(HALLSENSORPIN, OUTPUT); // HALL sensor input


  sprite.createSprite(240,536);
  sprite.setSwapBytes(1);

  /** Setup UART port  */
  Serial.begin(115200);
  
  rm67162_init();  // amoled lcd initialization
  //lcd_brightness(200);
  
  // //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  // xTaskCreatePinnedToCore(
  //                   recieveCANData,   /* Task function. */
  //                   "recieveCANData",     /* name of task. */
  //                   10000,       /* Stack size of task */
  //                   NULL,        /* parameter of the task */
  //                   1,           /* priority of the task */
  //                   &Task1,      /* Task handle to keep track of created task */
  //                   1);          /* pin task to core 0 */                  
  // delay(500);

  //   //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  // xTaskCreatePinnedToCore(
  //                   draw,   /* Task function. */
  //                   "recieveCANData",     /* name of task. */
  //                   10000,       /* Stack size of task */
  //                   NULL,        /* parameter of the task */
  //                   1,           /* priority of the task */
  //                   &Task2,      /* Task handle to keep track of created task */
  //                   0);          /* pin task to core 1 */
  //   delay(500);

}

void recieveCANData(){
      // You can set custom timeout, default is 1000
    if(ESP32Can.readFrame(rxFrame, 1000)) {
       // Comment out if too many frames
       //Serial.printf("Received frame: %03X  \r\n", rxFrame.identifier);
      
      canConnected = true; // conection indicator

 // ---------- lynxStatus - 0x600 ----------

        if(rxFrame.identifier == 0x600) { 
          currentPowerMap = rxFrame.data[2]; // 8-bit integer
          driverStatusWord = rxFrame.data[3]; // 8-bit integer
          driverLimitWord = rxFrame.data[4] | (rxFrame.data[5] << 8); // 16-bit integer
          driverErrorWord = rxFrame.data[6] | (rxFrame.data[7] << 8); // 16-bit integer   
        }

 // ---------- lynxMotorStatus - 0x610 ----------

        if(rxFrame.identifier == 0x610) {   // Standard OBD2 frame responce ID
          motorCurrent = rxFrame.data[0] | (rxFrame.data[1] << 8); // 16-bit integer
          motorRpm = rxFrame.data[2] | (rxFrame.data[3] << 8); // 16-bit integer
          vehicleSpeed = rxFrame.data[4] | (rxFrame.data[5] << 8); // 16-bit integer
          motorWatts = rxFrame.data[6] | (rxFrame.data[7] << 8); // 16-bit integer  
          vehicleSpeed = vehicleSpeed/100;
          //Serial.print("Vehicle Speed: ");
          //Serial.println(vehicleSpeed);
        }

  // ---------- lynxBatteryStatus - 0x618 ----------

        if(rxFrame.identifier == 0x618) { 
          mapType = rxFrame.data[1]; // 8-bit integer // SOC %
          SOC = rxFrame.data[2]; // 8-bit integer // SOH %
          battVoltage = rxFrame.data[4] | (rxFrame.data[5] << 8); // 16-bit integer // batt voltage
          battVoltage = battVoltage/100;
          battCurrent = rxFrame.data[6] | (rxFrame.data[7] << 8); // 16-bit integer // batt current        
          battCurrent = battCurrent/100; 
        }

  // ---------- ODO_trip - 0x620 ----------

        if(rxFrame.identifier == 0x620) { 
// 32-bit float - TRIP
        union {
          uint8_t b[4];
          float f;
        } TRIPf;
        TRIPf.b[0] = rxFrame.data[0];
        TRIPf.b[1] = rxFrame.data[1];
        TRIPf.b[2] = rxFrame.data[2];
        TRIPf.b[3] = rxFrame.data[3];

        TRIP = TRIPf.f; // assign the 4 bytes to a float


        // 32-bit float - ODO
        union {
          uint8_t b[4];
          float f;
        } ODOf;
        ODOf.b[0] = rxFrame.data[4];
        ODOf.b[1] = rxFrame.data[5];
        ODOf.b[2] = rxFrame.data[6];
        ODOf.b[3] = rxFrame.data[7];

        ODO = ODOf.f; // assign the 4 bytes to a float
        }


        // ---------- relativeValues - 0x626 ----------

        if(rxFrame.identifier == 0x626) { 
        // Extracting values considering little-endian format
        relMotorPhaseCurrent = rxFrame.data[0] | (rxFrame.data[1] << 8); // 16-bit integer // batt voltage
        relDriverTotLimit = rxFrame.data[2] | (rxFrame.data[3] << 8); // 16-bit integer // batt current  
        relSpeed = rxFrame.data[4] | (rxFrame.data[5] << 8); // 16-bit integer // batt voltage
      
        }

    }
    else {
      canConnected = false; // conection indicator
    }
}


// void recieveCANDataOLD(){

//   unsigned long currentMillis = millis();
//   if (currentMillis - previousMillisrecieveCANData >= intervalrecieveCANData) {
//     previousMillisrecieveCANData = currentMillis;



//   // Check for received CAN messages
//   twai_message_t rx_frame;

//   if (ESP32CAN_OK == ESP32Can.CANReadFrame(&rx_frame)) {  /* only print when CAN message is received*/
    
//   canConnected = true;
    
//     // // ---------- lynxStatus - 0x600 ----------

//     // if (rx_frame.identifier == lynxStatus) { // Check if the message ID matches the filter ID
//     //   Serial.print("Filtered CAN message received, ID: 0x");
//     //   Serial.println(rx_frame.identifier, HEX);

//     //   // Read the incoming data
//     //   uint8_t data[8];
//     //   int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
//     //   for (int i = 0; i < dataLength; i++) {
//     //     data[i] = rx_frame.data[i];
//     //   }

//     //   if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
//     //     // Extracting values considering little-endian format
//     //     currentPowerMap = data[2]; // 8-bit integer
//     //     driverStatusWord = data[3]; // 8-bit integer
//     //     driverLimitWord = data[4] | (data[5] << 8); // 16-bit integer
//     //     driverErrorWord = data[6] | (data[7] << 8); // 16-bit integer       

//     //     // Print the extracted values
//     //     Serial.print("Current Power Map: ");
//     //     Serial.println(currentPowerMap);
//     //     Serial.print("Driver Status Word: ");
//     //     Serial.println(driverStatusWord);
//     //     Serial.print("Driver Limit Word: ");
//     //     Serial.println(driverLimitWord);
//     //     Serial.print("Driver Error Word: ");
//     //     Serial.println(driverErrorWord);

//     //   } else {
//     //     Serial.println("Received data length is less than expected.");
//     //   }
//     // }





// // ---------- lynxMotorStatus - 0x610 ----------

//     if (rx_frame.identifier == lynxMotorStatus) { // Check if the message ID matches the filter ID
//       // Serial.print("Filtered CAN message received, ID: 0x");
//       // Serial.println(rx_frame.identifier, HEX);

//       // Read the incoming data
//       uint8_t data[8];
//       int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
//       for (int i = 0; i < dataLength; i++) {
//         data[i] = rx_frame.data[i];
//       }

//       if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
//         // Extracting values considering little-endian format
//         motorCurrent = data[0] | (data[1] << 8); // 16-bit integer
//         motorRpm = data[2] | (data[3] << 8); // 16-bit integer
//         vehicleSpeed = data[4] | (data[5] << 8); // 16-bit integer
//         motorWatts = data[6] | (data[7] << 8); // 16-bit integer     

//         vehicleSpeed = vehicleSpeed/100;
//         // // Print the extracted values
//         // Serial.print("Motor Current: ");
//         // Serial.println(motorCurrent);
//         // Serial.print("Motor RPM: ");
//         // Serial.println(motorRpm);
//         Serial.print("Vehicle Speed: ");
//         Serial.println(vehicleSpeed);
//         // Serial.print("Motor Watts: ");
//         // Serial.println(motorWatts);

//       } else {
//         //Serial.println("Received data length is less than expected.");
//       }
//     }







//     // ---------- lynxBatteryStatus - 0x618 ----------

//     if (rx_frame.identifier == lynxBatteryStatus) { // Check if the message ID matches the filter ID
//       // Serial.print("Filtered CAN message received, ID: 0x");
//       // Serial.println(rx_frame.identifier, HEX);

//       // Read the incoming data
//       uint8_t data[8];
//       int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
//       for (int i = 0; i < dataLength; i++) {
//         data[i] = rx_frame.data[i];
//       }

//       if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
//         // Extracting values considering little-endian format
//         mapType = data[1]; // 8-bit integer // SOC %
//         SOC = data[2]; // 8-bit integer // SOH %
//         battVoltage = data[4] | (data[5] << 8); // 16-bit integer // batt voltage
//         battVoltage = battVoltage/100;
//         battCurrent = data[6] | (data[7] << 8); // 16-bit integer // batt current        
//         battCurrent = battCurrent/100;

//         // // Print the extracted values
//         // Serial.print("Map Type : ");
//         // Serial.println(mapType);
//         // Serial.print("SOC: ");
//         // Serial.println(SOC);
//         // Serial.print("Batt Voltage: ");
//         // Serial.println(battVoltage);
//         // Serial.print("Cell Voltage: ");
//         // Serial.println((battVoltage/100)/cellSeriesCount); // V per cell.
//         // Serial.print("Batt Current: ");
//         // Serial.println(battCurrent);

//       } else {
//         //Serial.println("Received data length is less than expected.");
//       }
//     }





//     // ---------- ODO_trip - 0x620 ----------

// if (rx_frame.identifier == ODO_trip) { // Check if the message ID matches the filter ID
//       // Serial.print("Filtered CAN message received, ID: 0x");
//       // Serial.println(rx_frame.identifier, HEX);

//       // Read the incoming data
//       uint8_t data[8];
//       int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
//       for (int i = 0; i < dataLength; i++) {
//         data[i] = rx_frame.data[i];
//       }

//       if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
//         // Extracting values considering little-endian format      

//         // 32-bit float - TRIP
//         union {
//           uint8_t b[4];
//           float f;
//         } TRIPf;
//         TRIPf.b[0] = data[0];
//         TRIPf.b[1] = data[1];
//         TRIPf.b[2] = data[2];
//         TRIPf.b[3] = data[3];

//         TRIP = TRIPf.f; // assign the 4 bytes to a float


//         // 32-bit float - ODO
//         union {
//           uint8_t b[4];
//           float f;
//         } ODOf;
//         ODOf.b[0] = data[4];
//         ODOf.b[1] = data[5];
//         ODOf.b[2] = data[6];
//         ODOf.b[3] = data[7];

//         ODO = ODOf.f; // assign the 4 bytes to a float


//         // Serial.print("TRIP : ");
//         // Serial.println(TRIP);

//         // Serial.print("ODO : ");
//         // Serial.println(ODO);


//       } else {
//         //Serial.println("Received data length is less than expected.");
//       }
//     }


// // ---------- relativeValues - 0x626 ----------

//     if (rx_frame.identifier == relativeValues) { // Check if the message ID matches the filter ID
//       // Serial.print("Filtered CAN message received, ID: 0x");
//       // Serial.println(rx_frame.identifier, HEX);

//       // Read the incoming data
//       uint8_t data[8];
//       int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
//       for (int i = 0; i < dataLength; i++) {
//         data[i] = rx_frame.data[i];
//       }

//       if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
//         // Extracting values considering little-endian format
//         relMotorPhaseCurrent = data[0] | (data[1] << 8); // 16-bit integer // batt voltage
//         relDriverTotLimit = data[2] | (data[3] << 8); // 16-bit integer // batt current  
//         relSpeed = data[4] | (data[5] << 8); // 16-bit integer // batt voltage
      

//         // // Print the extracted values
//         // Serial.print("Relative Motor Phase Current : ");
//         // Serial.println(relMotorPhaseCurrent);
//         // Serial.print("Relative Driver Total Limit: ");
//         // Serial.println(relDriverTotLimit);
//         // Serial.print("Relative Speed: ");
//         // Serial.println(relSpeed);

//       } else {
//         //Serial.println("Received data length is less than expected.");
//       }
//     }




//     // // ---------- temps - 0x628 ----------

//     // if (rx_frame.identifier == temps) { // Check if the message ID matches the filter ID
//     //   Serial.print("Filtered CAN message received, ID: 0x");
//     //   Serial.println(rx_frame.identifier, HEX);

//     //   // Read the incoming data
//     //   uint8_t data[8];
//     //   int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
//     //   for (int i = 0; i < dataLength; i++) {
//     //     data[i] = rx_frame.data[i];
//     //   }

//     //   if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
//     //     // Extracting values considering little-endian format

//     //     motorRThermistor = data[0] | (data[1] << 8); // 16-bit integer // batt voltage
//     //     ptcTemp = data[2] | (data[3] << 8); // 16-bit integer // batt current        
//     //     drivetTemp = data[4]; // 8-bit integer // SOC %


//     //     // Print the extracted values
//     //     Serial.print("R Thermistor : ");
//     //     Serial.println(motorRThermistor);
//     //     Serial.print("PTC Temp: ");
//     //     Serial.println(ptcTemp);
//     //     Serial.print("Drive T Temp: ");
//     //     Serial.println(drivetTemp);

//     //   } else {
//     //     Serial.println("Received data length is less than expected.");
//     //   }
//     // }



//   }

//   else {
//     canConnected = false; // used to show connection status 
//   }
// }
// }



void draw()
 {

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisDraw >= intervalDraw) {
    previousMillisDraw = currentMillis;


// Text datum examples
// #define TL_DATUM 0  // Top left (default)
// #define TC_DATUM 1  // Top centre
// #define TR_DATUM 2  // Top right
// #define ML_DATUM 3  // Middle left
// #define CL_DATUM 3  // Centre left, same as above
// #define MC_DATUM 4  // Middle centre
// #define CC_DATUM 4  // Centre centre, same as above
// #define MR_DATUM 5  // Middle right
// #define CR_DATUM 5  // Centre right, same as above
// #define BL_DATUM 6  // Bottom left
// #define BC_DATUM 7  // Bottom centre
// #define BR_DATUM 8  // Bottom right
  
  sprite.fillSprite(TFT_BLACK);

  sprite.setTextDatum(4);
  sprite.setTextColor(speedTextColor);
  sprite.loadFont(OrbitronBlack130);
  //sprite.drawString(String("12"),120,95);
  sprite.drawNumber(vehicleSpeed,120,95);

  sprite.setTextDatum(2);
  sprite.setTextColor(speedTextColor);
  sprite.loadFont(OrbitronBlack15);
  sprite.drawString("KPH",229,10);

  // connection status
  sprite.setTextDatum(0);
  if (canConnected == true) {
    sprite.drawString("CON",10,10);
  }
  // else {
  //   sprite.drawString("NOT CONNECTED",10,10);
  // }



  // SOC ring

  SOCAngle = map(SOC, 0, 100, 0, 360); // remap SOC value to use in the ring slider guage

  if(SOC == 0) {
    SOCGuageColor = 0xf968; // turn ring red if SOC is 0
  }
  else if (SOC < 25) {
    SOCGuageColor = 0xfbc8; // turn ring orange if SOC is less than 25%
  }
  else {
    SOCGuageColor = 0x7E7C; // normal ring color
  }

  //bg ring
  sprite.drawSmoothArc(62/* x */, 258/* y */, 46/* r */, 42/* internal r */, 0/* start ang */, 360/* end ang */, 0xE71C/* fg color */, 0x0000/* bg color */, 1/* round ends 1 no 0 */);
  // front ring
  sprite.drawSmoothArc(62/* x */, 258/* y */, 50/* r */, 40/* internal r */, 0/* start ang */, SOCAngle/* end ang */, SOCGuageColor/* fg color */, 0x0000/* bg color */, 1/* round ends 1 no 0 */);
  


  //SIMULATE VALUES TESTING

  // vehicleSpeed++;

  //   if(vehicleSpeed > 99) {
  //   vehicleSpeed = 0;
  // }

  //   if(SOC == 0) {
  //   SOC = SOC+100;
  // }
  // SOC--;

  // relMotorPhaseCurrent = relMotorPhaseCurrent+500;

  // if(relMotorPhaseCurrent > 32767) {
  //   relMotorPhaseCurrent = -32767;
  // }



  // power guage

  if(relMotorPhaseCurrent > 0) {
    powerGuage = map(relMotorPhaseCurrent, 0, 32767, 0, 220); // remap SOC value to use in the power slider guage
    powerGuageColor = 0xffff; // normal color
    
  }
  if (relMotorPhaseCurrent > 24500) { // about 75% power turn to orange
    powerGuageColor = 0xfbc8;
  }
  else if (relMotorPhaseCurrent < 0) { // negative value is regen
    powerGuage = map(relMotorPhaseCurrent, 0, -32767, 0, 220); // remap SOC value to use in the power slider guage
    powerGuageColor = 0x0720; // regen turn green 
  }

  sprite.fillRect(14, 152, powerGuage, 36, powerGuageColor); // main bar

  // sprite.drawNumber(powerGuage,62,450);
  // sprite.drawNumber(relMotorPhaseCurrent,62,470);


  // // vertical thiner
  // sprite.fillRect(20, 150, 8, 40, TFT_BLACK);
  // sprite.fillRect(36, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(52, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(68, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(84, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(100, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(116, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(132, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(148, 150, 8, 40, TFT_BLACK);  
  // sprite.fillRect(164, 150, 8, 40, TFT_BLACK);
  // sprite.fillRect(180, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(196, 150, 8, 40, TFT_BLACK); 
  // sprite.fillRect(212, 150, 8, 40, TFT_BLACK);  


  // vertical wider
  sprite.fillRect(26, 150, 8, 40, TFT_BLACK);
  sprite.fillRect(48, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(70, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(92, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(114, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(136, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(158, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(180, 150, 8, 40, TFT_BLACK); 
  sprite.fillRect(202, 150, 8, 40, TFT_BLACK);  
  sprite.fillRect(224, 150, 8, 40, TFT_BLACK);


  // blackout bars to create grid
  // horizontal
  sprite.fillRect(12, 165, 212, 10, TFT_BLACK); 

  // //////////.  OLD

  // // blackout bars to create grid
  // // horizontal
  // sprite.fillRect(10, 156, 220, 4, TFT_BLACK); 
  // sprite.fillRect(10, 168, 220, 4, TFT_BLACK);
  // sprite.fillRect(10, 180, 220, 4, TFT_BLACK);
  
  // // vertical
  // sprite.fillRect(18, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(30, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(42, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(54, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(66, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(78, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(90, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(102, 148, 4, 44, TFT_BLACK); 
  // sprite.fillRect(114, 148, 4, 44, TFT_BLACK);  
  // sprite.fillRect(126, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(138, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(150, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(162, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(174, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(186, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(198, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(210, 148, 4, 44, TFT_BLACK);
  // sprite.fillRect(222, 148, 4, 44, TFT_BLACK);  







  sprite.setTextDatum(4);
  sprite.setTextColor(0xFFFF);
  sprite.loadFont(OrbitronBlack35);
  //sprite.drawNumber(int(round(SOC*100)), 62, 260);
  sprite.drawNumber(SOC, 62, 255);
  //sprite.drawNumber(SOC*100, 60, 350);
  //sprite.drawNumber(SOCAngle, 60, 450); 

  // draw values
  // drawFloat(value, precision, x, y, font);
   sprite.drawNumber(motorWatts,178,255);
   sprite.drawNumber(battCurrent,62,370);
   sprite.drawFloat(battVoltage/cellSeriesCount,2,178,370);

   sprite.drawFloat(TRIP,2,60,480);
   sprite.drawFloat(ODO,2,178,480);


  
  // label for SOC
  sprite.loadFont(OrbitronBlack15);
  sprite.drawString(String("SOC"), 62, 280);

  // labels
   sprite.drawString("MOTOR-W", 178, 280);
   sprite.drawString("BATT-A", 60, 395);
   sprite.drawString("CELL-V", 178, 395);

   sprite.drawString("TRIP", 60, 505);
   sprite.drawString("ODO", 178, 505);














  // // DEBUG VALUES DISPLAY

  // // lynxStatus
  // sprite.setTextDatum(5);
  // sprite.drawString("PowerMap - ",150,150);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(currentPowerMap,160,150);
  
  // sprite.setTextDatum(5);
  // sprite.drawString("StatusWord - ",150,170);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(driverStatusWord,160,170);
  
  // sprite.setTextDatum(5);
  // sprite.drawString("LimitWord - ",150,190);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(driverLimitWord,160,190);

  // sprite.setTextDatum(5);
  // sprite.drawString("ErrorWord - ",150,210);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(driverErrorWord,160,210);

  // // lynxMotorStatus
  // sprite.setTextDatum(5);
  // sprite.drawString("Motor A - ",150,230);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(motorCurrent,160,230);

  // sprite.setTextDatum(5);
  // sprite.drawString("Motor RPM - ",150,250);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(motorRpm,160,250);

  // sprite.setTextDatum(5);
  // sprite.drawString("Speed KPH - ",150,270);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(vehicleSpeed,160,270);
  
  // sprite.setTextDatum(5);
  // sprite.drawString("Motor W - ",150,290);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(motorWatts,160,290);  

  // // lynxBatteryStatus
  // sprite.setTextDatum(5);
  // sprite.drawString("Map Type - ",150,310);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(mapType,160,310);

  // sprite.setTextDatum(5);
  // sprite.drawString("SOC % - ",150,330);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(SOC,160,330);

  // sprite.setTextDatum(5);
  // sprite.drawString("Batt V - ",150,350);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(battVoltage,160,350);
  
  // sprite.setTextDatum(5);
  // sprite.drawString("Batt A - ",150,370);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(battCurrent,160,370); 

  // // ODO_trip
  // sprite.setTextDatum(5);
  // sprite.drawString("Trip - ",150,390);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(TRIP,160,390);

  // sprite.setTextDatum(5);
  // sprite.drawString("ODO - ",150,410);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(ODO,160,410);
 
  // // relativeValues
  // sprite.setTextDatum(5);
  // sprite.drawString("M Phase A - ",150,430);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(relMotorPhaseCurrent,160,430);

  // sprite.setTextDatum(5);
  // sprite.drawString("Tot Limit - ",150,450);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(relDriverTotLimit,160,450);

  // sprite.setTextDatum(5);
  // sprite.drawString("Rel Speed - ",150,470);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(relSpeed,160,470);

  // // relativeValues
  // sprite.setTextDatum(5);
  // sprite.drawString("M R Therm - ",150,490);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(motorRThermistor,160,490);

  // sprite.setTextDatum(5);
  // sprite.drawString("PTC Temp - ",150,510);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(ptcTemp,160,510);

  // sprite.setTextDatum(5);
  // sprite.drawString("Driver Temp - ",150,530);
  // sprite.setTextDatum(3);
  // sprite.drawNumber(drivetTemp,160,530);



  // draw the grid
  // outline
  sprite.drawLine(4, 4, 236, 4, lineColor);
  sprite.drawLine(4, 4, 4, 532, lineColor);
  sprite.drawLine(4, 532, 236, 532, lineColor);
  sprite.drawLine(236, 532, 236, 4, lineColor);

  // horizontal lines
  sprite.drawLine(4, 142, 236, 142, lineColor);
  sprite.drawLine(4, 198, 236, 198, lineColor);
  sprite.drawLine(4, 314, 236, 314, lineColor);
  sprite.drawLine(4, 430, 236, 430, lineColor);

  // vertical lines
  sprite.drawLine(120, 198, 120, 532, lineColor);

  // draw the accent corners
  sprite.fillRect(2, 2, 4, 8, lineAccColor);
  sprite.fillRect(2, 2, 8, 4, lineAccColor);

  sprite.fillRect(230, 2, 8, 4, lineAccColor);
  sprite.fillRect(234, 2, 4, 8, lineAccColor);

  sprite.fillRect(2, 526, 4, 8, lineAccColor);
  sprite.fillRect(2, 530, 8, 4, lineAccColor);  

  sprite.fillRect(234, 526, 4, 8, lineAccColor);
  sprite.fillRect(230, 530, 8, 4, lineAccColor);

  //LHS
  sprite.fillRect(2, 136, 4, 12, lineAccColor); // top of power bar
  sprite.fillRect(2, 140, 8, 4, lineAccColor);

  sprite.fillRect(2, 192, 4, 12, lineAccColor);
  sprite.fillRect(2, 196, 8, 4, lineAccColor);

  sprite.fillRect(2, 308, 4, 12, lineAccColor);
  sprite.fillRect(2, 312, 8, 4, lineAccColor);

  sprite.fillRect(2, 424, 4, 12, lineAccColor);
  sprite.fillRect(2, 428, 8, 4, lineAccColor);

  //MID
  sprite.fillRect(114, 195, 12, 4, lineAccColor);
  sprite.fillRect(118, 195, 4, 8, lineAccColor);

  sprite.fillRect(114, 312, 12, 4, lineAccColor);
  sprite.fillRect(118, 308, 4, 12, lineAccColor);

  sprite.fillRect(114, 428, 12, 4, lineAccColor);
  sprite.fillRect(118, 424, 4, 8, lineAccColor);

  //RHS
  sprite.fillRect(234, 136, 4, 12, lineAccColor); // top of power bar
  sprite.fillRect(230, 140, 8, 4, lineAccColor);

  sprite.fillRect(234, 192, 4, 12, lineAccColor);
  sprite.fillRect(230, 196, 8, 4, lineAccColor);

  sprite.fillRect(234, 308, 4, 12, lineAccColor);
  sprite.fillRect(230, 312, 8, 4, lineAccColor);

  sprite.fillRect(234, 424, 4, 12, lineAccColor);
  sprite.fillRect(230, 428, 8, 4, lineAccColor);
  // end grid

  //sprite.loadFont(middleFont);
  //sprite.setTextColor(0x0B91);

  lcd_PushColors(0, 0, 240,536, (uint16_t*)sprite.getPointer());
 }
 }

 void hallSensor() { // hall sensor input used for estop or speed reduction 
  
    Serial.print("Hall Sensor Value : ");
    Serial.println(analogRead(HALLSENSORPIN));

//   if(analogRead(HALLSENSORPIN)<2000); // magnet is not present
//  {
//     // write canbus change map to slow
//   }

 }





void loop() {

  recieveCANData();
  draw();
  hallSensor();
   
}