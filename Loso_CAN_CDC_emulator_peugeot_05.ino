// 125kbps CAN-INFO on RD4 and no termination resistor on MCP2515 SPI module = no jumpers
// most significant inputs and help from lorddevereux and Oranż Metylowy a.k.a. Kuba @Discord's OpenLeo
// main inspirations from
// https://autowp.github.io/
// https://github.com/kuba2k2/CDCEmu/

#define SKIP_0E6_COUNT 10
#define SKIP_0F6_COUNT  2
#define CANTP_ADDR 0x0A4

#include <mcp_can.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char MsgString[256]; // serial string
char str_tmp[12]; // used for dtostr float

#define CAN0_CS  10 // Set CS  to pin 10
#define CAN0_INT  2 // Set INT to pin  2
MCP_CAN CAN0(CAN0_CS);
uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

void setup() {
  //Serial.begin(230400); // be a bit faster than CAN 125kbps...
  Serial.begin(115200); // be a bit faster than CAN 125kbps...
  while (!Serial) {
    // Leonardo
  }
  delay(2000);
  Serial.println("Loso_CAN_CDC_emulator_peugeot_05");
  pinMode(CAN0_INT, INPUT);
  if (CAN0.begin(MCP_STDEXT, CAN_125KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("Error Initializing MCP2515...");
  }
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
  for (int b = 7; b >= 0; b --) {
    Serial.print(bitRead(inByte, b));
  }
}

uint32_t now_millis;

uint32_t Timer200_every_ms =  200;  // 162
uint32_t next_Timer200_check = 3000;

uint32_t Timer500_every_ms =  500;  // 1A0, 1A2, 1E2 MM:SS,365, 3A5   ////2A5 RDS 8xASCII
uint32_t next_Timer500_check = 3050;

uint32_t Timer1000_every_ms = 1000; // 531, 0E2, time ticking seconds
uint32_t next_Timer1000_check = 3100;


uint8_t cdc_num_disks = 1;

uint8_t cdc_disk_num = 1;

uint8_t cdc_disk_num_tracks = 6;
uint8_t cdc_disk_length_minutes = 80;
uint8_t cdc_disk_length_seconds =  0;

uint8_t cdc_track_num = 1;

uint8_t cdc_track_length_minutes = 80;
uint8_t cdc_track_length_seconds =  0;
uint8_t cdc_track_playing_minutes = 0;
uint8_t cdc_track_playing_seconds = 0;

uint8_t cdc_playing0 = 0x20; // 0xA0
uint8_t old_cdc_playing0 = cdc_playing0;

uint8_t counter0F6 = 0; // every 2nd, not every received message should be printed
uint8_t counter0E6 = 0; // every 5th

uint8_t tmp0_counter = 0;
uint8_t old_tmp0_counter = tmp0_counter;
uint8_t tmp1_counter = 0;
uint8_t tmp2_counter = 0;

//uint8_t sent125 = 0;
uint8_t sent0A4 = 0;


void loop() {
  now_millis = millis();

  if (now_millis >= next_Timer200_check) {
    next_Timer200_check += Timer200_every_ms;
    data[0] = cdc_playing0;
    switch (tmp0_counter) {
      case 0:
        data[1] = 0b10000000; // wait
        break;
      case 1:
        data[1] = 0b00000001; // stop
        //data[1] = 0b00000011; // play
        break;
      case 2:
        data[1] = 0b00000010; // pause
        //data[1] = 0b00000011; // play
        break;
      case 3:
        data[1] = 0b00000011; // play
        break;
      case 4:
        data[1] = 0b00000111; // playplay
        //data[1] = 0b00000011; // play
        break;
    }
    data[2] = 0x06;
    data[3] = cdc_disk_num; // 0x05; // current disk #
    data[4] = 0x00;
    data[5] = 0xFF; // 0xFF was working, positive value
    data[6] = 0x00;
    CAN0.sendMsgBuf(0x162, 0, 7, data);
    //Serial.println("Timer200 0x162 CDC current disk");

  }

  if (now_millis >= next_Timer500_check) {
    next_Timer500_check += Timer500_every_ms;

    data[0] = 0x92;
    data[1] = 0x00;
    CAN0.sendMsgBuf(0x1A0, 0, 2, data);
    //Serial.println("Timer500 0x1A0 CDC available");

    data[0] = cdc_disk_num_tracks;
    data[1] = cdc_disk_length_minutes;
    data[2] = cdc_disk_length_seconds;
    data[3] = 0x00;
    data[4] = 0x00;
    CAN0.sendMsgBuf(0x1A2, 0, 5, data);
    //Serial.println("Timer500 0x1A2 CDC track count");

    data[0] = cdc_track_num;
    if (tmp0_counter > 0) {
      data[1] = cdc_track_length_minutes;
      data[2] = cdc_track_length_seconds;
      data[3] = cdc_track_playing_minutes;
      data[4] = cdc_track_playing_seconds;
      data[5] = 0x00;
    } else {
      data[1] = 0xFF;
      data[2] = 0xFF;
      data[3] = 0xFF;
      data[4] = 0x7F;
      data[5] = 0x80;
    }
    data[6] = 0x00;
    CAN0.sendMsgBuf(0x1E2, 0, 7, data);
    //sprintf(MsgString, "Timer500 0x1E2 CDC current track %03d:%02d", cdc_track_playing_minutes, cdc_track_playing_seconds);
    //Serial.println(MsgString);
    //Serial.println("Timer500 0x1E2 CDC current track");

    /*
      data[0] = cdc_num_tracks; // number of tracks
      data[1] = 69; // minutes
      data[2] = 30; // seconds
      data[3] = 0x00; // audio cd 0, mp3 1
      data[4] = 0x00;
      CAN0.sendMsgBuf(0x365, 0, 5, data);
      //Serial.println("Timer500 0x365 CD disk info");

      data[0] = cdc_track_num;
      data[1] = 20; //0xFF;
      data[2] = 0;//0xFF;
      data[3] = cdc_track_playing_minutes;
      data[4] = cdc_track_playing_seconds;
      data[5] = 0x00;
      CAN0.sendMsgBuf(0x3A5, 0, 6, data);
      //Serial.println("Timer500 0x3A5 CD current track info");

      data[0] = 0x00;
      data[1] = 0x0B; // ready for reading
      data[2] = 0x00;
      CAN0.sendMsgBuf(0x325, 0, 3, data);
      //Serial.println("Timer500 0x325 CD tray info");
    */

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
    //if ( (old_tmp0_counter == 2) && (tmp0_counter == 3) && (sent0A4 == 0) ) {
    //if (tmp1_counter % 2 == 0) {
    //  sent0A4 = 1;
    //  send0A4();
    //}
  }



  if (now_millis >= next_Timer1000_check) {
    next_Timer1000_check += Timer1000_every_ms;

    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = (uint8_t)((uint8_t)cdc_num_disks << 4); // 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    CAN0.sendMsgBuf(0x0E2, 0, 8, data); // CDC current disk
    //Serial.println("Timer1000 0x0E2 CDC current disk");

    data[0] = 0x09;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x00;
    data[4] = 0x00;
    data[5] = 0x00;
    data[6] = 0x00;
    data[7] = 0x00;
    CAN0.sendMsgBuf(0x531, 0, 8, data);
    //Serial.println("Timer1000 0x531 CDC Yatour");
    //dtostrf(0.001 * now_millis, 8, 3, str_tmp);
    //sprintf(MsgString, "%s 531 8, Keepalive %03d:%02d", str_tmp, cdc_track_playing_minutes, cdc_track_playing_seconds);
    //Serial.println(MsgString);

    if (tmp0_counter > 1) {
      cdc_track_playing_seconds ++; // time ticking
      if (cdc_track_playing_seconds > 59) {
        cdc_track_playing_seconds = 0;
        cdc_track_playing_minutes ++;
      }
    }

    Serial.println();
    //Serial.println();
    Serial.print("tmp0_counter = ");
    Serial.println(tmp0_counter);
    Serial.print("tmp1_counter = ");
    Serial.println(tmp1_counter);

    tmp2_counter ++;
    if (tmp2_counter % 2 == 0) {
      tmp1_counter += 1;
    }
    if (tmp1_counter % 1 == 0) {
      if (tmp0_counter < 4) {
        old_tmp0_counter = tmp0_counter;
        tmp0_counter ++;
      }
    }

  }

  if (!digitalRead(CAN0_INT)) {               // If CAN0_INT pin is low, read receive buffer
    CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
    dtostrf(0.001 * now_millis, 8, 3, str_tmp);
    sprintf(MsgString, "%s %.3lX %d", str_tmp, rxId, len);

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
        //next_Timer200_check = now_millis + 20; // attemt to be more synchonized

        old_cdc_playing0 = cdc_playing0;
        if ( (rxBuf[0] & 0b10000000) && (rxBuf[0] & 0b00000010) ) { // start to play
          cdc_playing0 = 0xA0; // 0xA0 was working
        } else { // stop to play
          cdc_playing0 = 0x20; // 0x20 was working
        }
        if ( (old_cdc_playing0 == 0x20) && (cdc_playing0 == 0xA0) ) {
          sprintf(MsgString, "%s, CDC cmd play", MsgString);
          Serial.println(MsgString);
          tmp0_counter = 0;
          sent0A4 = 0;
          cdc_track_playing_minutes = 0;
          cdc_track_playing_seconds = 0;
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
            if (tmp0_counter > 0) { // debounce
              tmp0_counter = 0;
              tmp1_counter = 0;
              sent0A4 = 0;
              if (cdc_track_num >= cdc_disk_num_tracks) {
                cdc_track_num = 1;
              } else {
                cdc_track_num ++;
              }
              cdc_track_playing_minutes = 0;
              cdc_track_playing_seconds = 0;
            }
          }
          if (rxBuf[0] & 0b01000000) {
            sprintf(MsgString, "%s, Backward", MsgString);
            if (tmp0_counter > 0) { // debounce
              tmp0_counter = 0;
              tmp1_counter = 0;
              sent0A4 = 0;
              if (cdc_track_num <= 1) {
                cdc_track_num = cdc_disk_num_tracks;
              } else {
                cdc_track_num --;
              }
              cdc_track_playing_minutes = 0;
              cdc_track_playing_seconds = 0;
            }
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
  delay(1);
}

#define CAN_TP_DELAY 4

void send0A4() {
  Serial.println("Sending CAN-TP 0x0A4");
  /*
      data[0] = 0x05; // CAN TP single frame 5 bytes
      data[1] = 0x43;
      data[2] = 0x44;
      data[3] = 0x45;
      data[4] = 0x00;
      CAN0.sendMsgBuf(CANTP_ADDR, 0, 5, data);
  */
  /*
    // 20 00 40 05
    data[0] = 0x04; // CAN TP single frame 5 bytes
    data[1] = 0x20;
    data[2] = 0x00;
    data[3] = 0x40;
    data[4] = cdc_track_num + 1;
    CAN0.sendMsgBuf(CANTP_ADDR, 0, 5, data);
  */
  // 20 00 98 01 54 68 65 20 43 72 61 6e 62 65 72 72 69 65 73 00 00 00 00 00 41 6e 69 6d 61 6c 20 49 6e 73 74 69 6e 63 74 00 00 00 00 00
  // 20 00 98 01 54 68 65 20 43 72
  // 61 6e 62 65 72 72 69 65 73 00
  // 00 00 00 00 41 6e 69 6d 61 6c
  // 20 49 6e 73 74 69 6e 63 74 00
  // 00 00 00 00
#ifdef CANTP_ADDR
  RXLED1;

  // 20 00 98 01 54 68
  data[0] = 0x10; // header CAN TP First frame 6 bytes
  data[1] = 44; // header # bytes in payload = 6 + 7 + 7 + 7 + 7 + 7 + 3
  data[2] = 0x20; //(16 * (tmp1_counter % 8));// + (tmp1_counter % 16); // 0x20 CD, 0x10 RDS
  data[3] = 0x00;
  data[4] = 0xFF;//0x58 + (16 * (tmp1_counter % 16)) + (tmp1_counter % 16);//0x58; //tmp1_counter; //0xFF;//0x58; // message contains author, etc....
  data[5] = 0x80; //cdc_track_num;//0x01;
  data[6] = 0x54;
  data[7] = 0x68;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
  delay(CAN_TP_DELAY);
  // 65 20 43 72 61 6e 62
  data[0] = 0x21; // CAN TP Consecutive frame 7 bytes
  data[1] = 0x65;
  data[2] = 0x20;
  data[3] = 0x43;
  data[4] = 0x72;
  data[5] = 0x61;
  data[6] = 0x6e;
  data[7] = 0x62;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
  delay(CAN_TP_DELAY);
  // 65 72 72 69 65 73 00
  data[0] = 0x22; // CAN TP Consecutive frame 7 bytes
  data[1] = 0x65;
  data[2] = 0x72;
  data[3] = 0x72;
  data[4] = 0x69;
  data[5] = 0x65;
  data[6] = 0x73;
  data[7] = 0x00;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
  delay(CAN_TP_DELAY);
  // 00 00 00 00 41 6e 69
  data[0] = 0x23; // CAN TP Consecutive frame 7 bytes
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x41;
  data[6] = 0x6e;
  data[7] = 0x69;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
  delay(CAN_TP_DELAY);
  // 6d 61 6c 20 49 6e 73
  data[0] = 0x24; // CAN TP Consecutive frame 7 bytes
  data[1] = 0x6d;
  data[2] = 0x61;
  data[3] = 0x6c;
  data[4] = 0x20;
  data[5] = 0x49;
  data[6] = 0x6e;
  data[7] = 0x73;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
  delay(CAN_TP_DELAY);
  // 74 69 6e 63 74 00 00
  data[0] = 0x25; // CAN TP Consecutive frame 7 bytes
  data[1] = 0x74;
  data[2] = 0x69;
  data[3] = 0x6e;
  data[4] = 0x63;
  data[5] = 0x74;
  data[6] = 0x00;
  data[7] = 0x00;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 8, data);
  delay(CAN_TP_DELAY);
  // 00 00 00
  data[0] = 0x26; // CAN TP Consecutive frame 7 bytes
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  CAN0.sendMsgBuf(CANTP_ADDR, 0, 4, data);
  delay(CAN_TP_DELAY);
  RXLED0;
#endif // CANTP_ADDR
}

void send125(void) {
  Serial.println("Sending CAN-TP 0x125");
  /*
    43.024 125 7, 06, 70, FF, 00, 00, 00, 00
    43.235 125 8, 10, A6, 70, 06, 00, 40, FF, 00
    43.247 125 8, 21, 45, 76, 61, 20, 4B, 6F, 73
    43.257 125 8, 22, 74, 6F, 6C, 61, 6E, 79, 69
    43.269 125 8, 23, 6F, 76, 61, 00, 00, 00, 41
    43.280 125 8, 24, 7A, 20, 62, 75, 64, 65, 20
    43.292 125 8, 25, 70, 6F, 6B, 6F, 73, 65, 6E
    43.302 125 8, 26, 61, 20, 74, 72, 61, 50, 65
    43.314 125 8, 27, 74, 65, 72, 20, 56, 61, 73
    43.325 125 8, 28, 65, 6B, 2C, 20, 44, 75, 73
    43.336 125 8, 29, 61, 6E, 20, 52, 4F, 74, 76
    43.346 125 8, 2A, 61, 72, 61, 6A, 74, 65, 20
    43.358 125 8, 2B, 6B, 61, 73, 69, 6E, 6F, 00
    43.370 125 8, 2C, 00, 00, 00, 4D, 61, 72, 63
    43.380 125 8, 2D, 65, 6C, 61, 20, 4C, 61, 69
    43.390 125 8, 2E, 66, 65, 72, 6F, 76, 61, 00
    43.402 125 8, 2F, 00, 00, 50, 6F, 64, 20, 62
    43.413 125 8, 20, 69, 65, 6C, 6F, 75, 20, 61
    43.424 125 8, 21, 6C, 65, 6A, 6F, 75, 00, 00
    43.436 125 8, 22, 00, 44, 75, 73, 61, 6E, 20
    43.447 125 8, 23, 47, 72, 75, 6E, 00, 00, 00
    43.458 125 8, 24, 00, 00, 00, 00, 00, 00, 00
    43.468 125 8, 25, 4D, 61, 72, 69, 6E, 61, 00
    43.481 125 8, 26, 00, 00, 00, 00, 00, 00, 00
    43.491 125 7, 27, 00, 00, 00, 00, 00, 00
    46.800 125 7, 06, 00, 00, 00, 00, 00, 00
    46.826 125 7, 06, 00, 00, 00, 00, 00, 00
  */
  //    43.024 125 7, 06, 70, FF, 00, 00, 00, 00
  data[0] = 0x06;
  data[1] = 0x70;
  data[2] = 0xFF;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 7, data);
  delay(CAN_TP_DELAY);
  //    43.235 125 8, 10, A6, 70, 06, 00, 40, FF, 00
  data[0] = 0x10;
  data[1] = 0xA6;
  data[2] = 0x70;
  data[3] = 0x06;
  data[4] = 0x00;
  data[5] = 0x40;
  data[6] = 0xFF;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.247 125 8, 21, 45, 76, 61, 20, 4B, 6F, 73
  data[0] = 0x21;
  data[1] = 0x45;
  data[2] = 0x76;
  data[3] = 0x61;
  data[4] = 0x20;
  data[5] = 0x4B;
  data[6] = 0x6F;
  data[7] = 0x73;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.257 125 8, 22, 74, 6F, 6C, 61, 6E, 79, 69
  data[0] = 0x22;
  data[1] = 0x74;
  data[2] = 0x6F;
  data[3] = 0x6C;
  data[4] = 0x61;
  data[5] = 0x6E;
  data[6] = 0x79;
  data[7] = 0x69;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.269 125 8, 23, 6F, 76, 61, 00, 00, 00, 41
  data[0] = 0x23;
  data[1] = 0x6F;
  data[2] = 0x76;
  data[3] = 0x61;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x41;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.280 125 8, 24, 7A, 20, 62, 75, 64, 65, 20
  data[0] = 0x24;
  data[1] = 0x7A;
  data[2] = 0x20;
  data[3] = 0x62;
  data[4] = 0x75;
  data[5] = 0x64;
  data[6] = 0x65;
  data[7] = 0x20;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.292 125 8, 25, 70, 6F, 6B, 6F, 73, 65, 6E
  data[0] = 0x25;
  data[1] = 0x70;
  data[2] = 0x6F;
  data[3] = 0x6B;
  data[4] = 0x6F;
  data[5] = 0x73;
  data[6] = 0x65;
  data[7] = 0x6E;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.302 125 8, 26, 61, 20, 74, 72, 61, 50, 65
  data[0] = 0x26;
  data[1] = 0x61;
  data[2] = 0x20;
  data[3] = 0x74;
  data[4] = 0x72;
  data[5] = 0x61;
  data[6] = 0x50;
  data[7] = 0x65;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.314 125 8, 27, 74, 65, 72, 20, 56, 61, 73
  data[0] = 0x27;
  data[1] = 0x74;
  data[2] = 0x65;
  data[3] = 0x72;
  data[4] = 0x20;
  data[5] = 0x56;
  data[6] = 0x61;
  data[7] = 0x73;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.325 125 8, 28, 65, 6B, 2C, 20, 44, 75, 73
  data[0] = 0x28;
  data[1] = 0x65;
  data[2] = 0x6B;
  data[3] = 0x2C;
  data[4] = 0x20;
  data[5] = 0x44;
  data[6] = 0x75;
  data[7] = 0x73;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.336 125 8, 29, 61, 6E, 20, 52, 4F, 74, 76
  data[0] = 0x29;
  data[1] = 0x61;
  data[2] = 0x6E;
  data[3] = 0x20;
  data[4] = 0x52;
  data[5] = 0x4F;
  data[6] = 0x74;
  data[7] = 0x76;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.346 125 8, 2A, 61, 72, 61, 6A, 74, 65, 20
  data[0] = 0x2A;
  data[1] = 0x61;
  data[2] = 0x72;
  data[3] = 0x61;
  data[4] = 0x6A;
  data[5] = 0x74;
  data[6] = 0x65;
  data[7] = 0x20;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.358 125 8, 2B, 6B, 61, 73, 69, 6E, 6F, 00
  data[0] = 0x2B;
  data[1] = 0xB6;
  data[2] = 0x61;
  data[3] = 0x73;
  data[4] = 0x69;
  data[5] = 0x6E;
  data[6] = 0x6F;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.370 125 8, 2C, 00, 00, 00, 4D, 61, 72, 63
  data[0] = 0x2C;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x4D;
  data[5] = 0x61;
  data[6] = 0x72;
  data[7] = 0x63;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.380 125 8, 2D, 65, 6C, 61, 20, 4C, 61, 69
  data[0] = 0x2D;
  data[1] = 0x65;
  data[2] = 0x6C;
  data[3] = 0x61;
  data[4] = 0x20;
  data[5] = 0x4C;
  data[6] = 0x61;
  data[7] = 0x69;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.390 125 8, 2E, 66, 65, 72, 6F, 76, 61, 00
  data[0] = 0x2E;
  data[1] = 0x66;
  data[2] = 0x65;
  data[3] = 0x72;
  data[4] = 0x6F;
  data[5] = 0x76;
  data[6] = 0x61;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.402 125 8, 2F, 00, 00, 50, 6F, 64, 20, 62
  data[0] = 0x2F;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x50;
  data[4] = 0x6F;
  data[5] = 0x64;
  data[6] = 0x20;
  data[7] = 0x62;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.413 125 8, 20, 69, 65, 6C, 6F, 75, 20, 61
  data[0] = 0x20;
  data[1] = 0x69;
  data[2] = 0x65;
  data[3] = 0x6C;
  data[4] = 0x6F;
  data[5] = 0x75;
  data[6] = 0x20;
  data[7] = 0x61;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.424 125 8, 21, 6C, 65, 6A, 6F, 75, 00, 00
  data[0] = 0x21;
  data[1] = 0x6C;
  data[2] = 0x65;
  data[3] = 0x6A;
  data[4] = 0x6F;
  data[5] = 0x75;
  data[6] = 0x00;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.436 125 8, 22, 00, 44, 75, 73, 61, 6E, 20
  data[0] = 0x22;
  data[1] = 0x00;
  data[2] = 0x44;
  data[3] = 0x75;
  data[4] = 0x73;
  data[5] = 0x61;
  data[6] = 0x6E;
  data[7] = 0x20;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.447 125 8, 23, 47, 72, 75, 6E, 00, 00, 00
  data[0] = 0x23;
  data[1] = 0x47;
  data[2] = 0x72;
  data[3] = 0x75;
  data[4] = 0x6E;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.458 125 8, 24, 00, 00, 00, 00, 00, 00, 00
  data[0] = 0x24;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.468 125 8, 25, 4D, 61, 72, 69, 6E, 61, 00
  data[0] = 0x25;
  data[1] = 0x4D;
  data[2] = 0x61;
  data[3] = 0x72;
  data[4] = 0x69;
  data[5] = 0x6E;
  data[6] = 0x61;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.481 125 8, 26, 00, 00, 00, 00, 00, 00, 00
  data[0] = 0x26;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 8, data);
  delay(CAN_TP_DELAY);
  //    43.491 125 7, 27, 00, 00, 00, 00, 00, 00
  data[0] = 0x27;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  CAN0.sendMsgBuf(0x125, 0, 7, data);
  delay(CAN_TP_DELAY);
}
