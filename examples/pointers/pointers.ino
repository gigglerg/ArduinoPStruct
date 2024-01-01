/**
 * Example use of PStruct (persistent storage structure classes). In this example we store a user defined ADT (once) and then 
 * repeatedly loads it upon reboots.
 *
 * Architecture support:
 *  AVR8 (Uno/Nano)
 *  STM32 (F103.  Blue pill/Maple mini)
 *
 * Dave.C, 2019
 */

// PStruct use pointers for ADTs and give access to internal data.  In some situations it can reduce code footprint
#define PERSISTSTRUCT_POINTERS

#if defined(ARDUINO_ARCH_STM32)

// Include media storage access base class and user api
#include "media.h"
#include "struct.h"

// Include chip specific media wrapper for flash
#include "stm32/f103/wrap.h"

#elif defined(ARDUINO_ARCH_AVR)

// Include media storage access base class and user api
#include "media.h"
#include "struct.h"

// Include chip specific media wrapper
#include "mega/flash.h"
#include "mega/wrap.h"

#else
#error Unsupported core
#endif

/**
 * Application specific macro stating how many storage pages are to be used.  this essential controls wear levelling as a 
 * higher value will use more storage over time but increases endurance and therefore your applications useful life.  if 
 * you know you are likely to perform a lot of writes, increase the value to suit.  you may need to look at your devices
 * flash module in manufacturer datasheet/reference manual in order to correctly size.
 */
#define PSTRUCT_USE_N_PAGES        10


/**
 * Application specific ADT, you define this to suit your data requirements.  It is written to and read from persistent storage
 * and you will access your data via this type.  most likely this will be a configuration structure of some form.
 */
typedef struct {
    uint32_t value1;
    uint16_t value2;
    char  str[12];    // C array size ensures ensures structure not a division of sizeof(uint32_t)
}cfg_t;


/**
 * Create your media access instance.  this wrapper class allows you to customise where your persistent storage resides.  it is
 * normally via on-chip flash but could be some place else should you decide to write your own.
 */
wrap::Flash g_media;


/**
 * Use cfg_t and g_media for persistent storage, n pages worth.  Use this instance to update media with your structure changes
 * or to simple load your structure.
 */
persist::Struct<cfg_t> g_cfg_ps(g_media, g_media.GetEnd() - (g_media.GetPageSize() * PSTRUCT_USE_N_PAGES), PSTRUCT_USE_N_PAGES);


/**
 * Your application ADT.  use this to access your data
 */
cfg_t* gp_cfg = g_cfg_ps.Get();  // Always same ram location internally so get once


void setup() {
    Serial.begin(9600);
    while(!Serial) { }
    
    Serial.println("setup()");

    // Attempt to load user ADT from media
    if (g_cfg_ps.Load()) {
        // ADT loaded.  now you can access your data
    }else {
        // ADT load fail.  Assume we have never written it and setup default here.  We should really only run this code once on a new device

        Serial.println("New device or worn out device.  Using default settings");

        // Initial structure test data
        gp_cfg->value1 = 0xdeadbeef;
        gp_cfg->value2 = 0x1234;
    strcpy(gp_cfg->str, "World");
        
        // Save ADT to media with attempt to find next position just use start of media for next write
        if (g_cfg_ps.Save(true)) {
            Serial.println("Write success");
        }else {
            Serial.println("Write failed.  assume device worn out!");
        }
    }

    // print out ADT
    Serial.println("ADT dump");
    Serial.println("--------");
    Serial.print("gp_cfg->value1 = ");
    Serial.println(gp_cfg->value1, HEX);
    Serial.print("gp_cfg->value2 = ");
    Serial.println(gp_cfg->value2, HEX);
    Serial.print("gp_cfg->str = ");
    Serial.println(gp_cfg->str);
    Serial.println("Done");
}

void loop() {
}