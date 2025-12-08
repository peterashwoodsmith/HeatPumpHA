//
// This ESP32 ARDUINO program is a Zibgee end device that will receive commands to be translated into the IR remote codes for a 
// MISTUBISHI HEATE PUMP. THis is work in progress for a Home Assistant end point that you attach near the IR sensor the the
// Mitsubishi heat pump and that can be battery powered.
//


// M I T S U B I S H I  IR stuff follows

int halfPeriodicTime;
int IRpin;
int khz;

typedef enum HvacMode {
  HVAC_HOT,
  HVAC_COLD,
  HVAC_DRY,
  HVAC_FAN, // used for Panasonic only
  HVAC_AUTO
} HvacMode_t; // HVAC  MODE

typedef enum HvacFanMode {
  FAN_SPEED_1,
  FAN_SPEED_2,
  FAN_SPEED_3,
  FAN_SPEED_4,
  FAN_SPEED_5,
  FAN_SPEED_AUTO,
  FAN_SPEED_SILENT
} HvacFanMode_t;  // HVAC  FAN MODE

typedef enum HvacVanneMode {
  VANNE_AUTO,
  VANNE_H1,
  VANNE_H2,
  VANNE_H3,
  VANNE_H4,
  VANNE_H5,
  VANNE_AUTO_MOVE
} HvacVanneMode_t;  // HVAC  VANNE MODE

typedef enum HvacWideVanneMode {
  WIDE_LEFT_END,
  WIDE_LEFT,
  WIDE_MIDDLE,
  WIDE_RIGHT,
  WIDE_RIGHT_END
} HvacWideVanneMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacAreaMode {
  AREA_SWING,
  AREA_LEFT,
  AREA_AUTO,
  AREA_RIGHT
} HvacAreaMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacProfileMode {
  NORMAL,
  QUIET,
  BOOST
} HvacProfileMode_t;  // HVAC PANASONIC OPTION MODE

// HVAC MITSUBISHI_
#define HVAC_MITSUBISHI_HDR_MARK    3400
#define HVAC_MITSUBISHI_HDR_SPACE   1750
#define HVAC_MITSUBISHI_BIT_MARK    450
#define HVAC_MITSUBISHI_ONE_SPACE   1300
#define HVAC_MISTUBISHI_ZERO_SPACE  420
#define HVAC_MITSUBISHI_RPT_MARK    440
#define HVAC_MITSUBISHI_RPT_SPACE   17100 // Above original iremote limit

/****************************************************************************
/* Send IR command to Mitsubishi HVAC - sendHvacMitsubishi
/***************************************************************************/
void sendHvacMitsubishi(
  HvacMode                HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
  int                     HVAC_Temp,           // Example 21  (°c)
  HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
  HvacVanneMode           HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
  int                     OFF                  // Example false
)
{
  byte mask = 1; //our bitmask
  byte data[18] = { 0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06, 0x30, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F };
  // data array is a valid trame, only byte to be chnaged will be updated.
  byte i;

#ifdef HVAC_MITSUBISHI_DEBUG
  Serial.println("Packet to send: ");
  for (i = 0; i < 18; i++) {
    Serial.print("_");
    Serial.print(data[i], HEX);
  }
  Serial.println(".");
#endif

  // Byte 6 - On / Off
  if (OFF) {
    data[5] = (byte) 0x0; // Turn OFF HVAC
  } else {
    data[5] = (byte) 0x20; // Tuen ON HVAC
  }

  // Byte 7 - Mode
  switch (HVAC_Mode)
  {
    case HVAC_HOT:   data[6] = (byte) 0x08; break;
    case HVAC_COLD:  data[6] = (byte) 0x18; break;
    case HVAC_DRY:   data[6] = (byte) 0x10; break;
    case HVAC_AUTO:  data[6] = (byte) 0x20; break;
    default: break;
  }

  // Byte 8 - Temperature
  // Check Min Max For Hot Mode
  byte Temp;
  if (HVAC_Temp > 31) { Temp = 31;}
  else if (HVAC_Temp < 16) { Temp = 16; } 
  else { Temp = HVAC_Temp; };
  data[7] = (byte) Temp - 16;

  // Byte 10 - FAN / VANNE
  switch (HVAC_FanMode)
  {
    case FAN_SPEED_1:       data[9] = (byte) B00000001; break;
    case FAN_SPEED_2:       data[9] = (byte) B00000010; break;
    case FAN_SPEED_3:       data[9] = (byte) B00000011; break;
    case FAN_SPEED_4:       data[9] = (byte) B00000100; break;
    case FAN_SPEED_5:       data[9] = (byte) B00000100; break; //No FAN speed 5 for MITSUBISHI so it is consider as Speed 4
    case FAN_SPEED_AUTO:    data[9] = (byte) B10000000; break;
    case FAN_SPEED_SILENT:  data[9] = (byte) B00000101; break;
    default: break;
  }

  switch (HVAC_VanneMode)
  {
    case VANNE_AUTO:        data[9] = (byte) data[9] | B01000000; break;
    case VANNE_H1:          data[9] = (byte) data[9] | B01001000; break;
    case VANNE_H2:          data[9] = (byte) data[9] | B01010000; break;
    case VANNE_H3:          data[9] = (byte) data[9] | B01011000; break;
    case VANNE_H4:          data[9] = (byte) data[9] | B01100000; break;
    case VANNE_H5:          data[9] = (byte) data[9] | B01101000; break;
    case VANNE_AUTO_MOVE:   data[9] = (byte) data[9] | B01111000; break;
    default: break;
  }

  // Byte 18 - CRC
  data[17] = 0;
  for (i = 0; i < 17; i++) {
    data[17] = (byte) data[i] + data[17];  // CRC is a simple bits addition
  }

#ifdef HVAC_MITSUBISHI_DEBUG
  Serial.println("Packet to send: ");
  for (i = 0; i < 18; i++) {
    Serial.print("_"); Serial.print(data[i], HEX);
  }
  Serial.println(".");
  for (i = 0; i < 18; i++) {
    Serial.print(data[i], BIN); Serial.print(" ");
  }
  Serial.println(".");
#endif

  space(0);
  for (int j = 0; j < 2; j++) {  // For Mitsubishi IR protocol we have to send two time the packet data
    // Header for the Packet
    mark(HVAC_MITSUBISHI_HDR_MARK);
    space(HVAC_MITSUBISHI_HDR_SPACE);
    for (i = 0; i < 18; i++) {
      // Send all Bits from Byte Data in Reverse Order
      for (mask = 00000001; mask > 0; mask <<= 1) { //iterate through bit mask
        if (data[i] & mask) { // Bit ONE
          mark(HVAC_MITSUBISHI_BIT_MARK);
          space(HVAC_MITSUBISHI_ONE_SPACE);
        }
        else { // Bit ZERO
          mark(HVAC_MITSUBISHI_BIT_MARK);
          space(HVAC_MISTUBISHI_ZERO_SPACE);
        }
        //Next bits
      }
    }
    // End of Packet and retransmission of the Packet
    mark(HVAC_MITSUBISHI_RPT_MARK);
    space(HVAC_MITSUBISHI_RPT_SPACE);
    space(0); // Just to be sure
  }
}

/****************************************************************************
/* mark ( int time) 
/***************************************************************************/ 
void mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  long beginning = micros();
  while(micros() - beginning < time){
    digitalWrite(IRpin, HIGH);
    delayMicroseconds(halfPeriodicTime);
    digitalWrite(IRpin, LOW);
    delayMicroseconds(halfPeriodicTime); //38 kHz -> T = 26.31 microsec (periodic time), half of it is 13
  }
}

/****************************************************************************
/* space ( int time) 
/***************************************************************************/ 
/* Leave pin off for time (given in microseconds) */
void space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  digitalWrite(IRpin, LOW);
  if (time > 0) delayMicroseconds(time);
}


void setupIR() {
  // put your setup code here, to run once:
  IRpin=10;
  khz=38;
  halfPeriodicTime = 500/khz;
  pinMode(IRpin, OUTPUT);
  Serial.println("setupIR done");
}

// Z I G B E E STUFF FOLLOWS

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"


// Each cluster within the device has its own id, 10,11,12 ...

ZigbeeLight       zbLight1     = ZigbeeLight(10);
ZigbeeLight       zbLight2     = ZigbeeLight(11);
ZigbeeFanControl  zbFanControl = ZigbeeFanControl(12);
ZigbeePowerOutlet zbOutlet     = ZigbeePowerOutlet(13);
ZigbeeBinary      zbHotCold    = ZigbeeBinary(14);

bool             lightStatus1  = 0;
bool             lightStatus2  = 0;
ZigbeeFanMode    fanStatus     = FAN_MODE_OFF;
bool             powerStatus   = 0;
bool             hotColdStatus  = 0;

void displayPowerStatus()
{
     Serial.println(powerStatus ? "POWER ON" : "POWER OFF");
}

void displayLightStatus1()
{
     Serial.println(lightStatus1 ? "LIGHT1 ON" : "LIGHT1 OFF");
}

void displayLightStatus2()
{
     Serial.println(lightStatus2 ? "LIGHT1 ON" : "LIGHT1 OFF");
}

void displayFanStatus()
{    char buf[256];
     char *s = "UNK";
     switch (fanStatus) {
          case FAN_MODE_OFF:    s = "OFF";    break;
          case FAN_MODE_LOW:    s = "LOW";    break; 
          case FAN_MODE_MEDIUM: s = "MEDIUM"; break; 
          case FAN_MODE_HIGH:   s = "HIGH";   break; 
          case FAN_MODE_ON:     s = "ON";     break;
     }
     sprintf(buf,"FAN STATUS = %s (%x)\n", s, (unsigned int) fanStatus);
     Serial.println(buf);
}

void displayHotColdStatus()
{    char buf[256];
     sprintf(buf,"HOT COLD STATUS = %s (%x)\n", hotColdStatus ? "COLD" : "HOT", (unsigned int) hotColdStatus);
     Serial.println(buf);
}


void setLight1(bool value)
{
     lightStatus1 = value;
     Serial.print("HA"); displayLightStatus1();
     if (lightStatus1) {
        sendHvacMitsubishi(HVAC_HOT, 21, FAN_SPEED_AUTO, VANNE_AUTO_MOVE, false /* NOT OFF */);
     } else {
        sendHvacMitsubishi(HVAC_HOT, 21, FAN_SPEED_AUTO, VANNE_AUTO_MOVE, true /* YES OFF */);
     }
}

void setLight2(bool value)
{
     lightStatus2 = value;
     Serial.print("HA=> "); displayLightStatus2();
}


void setFanMode(ZigbeeFanMode mode);   // Compiler seems to need this or gets confused by enum
void setFanMode(ZigbeeFanMode mode)
{
  fanStatus = (ZigbeeFanMode) (((unsigned int) mode) & 0xff);  // we get garbage bytes at the top
  Serial.print("HA=> "); displayFanStatus();
}

void setPower(bool value)
{
     powerStatus = value;
     Serial.print("HA=> "); displayPowerStatus();
}

void setHotCold(bool state)
{
     hotColdStatus = state;
     Serial.print("HA=> "); displayHotColdStatus();
}

// BASIC ARDUINO SETUP

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();

  setupIR();
  Serial.println("MITS IR setup");
  Serial.println("RiverView Zigbee light");
  //
  zbLight1.setManufacturerAndModel("RiverView", "ESP32C6Light");
  zbLight2.setManufacturerAndModel("RiverView", "ESP32C6Light");
  zbFanControl.setManufacturerAndModel("RiverView", "ESP32C6Light");
  zbOutlet.setManufacturerAndModel("RiverView", "ESP32C6Light");
  zbHotCold.setManufacturerAndModel("RiverView", "ESP32C6Light");

  Serial.println("1");
  // Set minimum and maximum temperature measurement value
  zbLight1.onLightChange(setLight1);
  zbLight2.onLightChange(setLight2);
  zbFanControl.setFanModeSequence(FAN_MODE_SEQUENCE_LOW_MED_HIGH);
  zbFanControl.onFanModeChange(setFanMode);
  zbOutlet.onPowerOutletChange(setPower);

  zbHotCold.addBinaryOutput();
  zbHotCold.setBinaryOutputApplication(BINARY_OUTPUT_APPLICATION_TYPE_HVAC_FAN);
  zbHotCold.setBinaryOutputDescription("Heat Cool Switch");
  zbHotCold.onBinaryOutputChange(setHotCold);

  Serial.println("2");
  Zigbee.addEndpoint(&zbLight1);
  Zigbee.addEndpoint(&zbLight2);
  Zigbee.addEndpoint(&zbFanControl);
  Zigbee.addEndpoint(&zbOutlet);
  Zigbee.addEndpoint(&zbHotCold);

  Serial.println("3");
  delay(1000);
  // Create a default Zigbee configuration for End Device
  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  Serial.println("Starting Zigbee");
  delay(5000);
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin(&zigbeeConfig, true)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting ESP32!");
    ESP.restart();  // If Zigbee failed to start, reboot the device and try again
  }
  Serial.println("Connecting to network");
  delay(5000);
  while (!Zigbee.connected()) {
    Serial.println("waiting to connect");
    delay(1000);
  }
  Serial.println("Successfully connected to Zigbee network");
  // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
  delay(1000);
  // Call the function to measure temperature and put the device to deep sleep

}

// NOTHING TO DO IN MAIN LOOP ITS ALL CALLBACK BASED SO JUST PRINT STATUS.

void loop() {
   displayPowerStatus();
   displayLightStatus1();
   displayLightStatus2();
   displayFanStatus();
   displayHotColdStatus();
   delay(10000);
}