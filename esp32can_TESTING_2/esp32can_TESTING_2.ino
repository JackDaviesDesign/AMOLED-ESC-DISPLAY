#include <ESP32CAN.h>
#include <CAN_config.h>

// CAN configuration
CAN_device_t CAN_cfg; // CAN Config
const int rx_queue_size = 10;

const uint32_t filterId1 = 0x618; // CAN ID to filter
const uint32_t filterId2 = 0x620; // CAN ID to filter


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

  // Check for received CAN messages
  CAN_frame_t rx_frame;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    if (rx_frame.MsgID == filterId1) { // Check if the message ID matches the filter ID
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
        uint8_t SOC = data[2]; // 8-bit integer // SOC %
        uint8_t SOH = data[3]; // 8-bit integer // SOH %
        float battV = data[4] | (data[5] << 8); // 16-bit integer // batt voltage
        float battA = data[6] | (data[7] << 8); // 16-bit integer // batt current        

        // Print the extracted values
        Serial.print("SOC: ");
        Serial.println(SOC);
        Serial.print("SOH: ");
        Serial.println(SOH);
        Serial.print("Batt V: ");
        Serial.println(battV/100);
        Serial.print("Batt A: ");
        Serial.println(battA/100);

      } else {
        Serial.println("Received data length is less than expected.");
      }
    }





if (rx_frame.MsgID == filterId2) { // Check if the message ID matches the filter ID
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

        // // 32-bit float - TRIP
        // union {
        //   uint8_t b[4];
        //   float f;
        // } TRIP;
        // TRIP.b[0] = data[0];
        // TRIP.b[1] = data[1];
        // TRIP.b[2] = data[2];
        // TRIP.b[3] = data[3];

        // 32-bit float - ODO
        union {
          uint8_t b[4];
          float f;
        } ODO;
        ODO.b[0] = data[4];
        ODO.b[1] = data[5];
        ODO.b[2] = data[6];
        ODO.b[3] = data[7];

        // Serial.print("TRIP : ");
        // Serial.println(TRIP.f);

        Serial.print("ODO : ");
        Serial.println(ODO.f);


      } else {
        Serial.println("Received data length is less than expected.");
      }
    }







  }
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
