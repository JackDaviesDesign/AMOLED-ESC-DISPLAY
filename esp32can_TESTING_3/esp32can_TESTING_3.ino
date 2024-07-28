#include <ESP32CAN.h> // https://github.com/sdp8483/ESP32-Arduino-CAN
//using updated library to work with ESP32-S3

// CAN configuration
// CAN_device_t CAN_cfg; // CAN Config
// const int rx_queue_size = 10;

// Change CAN_SPEED & Tx,Rx pins in setup below to match hardware. Silixcon default CAN speed is 1000KBPS / 1MBPS

const uint8_t cellSeriesCount = 21; // Number of batt cells in series. Used to calc V per cell.

// Incoming CAN Values from Silixcon

// lynxStatus
uint8_t currentPowerMap;
uint8_t driverStatusWord;
uint16_t driverLimitWord;
uint16_t driverErrorWord;

// lynxMotorStatus
uint16_t motorCurrent; // Motor current [A] (q axis)
uint16_t motorRpm; // Motor speed [rpm]
uint16_t vehicleSpeed; // Vehicle speeed [km/h]
uint16_t motorWatts; // Motor power [W]

// lynxBatteryStatus
uint8_t mapType;  // Map type: 1 - Normal   2 - Restricted   3 - reverse   4 - boost   5 - Reserve map
uint8_t SOC; // SOC [%]
float battVoltage; // Battery voltage [0.01 V]
uint16_t battCurrent; // Battery current [0.02 A]

// ODO_trip
float TRIP; // TRIP [0.01 km ], distance counter. Can by reset from display or CAN command.
float ODO; // ODO [0.1 km], total distance counter

// relativeValues   -   Mostly used by the display for bargrafs
uint16_t relMotorPhaseCurrent; // Relative motor phase current -32767 - 32767. (32767 = iref)
uint16_t relDriverTotLimit; // Driver total limit. 32767 = Full power  0 = Zero power (100% limitation)
uint16_t relSpeed; // Relative speed (-323767 - 32767). Only works if parameter /maps/maxkph is set. The parameter is the full value.

// temps
uint16_t motorRThermistor; // Driver /driver/motor/RThermistor  Disconnected sensor = 0xFFFF
uint16_t ptcTemp; // Driver /driver/ptctemp
uint8_t drivetTemp; // Driver /drivet/temp

// Incoming CAN Values from Silixcon


// CAN IDs to filter - https://docs.silixcon.com/docs/fw/apps/esc/lynx/can/periodic_messages
const uint32_t lynxStatus = 0x600; // Sent every 100ms
const uint32_t lynxMotorStatus = 0x610; // Sent every 100ms
const uint32_t lynxBatteryStatus = 0x618; // Sent every 100ms
const uint32_t ODO_trip = 0x620; // Sent every 1s
const uint32_t rangeEstimator = 0x625; // Sent every 800ms - Range estimator feature is not finished yet. It is not recommended to use it.
const uint32_t relativeValues = 0x626; // Sent every 250ms
const uint32_t temps = 0x628; // Sent every 800ms


void setup() {
  Serial.begin(115200);

  ESP32Can.CANInit(GPIO_NUM_15, GPIO_NUM_14, ESP32CAN_SPEED_1MBPS); // TX PIN , RX PIN , BAUD RATE

}

void loop() {

recieveCANData();

delay(20);

}






void recieveCANData(){

  // Check for received CAN messages
  twai_message_t rx_frame;

  if (ESP32CAN_OK == ESP32Can.CANReadFrame(&rx_frame)) {  /* only print when CAN message is received*/
    
    
    // ---------- lynxStatus - 0x600 ----------

    if (rx_frame.identifier == lynxStatus) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.identifier, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data[i];
      }

      if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
        // Extracting values considering little-endian format
        currentPowerMap = data[2]; // 8-bit integer
        driverStatusWord = data[3]; // 8-bit integer
        driverLimitWord = data[4] | (data[5] << 8); // 16-bit integer
        driverErrorWord = data[6] | (data[7] << 8); // 16-bit integer       

        // Print the extracted values
        Serial.print("Current Power Map: ");
        Serial.println(currentPowerMap);
        Serial.print("Driver Status Word: ");
        Serial.println(driverStatusWord);
        Serial.print("Driver Limit Word: ");
        Serial.println(driverLimitWord);
        Serial.print("Driver Error Word: ");
        Serial.println(driverErrorWord);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }





// ---------- lynxMotorStatus - 0x610 ----------

    if (rx_frame.identifier == lynxMotorStatus) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.identifier, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data[i];
      }

      if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
        // Extracting values considering little-endian format
        motorCurrent = data[0] | (data[1] << 8); // 16-bit integer
        motorRpm = data[2] | (data[3] << 8); // 16-bit integer
        vehicleSpeed = data[4] | (data[5] << 8); // 16-bit integer
        motorWatts = data[6] | (data[7] << 8); // 16-bit integer     

        // Print the extracted values
        Serial.print("Motor Current: ");
        Serial.println(motorCurrent);
        Serial.print("Motor RPM: ");
        Serial.println(motorRpm);
        Serial.print("Vehicle Speed: ");
        Serial.println(vehicleSpeed);
        Serial.print("Motor Watts: ");
        Serial.println(motorWatts);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }







    // ---------- lynxBatteryStatus - 0x618 ----------

    if (rx_frame.identifier == lynxBatteryStatus) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.identifier, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data[i];
      }

      if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
        // Extracting values considering little-endian format
        mapType = data[1]; // 8-bit integer // SOC %
        SOC = data[2]; // 8-bit integer // SOH %
        battVoltage = data[4] | (data[5] << 8); // 16-bit integer // batt voltage
        battCurrent = data[6] | (data[7] << 8); // 16-bit integer // batt current        

        // Print the extracted values
        Serial.print("Map Type : ");
        Serial.println(mapType);
        Serial.print("SOC: ");
        Serial.println(SOC);
        Serial.print("Batt Voltage: ");
        Serial.println(battVoltage/100);
        Serial.print("Cell Voltage: ");
        Serial.println((battVoltage/100)/cellSeriesCount); // V per cell.
        Serial.print("Batt Current: ");
        Serial.println(battCurrent/100);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }





    // ---------- ODO_trip - 0x620 ----------

if (rx_frame.identifier == ODO_trip) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.identifier, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data[i];
      }

      if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
        // Extracting values considering little-endian format      

        // 32-bit float - TRIP
        union {
          uint8_t b[4];
          float f;
        } TRIPf;
        TRIPf.b[0] = data[0];
        TRIPf.b[1] = data[1];
        TRIPf.b[2] = data[2];
        TRIPf.b[3] = data[3];

        TRIP = TRIPf.f; // assign the 4 bytes to a float


        // 32-bit float - ODO
        union {
          uint8_t b[4];
          float f;
        } ODOf;
        ODOf.b[0] = data[4];
        ODOf.b[1] = data[5];
        ODOf.b[2] = data[6];
        ODOf.b[3] = data[7];

        ODO = ODOf.f; // assign the 4 bytes to a float


        Serial.print("TRIP : ");
        Serial.println(TRIP);

        Serial.print("ODO : ");
        Serial.println(ODO);


      } else {
        Serial.println("Received data length is less than expected.");
      }
    }


// ---------- relativeValues - 0x626 ----------

    if (rx_frame.identifier == relativeValues) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.identifier, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data[i];
      }

      if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
        // Extracting values considering little-endian format
        relMotorPhaseCurrent = data[0] | (data[1] << 8); // 16-bit integer // batt voltage
        relDriverTotLimit = data[2] | (data[3] << 8); // 16-bit integer // batt current  
        relSpeed = data[4] | (data[5] << 8); // 16-bit integer // batt voltage
      

        // Print the extracted values
        Serial.print("Relative Motor Phase Current : ");
        Serial.println(relMotorPhaseCurrent);
        Serial.print("Relative Driver Total Limit: ");
        Serial.println(relDriverTotLimit);
        Serial.print("Relative Speed: ");
        Serial.println(relSpeed);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }




    // ---------- temps - 0x628 ----------

    if (rx_frame.identifier == temps) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.identifier, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.data_length_code; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data[i];
      }

      if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
        // Extracting values considering little-endian format

        motorRThermistor = data[0] | (data[1] << 8); // 16-bit integer // batt voltage
        ptcTemp = data[2] | (data[3] << 8); // 16-bit integer // batt current        
        drivetTemp = data[4]; // 8-bit integer // SOC %


        // Print the extracted values
        Serial.print("R Thermistor : ");
        Serial.println(motorRThermistor);
        Serial.print("PTC Temp: ");
        Serial.println(ptcTemp);
        Serial.print("Drive T Temp: ");
        Serial.println(drivetTemp);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }


  }
}

void draw() {  // print to display

  // display code here

}

