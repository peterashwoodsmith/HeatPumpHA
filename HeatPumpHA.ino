//
// This ESP32 ARDUINO program is a Zibgee end device that will receive commands to be translated into the IR remote codes for a 
// MISTUBISHI HEATE PUMP. THis is work in progress for a Home Assistant end point that you attach near the IR sensor of the
// Mitsubishi heat pump and that can be battery powered.
//
// The code has two major parts. The first is the IR/Mitsibushi send command that takes all the desired settings as arguments.
// Following that is the zibgee 3.0 Espressif Arduino code to create the end points and their controls (clusters) that select
// the various parameters or the heat pump. This code is just from the HVAC IR Control project:
//       
//     PARTA     https://github.com/r45635/HVAC-IR-Control ,           (c)  Vincent Cruvellier
//     PARTB     https://github.com/peterashwoodsmith/HeatPumpHA       (c)  Peter Ashwood-Smith Dec 2025 
//
// Building this code in Arduino IDE requires setting
//          Tools/USB CDC on boot - enabled (allows serial IO for debugging)
//          Tools/Core debug level (set as desired useful for debugging zibbee attach etc.)
//          Tools/Erase all flash before upload
//          Tools/partition scheme: 4MB with spiffs
//          Tools/zibgee mode ED (end device)
//
#include <esp_task_wdt.h>
#include <nvs_flash.h>

//
// Interrupt service routines for the reset button, must be 5 seconds between the
// depress event and the release event.
//
unsigned isr_resetdepress_t = 0;

//
// Reset button just pressed so record seconds since reboot.
//
void isr_resetButtonPress()
{
     isr_resetdepress_t = (millis() / 1000);
}

//
// Reset button just released so look to see if we have at least 5 seconds elapsed between
// the two. If we do, then we clear the non volatile storage and reboot, otherwise just
// reboot.
//
void isr_resetButtonRelease()
{
     if (isr_resetdepress_t > 0) {
         if ((millis() / 1000) - isr_resetdepress_t > (60 * 5)) {
             nvs_flash_erase();
         }
     }
     ESP.restart();
}

//
// Setup the Interrupt service routing for the reset button. We have a falling and rising to detect press/release
// of this buton.
void isr_setup()
{    
     isr_resetdepress_t = 0;
     const unsigned int resetButton = 11;
     pinMode(resetButton, INPUT_PULLUP);
     attachInterrupt(digitalPinToInterrupt(resetButton), isr_resetButtonPress,   FALLING); // Pullup is grounded it falls
     attachInterrupt(digitalPinToInterrupt(resetButton), isr_resetButtonRelease, RISING);  // Ground released it rises.
}

// PART A
//
// These are the Infra red parameters used to determine intervals and which I/O pin to use to drive the IR board.
// 
int ir_halfPeriodicTime;
int ir_pin;
int ir_khz;

// 
//  IR code used in the Mitsubish IR protocol.
//
#define HVAC_MITSUBISHI_HDR_MARK    3400
#define HVAC_MITSUBISHI_HDR_SPACE   1750
#define HVAC_MITSUBISHI_BIT_MARK    450
#define HVAC_MITSUBISHI_ONE_SPACE   1300
#define HVAC_MISTUBISHI_ZERO_SPACE  420
#define HVAC_MITSUBISHI_RPT_MARK    440
#define HVAC_MITSUBISHI_RPT_SPACE   17100 
//                                      
#define HVAC_MODE_HOT               0             // Enums doing wierd things with compiler, eliminate
#define HVAC_MODE_COLD              1
#define HVAC_MODE_DRY               2
#define HVAC_MODE_FAN               3
#define HVAC_MODE_AUTO              4
//
#define HVAC_FANMODE_SPEED_1        0
#define HVAC_FANMODE_SPEED_2        1
#define HVAC_FANMODE_SPEED_3        2
#define HVAC_FANMODE_SPEED_4        3
#define HVAC_FANMODE_SPEED_5        4
#define HVAC_FANMODE_SPEED_AUTO     5
#define HVAC_FANMODE_SPEED_SILENT   6
//
#define HVAC_VANEMODE_AUTO          0
#define HVAC_VANEMODE_H1            1
#define HVAC_VANEMODE_H2            2
#define HVAC_VANEMODE_H3            3
#define HVAC_VANEMODE_H4            4
#define HVAC_VANEMODE_H5            5
#define HVAC_VANEMODE_AUTO_MOVE     6

//
// Send IR command to Mitsubishi HVAC - ir_sendHvacMitsubishi, this will generate a single 18 byte packet containing the desired
// mode, temperature, fan behaior, vane behavior and on/off setting.
//
void ir_sendHvacMitsubishi(
  int                     HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
  int                     HVAC_Temp,           // Example 21  (°c)
  int                     HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
  int                     HVAC_VaneMode,       // Example VANNE_AUTO_MOVE  HvacMitsubishiVaneMode
  int                     HVAC_powerOff        // Example false
)
{
  byte mask = 1; //our bitmask
  byte data[18] = { 0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06, 0x30, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F };
  // data array is a valid trame, only byte to be chnaged will be updated.
  byte i;
  //
#ifdef HVAC_MITSUBISHI_DEBUG
  Serial.println("Packet to send: ");
  for (i = 0; i < 18; i++) {
    Serial.print("_");
    Serial.print(data[i], HEX);
  }
  Serial.println(".");
#endif
  //
  // Byte 6 - On / Off
  if (HVAC_powerOff) {
    data[5] = (byte) 0x0; // Turn OFF HVAC
  } else {
    data[5] = (byte) 0x20; // Tuen ON HVAC
  }
  //
  // Byte 7 - Mode
  switch (HVAC_Mode)
  {
    case HVAC_MODE_HOT:   data[6] = (byte) 0x08; break;
    case HVAC_MODE_COLD:  data[6] = (byte) 0x18; break;
    case HVAC_MODE_DRY:   data[6] = (byte) 0x10; break;
    case HVAC_MODE_AUTO:  data[6] = (byte) 0x20; break;
    default: break;
  }
  //
  // Byte 8 - Temperature
  // Check Min Max For Hot Mode
  byte Temp;
  if (HVAC_Temp > 31) { Temp = 31;}
  else if (HVAC_Temp < 16) { Temp = 16; } 
  else { Temp = HVAC_Temp; };
  data[7] = (byte) Temp - 16;
  //
  // Byte 10 - FAN / VANNE
  switch (HVAC_FanMode)
  {
    case HVAC_FANMODE_SPEED_1:       data[9] = (byte) B00000001; break;
    case HVAC_FANMODE_SPEED_2:       data[9] = (byte) B00000010; break;
    case HVAC_FANMODE_SPEED_3:       data[9] = (byte) B00000011; break;
    case HVAC_FANMODE_SPEED_4:       data[9] = (byte) B00000100; break;
    case HVAC_FANMODE_SPEED_5:       data[9] = (byte) B00000100; break; //No FAN speed 5 for MITSUBISHI so it is consider as Speed 4
    case HVAC_FANMODE_SPEED_AUTO:    data[9] = (byte) B10000000; break;
    case HVAC_FANMODE_SPEED_SILENT:  data[9] = (byte) B00000101; break;
    default: break;
  }
  //
  switch (HVAC_VaneMode)
  {
    case HVAC_VANEMODE_AUTO:        data[9] = (byte) data[9] | B01000000; break;
    case HVAC_VANEMODE_H1:          data[9] = (byte) data[9] | B01001000; break;
    case HVAC_VANEMODE_H2:          data[9] = (byte) data[9] | B01010000; break;
    case HVAC_VANEMODE_H3:          data[9] = (byte) data[9] | B01011000; break;
    case HVAC_VANEMODE_H4:          data[9] = (byte) data[9] | B01100000; break;
    case HVAC_VANEMODE_H5:          data[9] = (byte) data[9] | B01101000; break;
    case HVAC_VANEMODE_AUTO_MOVE:   data[9] = (byte) data[9] | B01111000; break;
    default: break;
  }
  //
  // Byte 18 - CRC
  data[17] = 0;
  for (i = 0; i < 17; i++) {
    data[17] = (byte) data[i] + data[17];  // CRC is a simple bits addition
  }
  //
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
  //
  ir_space(0);
  for (int j = 0; j < 2; j++) {  // For Mitsubishi IR protocol we have to send two time the packet data
    // Header for the Packet
    ir_mark(HVAC_MITSUBISHI_HDR_MARK);
    ir_space(HVAC_MITSUBISHI_HDR_SPACE);
    for (i = 0; i < 18; i++) {
      // Send all Bits from Byte Data in Reverse Order
      for (mask = 00000001; mask > 0; mask <<= 1) { //iterate through bit mask
        if (data[i] & mask) { // Bit ONE
          ir_mark(HVAC_MITSUBISHI_BIT_MARK);
          ir_space(HVAC_MITSUBISHI_ONE_SPACE);
        }
        else { // Bit ZERO
          ir_mark(HVAC_MITSUBISHI_BIT_MARK);
          ir_space(HVAC_MISTUBISHI_ZERO_SPACE);
        }
        //Next bits
      }
    }
    // End of Packet and retransmission of the Packet
    ir_mark(HVAC_MITSUBISHI_RPT_MARK);
    ir_space(HVAC_MITSUBISHI_RPT_SPACE);
    ir_space(0); // Just to be sure
  }
}

//
// send an ir_mark ( int time ) 
// 
// Sends an IR ir_mark for the specified number of microseconds.
// The ir_mark output is modulated at the PWM frequency.
//
void ir_mark(int time) {
  long beginning = micros();
  while(micros() - beginning < time){
    digitalWrite(ir_pin, HIGH);
    delayMicroseconds(ir_halfPeriodicTime);
    digitalWrite(ir_pin, LOW);
    delayMicroseconds(ir_halfPeriodicTime); //38 kHz -> T = 26.31 microsec (periodic time), half of it is 13
  }
}

//
// ir_space ( int time) 
// Leave pin off for time (given in microseconds) 
// Sends an IR ir_space for the specified number of microseconds.
// A ir_space is no output, so the PWM output is disabled.
//
void ir_space(int time) {
  digitalWrite(ir_pin, LOW);
  if (time > 0) delayMicroseconds(time);
}

//
// Setup the variables and pins for use the the code above to send to the IR board.
// Only thing you may need to change here is what ir_pin you want to use.
//
void ir_setup() {
  // put your setup code here, to run once:
  ir_pin=10;
  ir_khz=38;
  ir_halfPeriodicTime = 500/ir_khz;
  pinMode(ir_pin, OUTPUT);
  Serial.println("ir_setup done");
}

//
// PART B - Z I G B E E / Home Assistant interface
//
#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"


// 
// These are the EP control entities each has one cluser which is a 'knob' that controls an HVAC parameter
//
ZigbeePowerOutlet zbOutlet      = ZigbeePowerOutlet(10);    // Power on/off knob
ZigbeeBinary      zbColdHot     = ZigbeeBinary(11);         // Heat or cooling knob
ZigbeeAnalog      zbTemp        = ZigbeeAnalog(12);         // Desired temperature slider
ZigbeeAnalog      zbFanControl  = ZigbeeAnalog(13);         // How the fans operate slider
ZigbeeAnalog      zbVaneControl = ZigbeeAnalog(14);         // Van motion/positions slider

//
// These are the variables that maintain the state of what HA has asked to be set
// set.
//
bool              ha_powerStatus   = 0;    // powered on/off
bool              ha_coldHotStatus = 0;    // heating or cooling mode
unsigned int      ha_fanStatus     = 0;    // fan position or movement
unsigned int      ha_tempStatus    = 0;    // desired temperature
unsigned int      ha_vaneStatus    = 0;    // how the vanes move or don't

// NVS Realted stuff
const char       *ha_nvs_name = "_HeatPumpHA_NVS";   // Unique name for our partition
const char       *ha_nvs_vname= "_vars_";            // name for our packeed variables
nvs_handle_t      ha_nvs_handle;                     // Once open this is read/write to NVS

//
// We are looking for persistant values of the ha_variables above. So we open the Non Volatile Storage
// and try to read them, if we find them they are packed into a UINT32 so we unpack them into our
// global ha_xxx variables and continue. We later will write these when every they change via HA.
// Format is just one attribute per nibble, more than enough room for all the different values in a 
// single UINT32.
//
void ha_nvs_read()
{
     esp_err_t err = nvs_flash_init();
     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
     }
     ESP_ERROR_CHECK(err);
     err = nvs_open(ha_nvs_name, NVS_READWRITE, &ha_nvs_handle);
     if (err != ESP_OK) {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }
    uint32_t vars = 0;  
    err = nvs_get_u32(ha_nvs_handle, ha_nvs_vname, &vars);
    if (err == ESP_ERR_NVS_NOT_FOUND)
        Serial.printf("Vars %s not found\n", ha_nvs_vname);
    else if (err != ESP_OK)
        Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    else {
        Serial.printf("Vars %s found = %x\n", ha_nvs_vname, vars);
        // Trivial encoding format - just one nibble per attribute.
        ha_powerStatus   =  vars & 0xf; vars >>= 4;
        ha_coldHotStatus =  vars & 0xf; vars >>= 4;
        ha_fanStatus     =  vars & 0xf; vars >>= 4; 
        ha_vaneStatus    =  vars & 0xf; vars >>= 4;
        ha_tempStatus    =  vars & 0x1f;                     // Need 5 bits for temp
        Serial.printf("Unpacked vars: pow=%d hot/cld=%d fan=%d temp=%d vane=%d\n",
                       ha_powerStatus, ha_coldHotStatus, ha_fanStatus, ha_tempStatus, ha_vaneStatus);
    }
}

//
// And here is the write to NVS of the attributes after they have been changed and sent to the Heat Pump
//
void ha_nvs_write()
{
     uint32_t vars  = ha_tempStatus    & 0x1f; vars <<= 5;   // Need 5 bites for temp
              vars |= ha_vaneStatus    & 0xf;  vars <<= 4;
              vars |= ha_fanStatus     & 0xf;  vars <<= 4;
              vars |= ha_coldHotStatus & 0xf;  vars <<= 4;
              vars |= ha_powerStatus   & 0xf; 
     //
     Serial.printf("Unpacked vars: pow=%d hot/cld=%d fan=%d temp=%d vane=%d\n",
                       ha_powerStatus, ha_coldHotStatus, ha_fanStatus, ha_tempStatus, ha_vaneStatus);   
     Serial.printf("Packed = %x\n", vars);
     //
     esp_err_t err = nvs_set_u32(ha_nvs_handle, ha_nvs_vname, vars);
     if (err != ESP_OK) {
         Serial.printf("Vars %s can't write, because %s\n", ha_nvs_vname, esp_err_to_name(err));
         return;
     }
     err = nvs_commit(ha_nvs_handle);
     if (err != ESP_OK) {
         Serial.printf("Vars %s can't commit, because %s\n", ha_nvs_vname, esp_err_to_name(err));
     }  
}

//
// Send the Heat Pump the proper commands to synchronize with what HA as asked. Basically convert from the above ha_ variables
// to corresponding hv_ variables and call the send function which will encode the 18 byte frame to the IR transmitter.
//
void ha_syncHeadPump()
{
     int hv_mode;
     switch(ha_coldHotStatus) {
         case 1: hv_mode = HVAC_MODE_HOT;  break;
         case 0: hv_mode = HVAC_MODE_COLD; break;
         default:
                 Serial.printf("Invalid power status =% d\n", ha_coldHotStatus);
                 return;
     }
     //
     int hv_powerOff  = ha_powerStatus ? 0 : 1;        // its an off flag to the UI so reversed from HA.
     int hv_temp      = ha_tempStatus;
     int hv_fanMode   = ha_fanStatus;
     int hv_vanneMode = ha_vaneStatus;
     //
     Serial.printf("*** SEND HVAC COMMAND: mode=%d, temp=%d, fan=%d, vane=%d, off=%d ***\n",
                   hv_mode, hv_temp, hv_fanMode, hv_vanneMode, hv_powerOff);
     ir_sendHvacMitsubishi(hv_mode, hv_temp, hv_fanMode, hv_vanneMode, hv_powerOff);
}

//
// These are just useful debugging functions to display the attributes that HA has given us.
// One for each attributes.
//
void ha_displayPowerStatus()
{
     Serial.println(ha_powerStatus ? "POWER ON" : "POWER OFF"); 
}
//
void ha_displayFanStatus()
{     
     Serial.printf("FAN STATUS = %d\n", ha_fanStatus);
}
//
void ha_displayVaneStatus()
{
     if ((ha_vaneStatus < 0)||(ha_vaneStatus > 6))
        Serial.printf("VANE STATUS = %d ????\n", ha_vaneStatus);
     else
        Serial.printf("VANE STATUS = %d\n", ha_vaneStatus);
}
//
void ha_displayColdHotStatus()
{    
     Serial.printf("COOL or HEAT STATUS = %s (%x)\n", ha_coldHotStatus ? "HEAT" : "COOL", (unsigned int) ha_coldHotStatus);
}
//
void ha_displayTempStatus()
{    
     Serial.printf("TEMP STATUS = %d\n", ha_tempStatus);
}

//
// This is just a little schedule time for when to do a sync. When we get attribute changes we schedule and update 5s in the future.
// THis just allows us to aggregate multiple quick arriving atribute changes.
//
unsigned long ha_update_t = 0;      // not sure about callback thread if its this or different thread.

//
// These are the callback functions that set each of the attributes. When HA makes a change to the attribute the zibgee library
// will call these so we can record the desired setting. Note that we do not immediately try to send the IR packet to the HVAC
// rather we will wait a bit to make sure that if the user changes a few attributes in a few seconds can just send one command and
// save a little bit of power. One set function per attribute follows.
//
void ha_setFan(float value)   
{
     ha_fanStatus = value;
     Serial.print("HA=> "); ha_displayFanStatus();
     ha_update_t = millis() + 5000;
}
//
void ha_setPower(bool value)
{
     ha_powerStatus = value;
     Serial.print("HA=> "); ha_displayPowerStatus();
     ha_update_t = millis() + 5000;
}
//
void ha_setColdHot(bool state)
{
     ha_coldHotStatus = state;
     Serial.print("HA=> "); ha_displayColdHotStatus();
     ha_update_t = millis() + 5000;
}
//
void ha_setTemp(float value)
{    ha_tempStatus = value;
     Serial.print("HA=> "); ha_displayTempStatus();
     ha_update_t = millis() + 5000;
}
//
void ha_setVane(float value)
{
     ha_vaneStatus = value;
     Serial.print("HA=> "); ha_displayVaneStatus();
     ha_update_t = millis() + 5000;
}

// BASIC ARDUINO SETUP

void setup() {
     // 
     // If we don't get watch dog resets each 5 minutes then reboot.
     //
     //esp_task_wdt_config_t wdt_config = { 1000*60*60*5, (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1, true };
     //esp_task_wdt_init(&wdt_config);
     //esp_task_wdt_add(NULL);
     //
     // Debug stuff
     //
     Serial.begin(115200);
     delay(100);
     Serial.println("RiverView zigbee up");
     //
     // Bring up the IR interface & enable interrupts for reset buttons.
     //
     ir_setup();
     isr_setup();
     //
     // Read all the Non volatile variables from last boot.
     //
     ha_nvs_read();      
     //
     // Add the zibgee clusters (buttons/sliders etc.)
     //
     Serial.println("On of Power switch cluster");
     zbOutlet.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbOutlet.onPowerOutletChange(ha_setPower);
     //
     Serial.println("Cold/Hot Switch cluster");
     zbColdHot.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbColdHot.addBinaryOutput();
     zbColdHot.setBinaryOutputApplication(BINARY_OUTPUT_APPLICATION_TYPE_HVAC_OTHER);
     zbColdHot.setBinaryOutputDescription("Cool => Heat");
     zbColdHot.onBinaryOutputChange(ha_setColdHot);
     //
     Serial.println("Temp Selector cluster");
     zbTemp.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbTemp.addAnalogOutput();
     zbTemp.setAnalogOutputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
     zbTemp.setAnalogOutputDescription("Temperature C");
     zbTemp.setAnalogOutputResolution(1);
     zbTemp.setAnalogOutputMinMax(0, 30); 
     zbTemp.onAnalogOutputChange(ha_setTemp);
     //
     Serial.println("Fan Selector cluster");
     zbFanControl.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbFanControl.addAnalogOutput();
     zbFanControl.setAnalogOutputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
     zbFanControl.setAnalogOutputDescription("Fan 0-4 (5-auto, 6-silent)");
     zbFanControl.setAnalogOutputResolution(1);
     zbFanControl.setAnalogOutputMinMax(0, 7);  
     zbFanControl.onAnalogOutputChange(ha_setFan);
     //
     Serial.println("Vane Selector cluster");
     zbVaneControl.setManufacturerAndModel("RiverView", "ESP32C6Light");
     zbVaneControl.addAnalogOutput();
     zbVaneControl.setAnalogOutputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
     zbVaneControl.setAnalogOutputDescription("Vane (0=Auto,1,2,3,4,5,6=move);");
     zbVaneControl.setAnalogOutputResolution(1);
     zbVaneControl.setAnalogOutputMinMax(0, 6);  
     zbVaneControl.onAnalogOutputChange(ha_setVane);
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
     delay(5000);       // Seems necessary or it connects without connecting
     //
     Serial.println("Connecting to network");         
     while (!Zigbee.connected()) {
        Serial.println("connectint..\n");
        delay(1000);
     }
     Serial.println("Successfully connected to Zigbee network");
     // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
     delay(1000);
}

// NOTHING TO DO IN MAIN LOOP ITS ALL CALLBACK BASED SO JUST PRINT STATUS.

void loop() {
     //esp_task_wdt_reset();     // All ok
     //
     Serial.printf("----------------------------- loop ------------------------------------\n");
     ha_displayPowerStatus();
     ha_displayColdHotStatus();
     ha_displayTempStatus();
     ha_displayFanStatus();
     ha_displayVaneStatus();
     delay(10000);
     //
     // If synch required then send the IR now. Wonder if there is problem with mutual exclusion and
     // callback functions. Are they in same thread? If not this variable is volatile.
     //
     if ((ha_update_t > 0) && (millis() > ha_update_t)) {
         ha_update_t = 0;
         Serial.println("Synch with HVAC required\n");
         ha_syncHeadPump();
         ha_nvs_write(); 
     }
}
