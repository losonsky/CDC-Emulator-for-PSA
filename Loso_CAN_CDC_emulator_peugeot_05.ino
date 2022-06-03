// CAN peugeot 125kbps CAN-INFO no termination resistor MCP2515 SPI module
// https://autowp.github.io/
// https://github.com/kuba2k2/CDCEmu/

//#define TEST_MESSAGES

#define SKIP_0E6_COUNT 10
#define SKIP_0F6_COUNT  2

#include <mcp_can.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char MsgString[256]; // Array to store serial string
char str_tmp[12];

#define CAN0_INT  2 // Set INT to pin  2
#define CAN0_CS  10 // Set CS  to pin 10
MCP_CAN CAN0(CAN0_CS);
uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

void setup() {
  Serial.begin(230400); // be a bit faster than CAN 125kbps...
  while (!Serial) {
    // Leonardo
  }
  delay(2000);
  Serial.println("Loso_CAN_CDC_emulator_peugeot_05");

  pinMode(CAN0_INT, INPUT);
  CAN0.begin(MCP_STD, CAN_125KBPS, MCP_8MHZ); // STD filtering
  CAN0.init_Mask(0, 0, 0x7FF0000);
  CAN0.init_Mask(1, 0, 0x7FF0000);

  CAN0.init_Filt(0, 0, 0x21F0000); // RC under seering wheel
  CAN0.init_Filt(1, 0, 0x1310000); // CDC command
  CAN0.init_Filt(2, 0, 0x0E60000); // Voltage
  CAN0.init_Filt(3, 0, 0x0F60000); // Temperature

  CAN0.setMode(MCP_NORMAL);
  Serial.println("Setup done.");
}

void printBinary(byte inByte) {
  //Serial.print("0b");
  for (int b = 7; b >= 0; b --) {
    Serial.print(bitRead(inByte, b));
  }
}

uint32_t now_millis;

uint32_t Msg0_every_ms =  200;  // 162
uint32_t next_Msg0_check = 3000;

uint32_t Msg1_every_ms =  500;  // 1A2, 2A5 RDS 8xASCII
uint32_t next_Msg1_check = 3050;

uint32_t Msg2_every_ms =  500;  // 1E2 MM:SS
uint32_t next_Msg2_check = 3100;

uint32_t Msg3_every_ms =  500;  // 1A0
uint32_t next_Msg3_check = 3150;

uint32_t Msg4_every_ms = 1000;  // 531
uint32_t next_Msg4_check = 3200;

uint32_t Msg5_every_ms = 1000;  // 0E2
uint32_t next_Msg5_check = 3250;

uint32_t Msg6_every_ms = 1000; // time ticking seconds
uint32_t next_Msg6_check = 3300;

uint8_t cdc_minutes = 0;
uint8_t cdc_seconds = 0;

uint8_t cdc_playing0 = 0x20;
uint8_t cdc_playing1 = 0x01;

uint8_t old_cdc_playing0 = cdc_playing0;

uint8_t counter0F6 = 0; // every 2nd, not every received message should be printed
uint8_t counter0E6 = 0; // every 5th

uint8_t tmp_counter = 1;
uint8_t tmp2_counter = 0;

void loop() {
  now_millis = millis();
  //RXLED0;
  if (now_millis >= next_Msg0_check) {
    next_Msg0_check += Msg0_every_ms;
    data[0] = cdc_playing0;
    data[1] = cdc_playing1;
    data[2] = 0x06;
    data[3] = 0x05; // current disk #
    data[4] = 0x00;
    data[5] = 0xFF; // tmp_counter; // enable source 0xFF was working
    data[6] = 0x00;
    CAN0.sendMsgBuf(0x162, 0, 7, data);
    //Serial.println("Msg0 0x162 CDC current disk");
  }

  if (now_millis >= next_Msg1_check) {
    next_Msg1_check += Msg1_every_ms;
    data[0] = 0x63;
    data[1] = 0x58;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    CAN0.sendMsgBuf(0x1A2, 0, 5, data);
    //Serial.println("Msg1 0x1A2 CDC track count");
  }

  if (now_millis >= next_Msg2_check) {
    next_Msg2_check += Msg2_every_ms;
    data[0] = 0x35; // 53 track #
    data[1] = 0x05;
    data[2] = 0x00;
    data[3] = cdc_minutes;
    data[4] = cdc_seconds;
    data[5] = 0x00;
    data[6] = 0x00;
    CAN0.sendMsgBuf(0x1E2, 0, 7, data);
    //sprintf(MsgString, "Msg2 0x1E2 CDC current track %03d:%02d", cdc_minutes, cdc_seconds);
    //Serial.println(MsgString);
    //Serial.println("Msg2 0x1E2 CDC current track");
  }

  if (now_millis >= next_Msg3_check) {
    next_Msg3_check += Msg3_every_ms;
    data[0] = 0x92;
    data[1] = 0x00;
    CAN0.sendMsgBuf(0x1A0, 0, 2, data);
    //Serial.println("Msg3 0x1A0 CDC available");
  }

  if (now_millis >= next_Msg4_check) {
  RXLED1;
    next_Msg4_check += Msg4_every_ms;
    data[0] = 0x09;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    CAN0.sendMsgBuf(0x531, 0, 8, data);
    //Serial.println("Msg4 0x531 CDC Yatour");
    //dtostrf(0.001 * now_millis, 8, 3, str_tmp);
    //sprintf(MsgString, "%s 531 8, Keepalive %03d:%02d", str_tmp, cdc_minutes, cdc_seconds);
    //Serial.println(MsgString);
    /*
        data[0] = 0x4C; // ASCII RDS
        data[1] = 0x6F;
        data[2] = 0x73;
        data[3] = 0x6F;
        data[4] = 0x43;
        data[5] = 0x44;
        data[6] = 0x43;
        data[7] = 0x00;
        CAN0.sendMsgBuf(0x2A5, 0, 8, data);
    */
    /*
        data[0] = 0x05; // CAN TP single frame 5 bytes
        data[1] = 0x43;
        data[2] = 0x44;
        data[3] = 0x45;
        data[4] = 0x00;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 5, data);
    */


#define CANTP_ADDR 0x0A4
#ifdef CANTP_ADDR
    data[0] = 0x10; // header CAN TP First frame 6 bytes
    data[1] = 27; // header # of bytes in payload => 6 + 7 + 7 + 7
    data[2] = 0x20; // 0x20 CD, 0x10 RDS
    data[3] = 0x00;
    data[4] = 0b01011000; // message contains author, etc....
    data[5] = 0x35; // track # must much # from CDCEmu?
    data[6] = 0x4C;
    data[7] = 0x6F;
    CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
    delay(10);

    data[0] = 0x21; // CAN TP Consecutive frame 7 bytes
    data[1] = 0x73;
    data[2] = 0x6F;
    data[3] = 0x20;
    data[4] = 0x43;
    data[5] = 0x44;
    data[6] = 0x43;
    data[7] = 0x2E;
    CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
    delay(10);

    data[0] = 0x22; // CAN TP Consecutive frame 7 bytes
    data[1] = 0x20;
    data[2] = 0x46;
    data[3] = 0x2E;
    data[4] = 0x2E;
    data[5] = 0x6B;
    data[6] = 0x20;
    data[7] = 0x79;
    CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
    delay(10);

    data[0] = 0x23; // CAN TP Consecutive frame 7 bytes
    data[1] = 0x6F;
    data[2] = 0x75;
    data[3] = 0x20;
    data[4] = 0x50;
    data[5] = 0x53;
    data[6] = 0x41;
    data[7] = 0x21;
    CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
    delay(10);

    Serial.println("Loso_CAN_CDC_emulator_peugeot_05");
  RXLED0;

    /*
      Serial.println(tmp_counter);
      tmp2_counter ++;
      if (tmp2_counter % 5 == 0) {
      if (tmp_counter == 128) {
        tmp_counter = 1;
      } else {
        tmp_counter *= 2;
      }
      }
    */

    /*
        data[0] = 0x23; // CAN TP Consecutive frame 7 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        data[7] = 0x50;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
        delay(10);

        data[0] = 0x24; // CAN TP Consecutive frame 7 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        data[7] = 0x50;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
        delay(10);

        data[0] = 0x25; // CAN TP Consecutive frame 7 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        data[7] = 0x50;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
        delay(10);

        data[0] = 0x26; // CAN TP Consecutive frame 7 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        data[7] = 0x50;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
        delay(10);

        data[0] = 0x27; // CAN TP Consecutive frame 7 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        data[7] = 0x50;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
        delay(10);

        data[0] = 0x28; // CAN TP Consecutive frame 7 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        data[7] = 0x50;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
        delay(10);

        data[0] = 0x29; // CAN TP Consecutive frame 6 bytes
        data[1] = 0x4A;
        data[2] = 0x4B;
        data[3] = 0x4C;
        data[4] = 0x4D;
        data[5] = 0x4E;
        data[6] = 0x4F;
        CAN0.sendMsgBuf(CANTP_ADDR, 0, 7, data);
        delay(10);
    */
#endif // CANTP_ADDR
  }

  if (now_millis >= next_Msg5_check) {
    next_Msg5_check += Msg5_every_ms;
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    CAN0.sendMsgBuf(0x0E2, 0, 8, data); // CDC current disk

#ifdef TEST_MESSAGES
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x71;
    data[6] = 0x00;
    data[7] = 0x00;
    CAN0.sendMsgBuf(0x0E6, 0, 8, data); // Voltage

    data[0] = 0x88;
    data[1] = 0x49;
    data[2] = 0x16;
    data[3] = 0xA2;
    data[4] = 0xF6;
    data[5] = 0x7C;
    data[6] = 0x7C;
    data[7] = 0x20;
    CAN0.sendMsgBuf(0x0F6, 0, 8, data); // Temperature
#endif // TEST_MESSAGES
    //Serial.println("Msg5 0x0E2 CDC current disk");
  }

  if (now_millis >= next_Msg6_check) { // time ticking
    next_Msg6_check += Msg6_every_ms;
    cdc_seconds ++;
    if (cdc_seconds > 59) {
      cdc_seconds = 0;
      cdc_minutes ++;
    }
  }

  if (!digitalRead(CAN0_INT)) {               // If CAN0_INT pin is low, read receive buffer
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    dtostrf(0.001 * now_millis, 8, 3, str_tmp);
    sprintf(MsgString, "%s %.3lX %d", str_tmp, rxId, len);
    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(MsgString, "%s REMOTE REQUEST FRAME", MsgString);
      Serial.println(MsgString);
    } else {
      switch (rxId) {
        case 0x0E6:
          counter0E6 ++;
          if (counter0E6 >= SKIP_0E6_COUNT) {
            counter0E6 = 0;
            float voltage = 0.05 * ((uint16_t)rxBuf[5] + 144);
            dtostrf(voltage, 5, 2, str_tmp);
            sprintf(MsgString, "%s, BatV = %s", MsgString, str_tmp);
            Serial.println(MsgString);
          }
          break;
        case 0x0F6:
          counter0F6 ++;
          if (counter0F6 >= SKIP_0F6_COUNT) {
            counter0F6 = 0;
            if (rxBuf[0] & 0b10001000) {
              sprintf(MsgString, "%s, IG_1", MsgString);
            } else {
              sprintf(MsgString, "%s, IG_0", MsgString);
            }
            if (rxBuf[7] & 0b10000000) {
              sprintf(MsgString, "%s, RV_1", MsgString);
            } else {
              sprintf(MsgString, "%s, RV_0", MsgString);
            }
            if (rxBuf[7] & 0b00000001) {
              sprintf(MsgString, "%s, TL_1", MsgString);
            } else {
              sprintf(MsgString, "%s, TL_0", MsgString);
            }
            if (rxBuf[7] & 0b00000010) {
              sprintf(MsgString, "%s, TR_1", MsgString);
            } else {
              sprintf(MsgString, "%s, TR_0", MsgString);
            }

            int8_t coolant = (int8_t)((uint8_t)rxBuf[1] - 39);
            sprintf(MsgString, "%s, Cool = %d", MsgString, coolant);

            uint32_t odometer = (uint32_t)((uint32_t)rxBuf[2] << 16 | (uint32_t)rxBuf[3] << 8 | (uint32_t)rxBuf[4]);
            dtostrf(0.1 * odometer, 8, 1, str_tmp);
            sprintf(MsgString, "%s, Odom = %s", MsgString, str_tmp);

            float temperature = 0.5 * ((uint8_t)rxBuf[6] - 79);
            dtostrf(temperature, 5, 1, str_tmp);
            sprintf(MsgString, "%s, Temp = %s", MsgString, str_tmp);

            Serial.println(MsgString);
            /*
              for (byte i = 0; i < len; i ++) {
              Serial.print(", ");
              printBinary(rxBuf[i]);
              sprintf(MsgString, " %.2X", rxBuf[i]);
              Serial.print(MsgString);
              }
              Serial.println();
            */
          }
          break;
        case 0x131:
          //next_Msg0_check = now_millis + 20; // attemt to be more synchonized
          old_cdc_playing0 = cdc_playing0;
          if ( (rxBuf[0] & 0b10000000) && (rxBuf[0] & 0b00000010) ) {
            cdc_playing0 = 0xA0; // 0xA0 was working
            cdc_playing1 = 0x07; // 0x03 was working
          } else {
            cdc_playing0 = 0x20; // 0x20 was working
            cdc_playing1 = 0x01; // 0x01 was working
          }
          if ( (old_cdc_playing0 == 0x20) && (cdc_playing0 == 0xA0) ) {
            sprintf(MsgString, "%s, CDC cmd play", MsgString);
            Serial.println(MsgString);
          }
          if ( (old_cdc_playing0 == 0xA0) && (cdc_playing0 == 0x20) ) {
            sprintf(MsgString, "%s, CDC cmd stop", MsgString);
            Serial.println(MsgString);
          }
          break;
        case 0x21F:
          if (rxBuf[0] & 0b11101110) {
            if (rxBuf[0] & 0b10000000) {
              sprintf(MsgString, "%s, Forward", MsgString);
            }
            if (rxBuf[0] & 0b01000000) {
              sprintf(MsgString, "%s, Backward", MsgString);
            }
            if (rxBuf[0] & 0b00100000) {
              sprintf(MsgString, "%s, Unknown", MsgString);
            }
            // 0
            if (rxBuf[0] & 0b00001000) {
              sprintf(MsgString, "%s, Volume up", MsgString);
            }
            if (rxBuf[0] & 0b00000100) {
              sprintf(MsgString, "%s, Volume down", MsgString);
            }
            if (rxBuf[0] & 0b00000010) {
              sprintf(MsgString, "%s, Source", MsgString);
            }
            // 0
            Serial.println(MsgString);
          }
          break;
        default:
          Serial.print(MsgString);
          for (byte i = 0; i < len; i ++) {
            Serial.print(", ");
            printBinary(rxBuf[i]);
            sprintf(MsgString, " %.2X", rxBuf[i]);
            Serial.print(MsgString);
          }
          Serial.println();
          break;
      }
    }
  }
  delay(2);
}
