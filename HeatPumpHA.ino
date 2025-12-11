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

// 
// These are the EP control entities.
//
ZigbeePowerOutlet zbOutlet      = ZigbeePowerOutlet(10);    // Power on/off
ZigbeeBinary      zbColdHot     = ZigbeeBinary(11);         // Heat or cooling
ZigbeeAnalog      zbTemp        = ZigbeeAnalog(12);         // Desired temperature
ZigbeeAnalog      zbFanControl  = ZigbeeAnalog(13);         // How the fans operate
ZigbeeAnalog      zbVaneControl = ZigbeeAnalog(14);         // Van motion/positions

//
// These are the variables that maintain the state of what HA as asked to be
// set.
//
int               ha_fanStatus     = 0;
bool              ha_powerStatus   = 0;
bool              ha_coldHotStatus = 0;
int               ha_tempStatus    = 0;
int               ha_vaneStatus    = 0;

//
// Send the Heat Pump the proper commands to synchronize with what HA as asked. Basically convert from the above ha_ variables
// to corresponding hv_ variables and call the send function.
//
void syncHeadPump()
{
    HvacMode hv_mode;
    switch(ha_coldHotStatus) {
         case 1: hv_mode = HVAC_HOT;  break;
         case 0: hv_mode = HVAC_COLD; break;
         default:
                 Serial.printf("Invalid power status =% d\n", ha_coldHotStatus);
                 return;
    }
    //
    int           hv_powerOff  = ha_powerStatus ? 1 : 0;
    int           hv_temp      = ha_tempStatus;
    HvacFanMode   hv_fanMode   = (HvacFanMode) ha_fanStatus;
    HvacVanneMode hv_vanneMode = (HvacVanneMode) ha_vaneStatus;
    //
     Serial.printf("*** SEND HVAC COMMAND: mode=%d, temp=%d, fan=%d, vane=%d, off=%d ***\n",
                   hv_mode, hv_temp, hv_fanMode, hv_vanneMode, hv_powerOff);
     sendHvacMitsubishi(hv_mode, hv_temp, hv_fanMode, hv_vanneMode, hv_powerOff);
}

void displayPowerStatus()
{
     Serial.println(ha_powerStatus ? "POWER ON" : "POWER OFF"); 
}

void displayFanStatus()
{     
     Serial.printf("FAN STATUS = %d\n", ha_fanStatus);
}

void displayVaneStatus()
{
     if ((ha_vaneStatus < 0)||(ha_vaneStatus > 6))
        Serial.printf("VANE STATUS = %d ????\n", ha_vaneStatus);
     else
        Serial.printf("VANE STATUS = %d\n", ha_vaneStatus);
}

void displayColdHotStatus()
{    
     Serial.printf("COOL or HEAT STATUS = %s (%x)\n", ha_coldHotStatus ? "HEAT" : "COOL", (unsigned int) ha_coldHotStatus);
}

void displayTempStatus()
{    
     Serial.printf("TEMP STATUS = %d\n", ha_tempStatus);
}

void setFan(float value)   
{
     ha_fanStatus = value;
     Serial.print("HA=> "); displayFanStatus();
}

void setPower(bool value)
{
     ha_powerStatus = value;
     Serial.print("HA=> "); displayPowerStatus();
}

void setColdHot(bool state)
{
     ha_coldHotStatus = state;
     Serial.print("HA=> "); displayColdHotStatus();
}

void setTemp(float value)
{    ha_tempStatus = value;
     Serial.print("HA=> "); displayTempStatus();
}

void setVane(float value)
{
     ha_vaneStatus = value;
     Serial.print("HA=> "); displayVaneStatus();
}

// BASIC ARDUINO SETUP

void setup() {
     Serial.begin(115200);
     delay(100);
     //
     Serial.println("MITS IR setup");
     setupIR();
     //
     Serial.println("RiverView Zigbee");
     Serial.println("On of Power switch cluster");
     zbOutlet.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbOutlet.onPowerOutletChange(setPower);
     //
     Serial.println("Cold/Hot Switch cluster");
     zbColdHot.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbColdHot.addBinaryOutput();
     zbColdHot.setBinaryOutputApplication(BINARY_OUTPUT_APPLICATION_TYPE_HVAC_OTHER);
     zbColdHot.setBinaryOutputDescription("Cool => Heat");
     zbColdHot.onBinaryOutputChange(setColdHot);
     //
     Serial.println("Temp Selector cluster");
     zbTemp.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbTemp.addAnalogOutput();
     zbTemp.setAnalogOutputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
     zbTemp.setAnalogOutputDescription("Temperature C");
     zbTemp.setAnalogOutputResolution(1);
     zbTemp.setAnalogOutputMinMax(0, 30); 
     zbTemp.onAnalogOutputChange(setTemp);
     //
     Serial.println("Fan Selector cluster");
     zbFanControl.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbFanControl.addAnalogOutput();
     zbFanControl.setAnalogOutputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
     zbFanControl.setAnalogOutputDescription("Fan 0-4 (5-auto, 6-silent)");
     zbFanControl.setAnalogOutputResolution(1);
     zbFanControl.setAnalogOutputMinMax(0, 7);  
     zbFanControl.onAnalogOutputChange(setFan);
     //
     Serial.println("Vane Selector cluster");
     zbVaneControl.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbVaneControl.addAnalogOutput();
     zbVaneControl.setAnalogOutputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
     zbVaneControl.setAnalogOutputDescription("Vane (0=Auto,1,2,3,4,5,6=move);");
     zbVaneControl.setAnalogOutputResolution(1);
     zbVaneControl.setAnalogOutputMinMax(0, 6);  
     zbVaneControl.onAnalogOutputChange(setVane);
     //
     Serial.println("Set battery power");
     //
     zbOutlet.setPowerSource(ZB_POWER_SOURCE_BATTERY, 95, 37);  
     //
     Zigbee.addEndpoint(&zbOutlet);
     Zigbee.addEndpoint(&zbColdHot);
     Zigbee.addEndpoint(&zbTemp);
     Zigbee.addEndpoint(&zbFanControl);
     Zigbee.addEndpoint(&zbVaneControl);
     //
     delay(1000);
     // Create a default Zigbee configuration for End Device
     esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
     Serial.println("Starting Zigbee");
     delay(1000);
     // When all EPs are registered, start Zigbee in End Device mode
     if (!Zigbee.begin(&zigbeeConfig, false)) {
        Serial.println("Zigbee failed to start!");
        Serial.println("Rebooting ESP32!");
        ESP.restart();  // If Zigbee failed to start, reboot the device and try again
     }
     delay(1000);       // Seems necessary or it connects without connecting
     //
     Serial.println("Connecting to network");         
     while (!Zigbee.connected()) {
        Serial.println("connectint..\n");
        delay(1000);
     }
     Serial.println("Successfully connected to Zigbee network");
     // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
     delay(1000);
     //
     ha_powerStatus   = zbOutlet.getPowerOutletState();     // These don't seem to help
     ha_coldHotStatus = zbColdHot.getBinaryOutput();
     ha_tempStatus    = zbTemp.getAnalogOutput();
     ha_fanStatus     = zbFanControl.getAnalogOutput();
     ha_vaneStatus    = zbVaneControl.getAnalogOutput();
}

// NOTHING TO DO IN MAIN LOOP ITS ALL CALLBACK BASED SO JUST PRINT STATUS.

void loop() {
     Serial.printf("----------------------------- loop ------------------------------------\n");
     displayPowerStatus();
     displayColdHotStatus();
     displayTempStatus();
     displayFanStatus();
     displayVaneStatus();
     delay(10000);
}
