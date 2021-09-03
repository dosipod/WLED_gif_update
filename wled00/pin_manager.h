#ifndef WLED_PIN_MANAGER_H
#define WLED_PIN_MANAGER_H
/*
 * Registers pins so there is no attempt for two interfaces to use the same pin
 */
#include <Arduino.h>
#include "const.h" // for USERMOD_* values, total gpio count

typedef struct PinManagerPinType {
  int8_t  pin {-1};
  uint8_t isOutput {true};
  PinManagerPinType(int8_t pin, uint8_t isOutput) : pin(pin), isOutput(isOutput) {}
  PinManagerPinType(int8_t pin) : pin(pin), isOutput(true) {}
  PinManagerPinType() : pin(-1), isOutput(true) {}
} managed_pin_type;

/*
 * Allows PinManager to "lock" an allocation to a specific
 * owner, so someone else doesn't accidentally de-allocate
 * a pin it hasn't allocated.  Also enhances debugging.
 * 
 * RAM Cost:
 *     17 bytes on ESP8266
 *     40 bytes on ESP32
 */
enum struct PinOwner : uint8_t {
  None          = 0,            // default == legacy == unspecified owner
  // High bit is set for all built-in pin owners
  // StatusLED  -- THIS SHOULD NEVER BE ALLOCATED -- see handleStatusLED()
  Ethernet      = 0x81,
  BusDigital    = 0x82,
  BusDigital2   = 0x83,
  BusPwm        = 0x84,   // 'BusP' == PWM output using BusPwm
  Button        = 0x85,   // 'Butn' == button from configuration
  IR            = 0x86,   // 'IR'   == IR receiver pin from configuration
  Relay         = 0x87,   // 'Rly'  == Relay pin from configuration
  SPI_RAM       = 0x88,   // 'SpiR' == SPI RAM
  DebugOut      = 0x89,   // 'Dbg'  == debug output always IO1
  DMX           = 0x8A,   // 'DMX'  == hard-coded to IO2
  // Use UserMod IDs from const.h here
  UM_Unspecified       = USERMOD_ID_UNSPECIFIED,        // 0x01
  UM_RGBRotaryEncoder  = USERMOD_ID_UNSPECIFIED,        // 0x01 // No define in const.h for this user module -- consider adding?
  UM_Example           = USERMOD_ID_EXAMPLE,            // 0x02 // Usermod "usermod_v2_example.h"
  UM_Temperature       = USERMOD_ID_TEMPERATURE,        // 0x03 // Usermod "usermod_temperature.h"
  // #define USERMOD_ID_FIXNETSERVICES                  // 0x04 // Usermod "usermod_Fix_unreachable_netservices.h" -- Does not allocate pins
  UM_PIR               = USERMOD_ID_PIRSWITCH,          // 0x05 // Usermod "usermod_PIR_sensor_switch.h"
  // #define USERMOD_ID_IMU                             // 0x06 // Usermod "usermod_mpu6050_imu.h" -- Uses "standard" I2C pins ... TODO -- enable shared I2C bus use
  UM_FourLineDisplay   = USERMOD_ID_FOUR_LINE_DISP,     // 0x07 // Usermod "usermod_v2_four_line_display.h
  UM_RotaryEncoderUI   = USERMOD_ID_ROTARY_ENC_UI,      // 0x08 // Usermod "usermod_v2_rotary_encoder_ui.h"
  // #define USERMOD_ID_AUTO_SAVE                       // 0x09 // Usermod "usermod_v2_auto_save.h" -- Does not allocate pins
  // #define USERMOD_ID_DHT                             // 0x0A // Usermod "usermod_dht.h" -- Statically allocates pins, not compatible with pinManager?
  // #define USERMOD_ID_MODE_SORT                       // 0x0B // Usermod "usermod_v2_mode_sort.h" -- Does not allocate pins
  // #define USERMOD_ID_VL53L0X                         // 0x0C // Usermod "usermod_vl53l0x_gestures.h" -- Uses "standard" I2C pins ... TODO -- enable shared I2C bus use
  UM_MultiRelay        = USERMOD_ID_MULTI_RELAY,        // 0x0D // Usermod "usermod_multi_relay.h"
  UM_AnimatedStaircase = USERMOD_ID_ANIMATED_STAIRCASE, // 0x0E // Usermod "Animated_Staircase.h"
  // #define USERMOD_ID_RTC                             // 0x0F // Usermod "usermod_rtc.h" -- Uses "standard" I2C pins ... TODO -- enable shared I2C bus use
  // #define USERMOD_ID_ELEKSTUBE_IPS                   // 0x10 // Usermod "usermod_elekstube_ips.h" -- Uses quite a few pins ... see Hardware.h and User_Setup.h
  // #define USERMOD_ID_SN_PHOTORESISTOR                // 0x11 // Usermod "usermod_sn_photoresistor.h" -- Uses hard-coded pin (PHOTORESISTOR_PIN == A0), but could be easily updated to use pinManager
};
static_assert(0u == static_cast<uint8_t>(PinOwner::None), "PinOwner::None must be zero, so default array initialization works as expected");

// wled00/cfg.cpp:734:22:   required from here
// wled00/src/dependencies/json/ArduinoJson-v6.h:4115:25:
// error: no matching function for call to 'convertToJson(const PinOwner&, ArduinoJson6181_90::VariantRef&)'
//      return convertToJson(src, dst); // Error here? See https://arduinojson.org/v6/unsupported-set/
//
// PROBLEM: Header problems if I try to include headers to have the ArduinoJson::VariantRef type defined here.
//   CHEAT: Use a template to avoid this, as it won't be instantiated until later, where it's used and the
//          ArduinoJson::VariantRef type is already defined.
template <typename T>
bool convertToJson(const PinOwner src, /*ArduinoJson*/ T dst) {
  static_assert(sizeof(int) > sizeof(uint8_t), "PinOwner: Review convertToJson() for potential truncation");
  return dst.set(static_cast<const int>(src));
}

class PinManagerClass {
  private:
  static constexpr const size_t BytesForPinAlloc = (TOTAL_GPIO_COUNT / 8) + ((TOTAL_GPIO_COUNT % 8) ? 1 : 0);
  uint8_t pinAlloc[BytesForPinAlloc] {};  // 1 bit per pin
  PinOwner ownerTag[TOTAL_GPIO_COUNT] {}; // zero-init == PinOwner::None; verified by static_assert above

  #if defined(ARDUINO_ARCH_ESP32) // this is defined for both ESP32 and ESP32S2
  static_assert(BytesForPinAlloc == 5, "Change in bytes for tracking pin allocation");
  uint8_t ledcAlloc[2] = {0x00, 0x00}; //16 LEDC channels
  #endif

  public:
  // De-allocates a single pin
  bool deallocatePin(byte gpio, PinOwner tag);
  // Allocates a single pin, with an owner tag.
  // De-allocation requires the same owner tag (or override)
  bool allocatePin(byte gpio, bool output, PinOwner tag);
  // Allocates all the pins, or allocates none of the pins, with owner tag.
  // Provided to simplify error condition handling in clients 
  // using more than one pin, such as I2C, SPI, rotary encoders,
  // ethernet, etc..
  bool allocateMultiplePins(const managed_pin_type * mptArray, byte arrayElementCount, PinOwner tag );
  // Per request, exposing pin ownership so can output via JSON/XML
  PinOwner currentPinOwner(byte gpio);

  #if !defined(ESP8266) // ESP8266 compiler doesn't understand deprecated attribute
  [[deprecated("Replaced by three-parameter allocatePin(gpio, output, ownerTag), for improved debugging")]]
  #endif
  inline bool allocatePin(byte gpio, bool output = true) { return allocatePin(gpio, output, PinOwner::None); }
  #if !defined(ESP8266) // ESP8266 compiler doesn't understand deprecated attribute
  [[deprecated("Replaced by three-parameter deallocatePin(gpio, output, ownerTag), for improved debugging")]]
  #endif
  inline void deallocatePin(byte gpio) { deallocatePin(gpio, PinOwner::None); }

  bool isPinAllocated(byte gpio, PinOwner tag = PinOwner::None);
  bool isPinOk(byte gpio, bool output = true);

  #ifdef ARDUINO_ARCH_ESP32
  byte allocateLedc(byte channels);
  void deallocateLedc(byte pos, byte channels);
  #endif
};

extern PinManagerClass pinManager;
#endif
