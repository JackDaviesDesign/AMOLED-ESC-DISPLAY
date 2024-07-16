#include <ESP32CAN.h>
#include <CAN_config.h>

// CAN configuration
CAN_device_t CAN_cfg; // CAN Config
const int rx_queue_size = 10;

// Change CAN_SPEED & Tx,Rx pins in setup below to match hardware. Silixcon default CAN speed is 1000KBPS

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
uint16_t battVoltage; // Battery voltage [0.01 V]
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

  // Set up CAN configuration
  CAN_cfg.speed = CAN_SPEED_1000KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_5; // TX pin
  CAN_cfg.rx_pin_id = GPIO_NUM_4; // RX pin
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));

  // Initialize CAN module
  ESP32Can.CANInit();


  // if (!ESP32Can.CANInit()) {
  //   Serial.println("CAN initialization failed!");
  //   while (1);
  // }

  // Serial.println("CAN initialized.");
}

void loop() {

recieveCANData();

}






void recieveCANData(){

  // Check for received CAN messages
  CAN_frame_t rx_frame;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    
    
    // ---------- lynxStatus - 0x600 ----------

    if (rx_frame.MsgID == lynxStatus) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.MsgID, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data.u8[i];
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

    if (rx_frame.MsgID == lynxMotorStatus) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.MsgID, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data.u8[i];
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

    if (rx_frame.MsgID == lynxBatteryStatus) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.MsgID, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data.u8[i];
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
        Serial.print("Batt Current: ");
        Serial.println(battCurrent/100);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }





    // ---------- ODO_trip - 0x620 ----------

if (rx_frame.MsgID == ODO_trip) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.MsgID, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data.u8[i];
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

    if (rx_frame.MsgID == relativeValues) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.MsgID, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data.u8[i];
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

    if (rx_frame.MsgID == temps) { // Check if the message ID matches the filter ID
      Serial.print("Filtered CAN message received, ID: 0x");
      Serial.println(rx_frame.MsgID, HEX);

      // Read the incoming data
      uint8_t data[8];
      int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
      for (int i = 0; i < dataLength; i++) {
        data[i] = rx_frame.data.u8[i];
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








  // // Check for received CAN messages
  // CAN_frame_t rx_frame;
  // if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
  //   Serial.print("New CAN message received, ID: 0x");
  //   Serial.println(rx_frame.MsgID, HEX);

  //   // Read the incoming data
  //   uint8_t data[8];
  //   int dataLength = rx_frame.FIR.B.DLC; // Get the number of bytes in the received message
  //   for (int i = 0; i < dataLength; i++) {
  //     data[i] = rx_frame.data.u8[i];
  //   }

  //   // Example: Assuming the message contains multiple values
  //   // Let's say the message contains:
  //   // - 2 bytes for value1 (16-bit integer)
  //   // - 1 byte for value2 (8-bit integer)
  //   // - 4 bytes for value3 (32-bit float)
  //   if (dataLength >= 7) { // Ensure there are at least 7 bytes of data
  //     // Extracting values considering little-endian format
  //     uint16_t value1 = data[0] | (data[1] << 8); // 16-bit integer
  //     uint8_t value2 = data[2]; // 8-bit integer

  //     // 32-bit float
  //     union {
  //       uint8_t b[4];
  //       float f;
  //     } value3;
  //     value3.b[0] = data[3];
  //     value3.b[1] = data[4];
  //     value3.b[2] = data[5];
  //     value3.b[3] = data[6];

  //     // Print the extracted values
  //     Serial.print("Value 1 (16-bit): ");
  //     Serial.println(value1);
  //     Serial.print("Value 2 (8-bit): ");
  //     Serial.println(value2);
  //     Serial.print("Value 3 (32-bit float): ");
  //     Serial.println(value3.f);
  //   } else {
  //     Serial.println("Received data length is less than expected.");
  //   }
  // }
//}
