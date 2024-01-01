/**
 * Example use of PStruct (persistent storage structure classes).
 * In this example we attempt to load a user defined ADT but if that fails we use default values.  The ADT is printed 
 * to serial and a counter field is then incremented with save attempted.  Next time the program boots the process 
 * repeats, each time persistent storage content should change.
 *
 * Architecture support:
 *  AVR8 (Uno/Nano)
 *  STM32 (F103.  Blue pill/Maple mini)
 *
 * Dave.C, 2019
 */

// include media storage access base class and user api
#include "media.h"
#include "struct.h"

#if defined(ARDUINO_ARCH_STM32)

// Include chip specific media wrapper for flash.  this wrapper could for be an off chip flash device
#include "stm32/f103/wrap.h"

// maple mini default board led
#define BOARD_LED_PIN 33

#elif defined(ARDUINO_ARCH_AVR)

// Include chip specific media wrapper for flash.  this wrapper could for be an off chip flash device
#include "mega/flash.h"
#include "mega/wrap.h"

// UNO default board led
#define BOARD_LED_PIN 13

#else
#error Unsupported core
#endif


/**
 * Application specific macro stating how many storage copies are to be used.  this essential controls wear levelling as a 
 * higher value will use more storage over time but increases endurance and therefore your applications useful life.  If 
 * you know you are likely to perform a lot of writes, increase the value to suit.  You may need to consult at your devices
 * flash module manufacturer datasheet/reference manual in order to correctly size.
 */
#define PSTRUCT_USE_N_WARELEVELS  5


/**
 * Application specific ADT, you define this to suit your data requirements.  it is written to and read from persistent storage
 * and you will access your data via this type.  Most likely this will be a configuration structure of some form.
 *
 * @note packing not important whatever gives best optimisation either for access by cpu (typically padded) or storage (not padded)
 */
#pragma pack(push,1)
typedef struct {
  uint32_t value1;
  uint16_t value2;
  char  str[7];  // C array size ensures structure not a division of sizeof(uint32_t) or sizeof(uint16_t)
}config_t;
#pragma pack(pop)


/**
 * Create your media access instance.  This wrapper class allows you to customise where your persistent storage resides.  It is
 * normally via on-chip flash but could be some place else should you decide to write your own.
 */
wrap::Flash g_media;


/**
 * Flash allocation by compiler rather than specifying an address that may or may not be used.  This declaration will allocate
 * enough space, aligned by page so it can be safely erased and programmed without affecting the rest of program code.  Doesn't 
 * matter what you fill with as it won't get loaded correctly and defaults will be written first time executing
 */
#if defined(ARDUINO_ARCH_AVR)
const uint8_t data[PERSISTSTRUCT_SIZE(config_t,AVR_FLASH_PAGE_SIZE,PSTRUCT_USE_N_WARELEVELS)] PROGMEM __attribute__ ((aligned (AVR_FLASH_PAGE_SIZE))) = { 0xff };
#elif defined(ARDUINO_ARCH_STM32)
const uint8_t data[PERSISTSTRUCT_SIZE(config_t,STM32F103X_FLASH_PAGE_SIZE,PSTRUCT_USE_N_WARELEVELS)] __attribute__ ((aligned (STM32F103X_FLASH_PAGE_SIZE))) = { 0xff };
#endif

/**
 * Use config_t and g_media for persistent storage, n pages worth.  use this instance to update media with your structure changes
 * or to simple load your structure.  Here we just stick the data manually at the end of storage
 */
persist::Struct<config_t> g_cfg_ps(g_media, reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(&data[0])), PSTRUCT_USE_N_WARELEVELS);

/**
 * Your application ADT.  Use this to access your data, defaults optional and depends upon what you do with save(...).  if using PERSISTSTRUCT_POINTERS 
 * then you'll have to write these before invoking save(...) because failed load would be using the same memory
 */
config_t g_cfg = {
    .value1 = 0x0c0ffee0,
    .value2 = 0x1234,
    "Heidi\0"
};


void setup() {
  Serial.begin(9600);
  while(!Serial) { }

  Serial.print("Loading config from ");
  Serial.println("flash");
  Serial.print("Raw storage start 0x");
  Serial.print(reinterpret_cast<uint32_t>(&data[0]), HEX);
  Serial.print(", size ");
  Serial.println(static_cast<uint32_t>(sizeof(data)), DEC);
  Serial.println("Bytes");
  Serial.print(g_cfg_ps.GetWareLevels(), DEC);
  Serial.println(" Ware levels\n");

  // attempt to load user ADT from media
  if (g_cfg_ps.Load(g_cfg)) {
    // ADT loaded.  now you can access your data
    Serial.println("Loaded");
  }else {
    // ADT load fail.  assume we have never written it and setup default here.  we should really only run this code once on a new device
    // NOTE: the way PStruct has been written even a worn out device should get here so long as one valid structure exists
    Serial.println("New or worn out device.  Using default settings");
    
    // save defaults in ADT to media with attempt to find next position just use start of media for next write
    if (g_cfg_ps.Save(g_cfg,true)) {
      Serial.println("Write success");
    }else {
      Serial.println("Write failed.  assume device worn out!");
    }
  }

  // print out ADT
  Serial.print("ADT dump @ 0x");
#if defined(ARDUINO_ARCH_STM32)
  Serial.println(reinterpret_cast<uint32_t>(g_cfg_ps.GetLocation()), HEX);
#elif defined(ARDUINO_ARCH_AVR)
  Serial.println(reinterpret_cast<uint16_t>(g_cfg_ps.GetLocation()), HEX);
#endif
  Serial.println("---------------");
  Serial.print("g_cfg.value1 = 0x");
  Serial.println(g_cfg.value1, HEX);
  Serial.print("g_cfg.value2 = 0x");
  Serial.println(g_cfg.value2, HEX);
  Serial.print("g_cfg.str = ");
  Serial.println(g_cfg.str);

  g_cfg.value2++;
  
  if (g_cfg_ps.Save(g_cfg)) {
    Serial.println("Update ok");
  }else {
    Serial.println("Update failed");
  }
  Serial.println("Done");

  pinMode(BOARD_LED_PIN, OUTPUT);
  for(;;) {
#ifdef GIGGLERS_SERIAL
    /**
     * https://github.com/gigglerg/Arduino_Core_STM32_Maple_Support
     * known issues
     */
    Serial.flush();
#endif
    digitalWrite(BOARD_LED_PIN, 0);
    delay(100);
    digitalWrite(BOARD_LED_PIN, 1);
    delay(200);
  }
}

void loop() {
}