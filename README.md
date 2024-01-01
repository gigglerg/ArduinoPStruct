# PStruct

**PStruct** (Persistent Structure) library offers access to different flash media 
including on-chip flash controllers for user data storage.
A simple load/save API is offered that wraps ware levelling over the top of device 
specific flash drivers.  For new devices you'd create a wrapper class which forms 
a standard interface.

While different types of media have their own characteristics; a known problem with 
flash is endurance and this can be extended with a ware levelling algorithm as 
implemented by PStruct.
PStruct stores user ADT's with cyclic redundancy check validation over multiple 
pages, handling restoring the most recent copy using old data as fall backs.

Build environments include Arduino framework and bare metal development as the 
library code is completely independent (excludes chip specific drivers).

[License: GPL2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)


## Using the library

For now as this is a library, Doxygen documentation is available by the following links

* [AVR8 documentation](../avr8_html/index.htm)
* [STM32 documentation](../stm32_html/index.htm)

To rebuild simple use make with architecture like "make avr8" or "make stm32" from within 
the "./docs" folder.


## Requirements

* Supports
  * Arduino ([AVR8 Mega single flash bank](http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)) devices
    * [Arduino Uno/Nano](https://www.arduino.cc/)
    * [Maple Mini](http://docs.leaflabs.com/static.leaflabs.com/pub/leaflabs/maple-docs/latest/hardware/maple-mini.html)
    * Blue Pill
    
  * [STM32 Cortex M series](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html)
    * Requires [STM32dunio Arduino Core STM32](https://github.com/stm32duino/Arduino_Core_STM32), tested release 1.4.0
    * Optional use of CRC hardware


## Library Installation

PStruct is available for installation in the [Arduino Library Manager].
Alternatively it can be downloaded from GitHub using the following:

1. Download the ZIP file from https://github.com/gigglerg/PStruct/archive/master.zip
2. Rename the downloaded file to `PStruct.zip`
3. From the Arduino IDE: Sketch -> Include Library... -> Add .ZIP Library...
4. Restart the Arduino IDE to see "PStruct" library with examples

See the comments in the example sketches for coding use details.


## Using in STM32 Workbench

Simple download the ZIP file as mentioned above in [Library Installation](#Library-Installation) and 
decompress source files into your source tree.  In Eclipse or your source editor add files from ZIP.


### Include library and wrapper drivers

```cpp
// Include media storage access base class and user api
#include "media.h"
#include "struct.h"

// Include wrapper for device specific drivers.  For STM32F103
#include "stm32/f103/wrap.h"

// Or

#include "mega/wrap.h"

// You may have to write your own wrapper.  Look at the existing wrappers for coding support.

// Now create an instance of media wrapper (the low level driver)
wrap::Flash media;

```

If you want to use pointers to save on memory footprint:


```cpp

#define PERSISTSTRUCT_POINTERS
#include "struct.h"

// API for load and save have changed and you'll have to use get(...)

```


### Creating your ADT

Whatever data you want to store, create it as a type and populate with default data (optional):

```cpp
// Here is my ADT that I want stored
typedef struct {
	uint32_t value;
    int16_t  some_other_value;
    ...
	char     str[13];
}my_data_t;

// Some data I want stored, it doesn't have to be global.  populate defaults (optional)
my_data_t adt = {
    .value = 0x12345678,
    .some_other_value = -178,
    .str = { "HELLOWORLD\0" }
};

```


### Now the PStruct instance for data handling

You can other more than one instance, using the same or different media and use them to work with 
various types of data stored a different locations.   The most obvious use is for device configuration 
information that may or may not change.


```cpp
// Create PStruct instance.  There are a couple of constructors and this one creates based upon defined
// start and end locations in storage medium.  Careful, here we are allocating storage at end of on-chip
// flash which is fine but remember you could overwrite the vector table if you put it at the start.
// See examples for further details, including how to specify start by compiler variables rather than 
// absolute addresses and control via number of ware levels.
#define PAGES 10 // allocate n physical pages
persist::Struct<adt> adt_access(media, media.getEnd() - (media.getPageSize() * PAGES), PAGES);

int main(...) {
    // attempt load ADT...
	if (adt_access.load(adt)) {
        // success
	}else {
		// ADT load fail.  assume we have never written it and use default
		
		// save ADT
		if (adt_access.save(adt,true /* force save without previous load */)) {
			Serial.println("Write success");
		}else {
			Serial.println("Write failed.  assume device worn out!");
		}
    }

    // now use ADT
    ...
}

```

Or you can let the compiler and linker resolve location within flash

```cpp
// You create this macro, name isnt important but the levels should be changed to meet
// application requirements.  MCU datasheet covers specific flash 
#define WARELEVELS 4

// Data you want persistent storage of...
typedef struct {
    uint32_t hw_version;
    uint32_t crc;
    uint16_t another_field;
    uint16_t something_else;
    uint32_t you_decide[2];
}appdata_t;

#if defined(ARDUINO_ARCH_AVR)
// For AVR core - Arduino
const uint8_t g_flash[PERSISTSTRUCT_SIZE(appdata_t,WARELEVELS)] PROGMEM __attribute__ ((aligned (AVR_FLASH_PAGE_SIZE))) = { 0xff };

#elif defined(ARDUINO_ARCH_STM32)
// For STM32 core - Maplemini, Blue pill
const uint8_t g_flash[PERSISTSTRUCT_SIZE(appdata_t,STM32F103X_FLASH_PAGE_SIZE,WARELEVELS)] __attribute__ ((aligned (STM32F103X_FLASH_PAGE_SIZE))) = { 0xff };
#endif

// MCU Media access instance...
wrap::Flash g_media;

// Create PStruct instance that you will use to access your most recently saved appdata_t stored in memory allocated to g_flash
persist::Struct<appdata_t> g_appdata(g_media, reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(&g_flash[0])), WARELEVELS);

```


## OS or Tasker

Implement media locking by overriding Read and Program methods of your lower level driver wrapper class rather 
than poking around with PStruct itself.   It might be most efficient to consider seperate read and write 
mutexes where access because of a write doesn't effect reads.


## Gotchas

1. AVR's currently only single bank devices are supported.  Larger flash memorys may require addition work 
to get PStruct working.
2. STM32, using STM32Cube HAL library.  You will have to enable the Flash module and optionally CRC.  These 
are done by macros specific to your project.  In Arduino add file "build_opt.h" with the following line:

```cpp
-DHAL_CRC_MODULE_ENABLED -DHAL_FLASH_MODULE_ENABLED
```


## Contributing

Contributions to the code are welcome using the normal Github pull request workflow.

Please remember the small memory footprint of most micro controllers like Arduino, so try and be efficient when adding new features.


### Code Style

When making contributions, please follow [Google's code style](https://google.github.io/styleguide/cppguide.html) where possible to keep the codebase consistent.

* Indent *4 spaces*, No hard tabs
* EOL Unix (Library code)
* The rest is online!
