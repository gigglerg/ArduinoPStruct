/**
 * Example use of PStruct (persistent storage structure classes).  In this example we store a user defined ADT (once) and 
 * then repeatedly loads it upon reboots.
 *
 * Architecture support:
 *  AVR8 (Uno/Nano)
 *
 * DG, 2019
 */

#if !defined(ARDUINO_ARCH_AVR)
#error Unsupported core
#endif

// Include media storage access base class and user api
#include "media.h"
#include "struct.h"

// Include chip specific media wrapper
#include "mega/wrap.h"

/**
 * Application specific macro stating how many storage pages are to be used.  this essential controls wear levelling as a 
 * higher value will use more storage over time but increases endurance and therefore your applications useful life.  If 
 * you know you are likely to perform a lot of writes, increase the value to suit.  You may need to look at your devices
 * flash module in manufacturer datasheet/reference manual in order to correctly size.
 */
#define PSTRUCT_USE_N_PAGES        5


/**
 * Application specific ADT, you define this to suit your data requirements.  It is written to and read from persistent storage
 * and you will access your data via this type.  Most likely this will be a configuration structure of some form.
 *
 * @note packing not important whatever gives to best optimisation either for access by cpu (typical padded) or storage (not padded)
 */
#pragma pack(push,1)
typedef struct {
    char  str[6];    // C array size ensures ensures structure not a division of sizeof(uint32_t)
    uint32_t value1;
    uint16_t value2;
}cfg_t;
#pragma pack(pop)


/**
 * Create your media access instance.  This wrapper class allows you to customise where your persistent storage resides.  It is
 * normally via on-chip flash but could be some place else should you decide to write your own.  AVR on-chip data EE doesn't use 
 * pages which is why here calculateStorageUnitSize() is used to make a page the size of your ADT + overhead - this is optional 
 * and you could specify a page size like used by various flash types
 */
wrap::Ee<persist::Struct<cfg_t>::GetStorageUnitSize()> g_media;


/**
 * Use cfg_t and g_media for persistent storage, n pages worth.  Use this instance to update media with your structure changes
 * or to simple load your structure.
 */
persist::Struct<cfg_t> g_cfg_ps(g_media, g_media.GetStart(), PSTRUCT_USE_N_PAGES);


/**
 * Your application ADT.  use this to access your data
 */
cfg_t g_cfg;


void setup() {
    Serial.begin(9600);
    while(!Serial) { }
    
    Serial.println("setup()");

    // Attempt to load user ADT from media
    if (g_cfg_ps.Load(g_cfg)) {
        // ADT loaded.  now you can access your data
    }else {
        // ADT load fail.  Assume we have never written it and setup default here.  we should really only run this code once on a new device

        Serial.println("New device or worn out device.  Using default settings");

        // Initial structure test data
        g_cfg.value1 = 0xcafef00d;
        g_cfg.value2 = 0x4321;
        g_cfg.str[0] = 'H';
        g_cfg.str[1] = 'e';
        g_cfg.str[2] = 'i';
        g_cfg.str[3] = 'd';
        g_cfg.str[4] = 'i';
        g_cfg.str[5] = '\0';
        
        // Save ADT to media with attempt to find next position just use start of media for next write
        if (g_cfg_ps.Save(g_cfg,true)) {
            Serial.println("Write success");
        }else {
            Serial.println("Write failed.  assume device worn out!");
        }
    }

    // Print out ADT
    Serial.println("ADT dump");
    Serial.println("--------");
    Serial.print("g_cfg.value1 = ");
    Serial.println(g_cfg.value1, HEX);
    Serial.print("g_cfg.value2 = ");
    Serial.println(g_cfg.value2, HEX);
    Serial.print("g_cfg.str = ");
    Serial.println(g_cfg.str);
    Serial.println("Done");
}

void loop() {
}