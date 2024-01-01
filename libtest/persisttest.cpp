/**
 * \file
 * Windows test harness for generic persistent storage class and struct uses architecture classes with stubs for lowest level (chip) flash API.
 * The flash is represented as a contiguous byte array in windows.
 * PROJECT: PStruct library
 * TARGET SYSTEM: win32 (simulation of Arduino AVR mega and STM32)
 */

#include "stdafx.h"

/**                      *****
 * For testing uncomment * 1 * of the architecture macros (STM32, AVR8MEGA_xxxx) and then optionally:
 *                       *****
 *
 *            PERSISTSTRUCT_POINTERS
 *            LARGE_STRUCT
 *            USE_STORAGE_END
 *
 * When structure changed, capture new crcs using:
 *            NO_DATA_DUMP
 *            RECORD_CRC
 * Manually paste them into appropriate global array.
 *
 * Use LOWLEVEL_DEBUG where problems occur during development to support
 * New architectures.
 */
#define STM32
//#define AVR8MEGA_FLASH
//#define AVR8MEGA_EE

// ware levels or possible write locations (copies)
#define WARE_LEVELS        5

// Rebuild with macro for internal pointers rather than references
//#define PERSISTSTRUCT_POINTERS

// Larger than 1 page data structure
//#define LARGE_STRUCT

// Declare storage end.  this selects a different persistent structure constructor
//#define USE_STORAGE_END

// Define to control data dump output, reduces test time and handy when updating crc list
#define NO_DATA_DUMP

// Define to capture output and manually update crc list prior
//#define RECORD_CRC

// Define to output low level debugging information on r/w operations
//#define LOWLEVEL_DEBUG

// VC++2010 not c11
#define constexpr

// test for multiple targets
#if defined(STM32) && defined(AVR8MEGA_FLASH) || defined(STM32) && defined(AVR8MEGA_EE) || defined(AVR8MEGA_FLASH) && defined(AVR8MEGA_EE)
#error Cannot build for multiple targets.  define STM32 OR AVR8MEGA_EE OR AVR8MEGA_FLASH
#endif // multiple targets

/* STM32 flash testing
 *********************************************************************************************/
#if defined(STM32)

#define ARDUINO_ARCH_STM32

#define TEST_PAGES    (WARE_LEVELS*3)
#define TEST_PAGE_SIZE 1024

// this is our persistent storage on 1K alignment, size chosen enough to meet testing and pretty much arbitrary
__declspec(align(TEST_PAGE_SIZE)) struct bf {
    uint8_t x[TEST_PAGE_SIZE * (TEST_PAGES + 2)];
};

bf bb;
uint8_t *gpFlashU8 = (uint8_t *)(&bb.x[TEST_PAGE_SIZE]);        // u8  ptr to persistent storage
uint32_t *gpFlashU32 = (uint32_t *)(&bb.x[TEST_PAGE_SIZE]);        // u32 ptr to persistent storage
uint32_t *gpFlashU32Base = (uint32_t *)&bb.x[0];                // u32 ptr to persistent storage real base (1 page extra so we can check it doesn't get over written)

#define FSIZE_U32 (sizeof(bb.x)/sizeof(uint32_t))

#include "media.h"
#include "../stm32/f103/wrap.h"
#include "../struct.h"

// succesful test crc's
uint32_t gCRC32List[] = {
#if defined(LARGE_STRUCT)
    0xd2b1c935,
    0xb5d391eb,
    0x495fd7ff,
    0x92538ab3,
    0x05b5004c,
    0xda4821c9,
    0x60eb76f4,
    0x4400bb89,
    0xfdcbc4ca,
    0x96171ea2,
    0xfadc2614,
    0x3c728c8f,
    0xecddccd8,
    0x665448d3,
    0x435be503,
    0xec65ef81,
    0x6a766c92,
    0x24eab45f,
    0x49568eba,
    0x850cabd1,
    0x17e84420,
#else // !defined(LARGE_STRUCT)
    0xd2b1c935,
    0x06aaa483,
    0xb1a28137,
    0x775286c5,
    0xc9032b58,
    0xea03eca6,
    0x32c8f693,
    0x86242d02,
    0x0d3428cf,
    0xc3c39902,
    0xd988a9dc,
    0x4ff47fb5,
    0xc5636221,
    0x908fbc29,
    0x5bc77713,
    0xa12633ea,
    0x97a9e8d1,
    0x6e250920,
    0x9d9b3ce6,
    0xfca7d8ff,
    0xd67d94fe,
#endif // !defined(LARGE_STRUCT)
};

#endif // defined(STM32)

/* AVR8 flash + EE testing
 *********************************************************************************************/
#if defined(AVR8MEGA_FLASH) || defined(AVR8MEGA_EE)

#define ARDUINO_ARCH_AVR

#if defined(AVR8MEGA_FLASH)
#define TEST_PAGES    (WARE_LEVELS*3)
#define TEST_PAGE_SIZE 128
#else // defined(AVR8MEGA_EE)
#define TEST_PAGES    5
#define TEST_PAGE_SIZE 40
#endif // defined(AVR8MEGA_EE)

#define TEST_PAGE_SIZE_ALIGN    1024

#define E2END                    4095

// this is our persistent storage on 1K alignment, size chosen enough to meet testing and pretty much arbitrary
__declspec(align(TEST_PAGE_SIZE_ALIGN)) struct bf {
    uint8_t x[TEST_PAGE_SIZE * (TEST_PAGES + 2)];
};

bf bb;
uint8_t *gpFlashU8 = (uint8_t *)(&bb.x[TEST_PAGE_SIZE]);        // u8  ptr to persistent storage
uint32_t *gpFlashU32 = (uint32_t *)(&bb.x[TEST_PAGE_SIZE]);        // u32 ptr to persistent storage
uint32_t *gpFlashU32Base = (uint32_t *)&bb.x[0];                // u32 ptr to persistent storage real base (1 page extra so we can check it doesn't get over written)
uint8_t pageBuffer[TEST_PAGE_SIZE];

#define FSIZE_U8    (TEST_PAGE_SIZE * (TEST_PAGES + 2))
#define FSIZE_U32    (FSIZE_U8/sizeof(uint32_t))

// rebuild with macro for internal pointers rather than references
//#define PERSISTSTRUCT_POINTERS

bool write_error_inject = false;
bool erase_error_inject = false;

uint8_t        SREG = 0;        // doesn't matter
uint8_t        SPMCSR = 0;
void cli(void) { }
void sei(void) { }
void eeprom_busy_wait(void) { }
void boot_rww_enable(void) { }
void boot_lock_bits_set(uint8_t lock_bits) {
}

uint8_t boot_spm_busy_wait(void) {
    return 0;    // not busy
}

// for address use uint32_t
#define USHRT_MAX_FLASH        (FLASHEND)

//#if (FLASHEND < USHRT_MAX_FLASH)

#define optiboot_addr_t    uint32_t

void optiboot_page_fill(optiboot_addr_t address, uint16_t data) {
    uint16_t *pb = (uint16_t*)pageBuffer;
    uint32_t o = (((uint32_t)address) % TEST_PAGE_SIZE)/sizeof(uint16_t);
    if (!o) {
        memset(&pageBuffer, -1, sizeof(pageBuffer));
    }
#if defined(LOWLEVEL_DEBUG)
    std::cout << "_SPM_FILL(" << std::hex << std::setw(8) << std::setfill('0') << (uint32_t)&pb[o] << ")" << " = " << data << std::endl;
#endif
    pb[o] = data;
}

void optiboot_page_erase(optiboot_addr_t address) {
    uint32_t i = TEST_PAGE_SIZE/sizeof(uint16_t);
    uint16_t *p = (uint16_t*)address;

#if defined(LOWLEVEL_DEBUG)
    std::cout << "_SPM_ERASE(" << std::hex << std::setw(8) << std::setfill('0') << (uint32_t)p << ")" << std::endl;
#endif
    while(i) {
        if (erase_error_inject && ((rand() % 100)>90)) {
            *p = 0;    // error
            erase_error_inject = false;
        }else {
            *p = 0xffff;
        }
        p++;
        i--;
    }
}

void optiboot_page_write(optiboot_addr_t address) {
#if defined(LOWLEVEL_DEBUG)
    std::cout << "_SPM_PAGEWRITE(" << std::hex << std::setw(8) << std::setfill('0') << (uint32_t)address << ")" << std::endl;
#endif
    if (write_error_inject && ((rand() % 100)>90)) {
        // error
        uint32_t i = rand() % sizeof(pageBuffer);
        pageBuffer[i] ^= 0xff;
        write_error_inject = false;
    }
    memcpy((void*)address, &pageBuffer, sizeof(pageBuffer));
}

uint32_t eeprom_read_dword(const uint32_t *__p)    {
    return gpFlashU32[(reinterpret_cast<uint32_t>(__p)>>2)];
}

void eeprom_update_dword(uint32_t *__p, uint32_t v) {
    if (write_error_inject && ((rand() % 100)>90)) {
        // error
        uint32_t i = rand() % sizeof(pageBuffer);
        gpFlashU32[(reinterpret_cast<uint32_t>(__p)>>2)] = 0xff;
        write_error_inject = false;
        return;
    }
    uint32_t d = eeprom_read_dword(__p);
    if (d != v) {
        gpFlashU32[(reinterpret_cast<uint32_t>(__p)>>2)] = v;
    }
}

void eeprom_read_block (void *__dst, const void *__src, size_t __n) {
    uint32_t *s = reinterpret_cast<uint32_t*>(const_cast<void*>(__src));
    uint32_t *d = reinterpret_cast<uint32_t*>(__dst);

    for (;__n>0;__n-=sizeof(uint32_t), s++, d++) {
        *d = eeprom_read_dword(s);
    }
}

uint16_t pgm_read_word(uint32_t __p) {
    uint16_t *p = reinterpret_cast<uint16_t*>(__p);

    return p[0];
}


#include "media.h"
#if defined(AVR8MEGA_FLASH)
#include "../mega/flash.h"
#endif // defined(AVR8MEGA_FLASH)
#include "../mega/wrap.h"
#include "../struct.h"

#if defined(AVR8MEGA_EE)
// succesful test crc's (EE)
uint32_t gCRC32List[] = {
#if defined(LARGE_STRUCT)
    0x00001209,
    0x00005779,
    0x00003d82,
    0x00002daf,
    0x000045ba,
    0x00000934,
    0x000013d8,
    0x0000174d,
    0x0000271d,
    0x00007b04,
    0x00002211,
    0x00003f87,
    0x0000d715,
    0x0000385e,
    0x0000afa0,
    0x00001042,
    0x0000267c,
    0x000087e2,
    0x000033ee,
    0x0000aeca,
    0x0000513b,
#else // !defined(LARGE_STRUCT)
    0x00001209,
    0x0000ebf2,
    0x0000f472,
    0x00003e22,
    0x00009c8b,
    0x000093bb,
    0x0000c179,
    0x00009cda,
    0x00008ee3,
    0x00002629,
    0x0000b0a0,
    0x0000ecf0,
    0x000053b8,
    0x0000fd8e,
    0x0000aadf,
    0x00003911,
    0x00006181,
    0x00006ad6,
    0x0000cc4d,
    0x0000fe66,
    0x000037b5,
#endif // !defined(LARGE_STRUCT)
};
#else // !defined(AVR8MEGA_EE)
// succesful test crc's (FLASH)
uint32_t gCRC32List[] = {
#if defined(LARGE_STRUCT)
    0x0000ef15,
    0x0000747e,
    0x00006aad,
    0x00003f93,
    0x00007e6c,
    0x00006d12,
    0x00000d4b,
    0x0000eaad,
    0x0000779f,
    0x000071f1,
    0x0000a462,
    0x00009ef1,
    0x00005ae4,
    0x0000a385,
    0x00001b95,
    0x00003c4c,
    0x000048e1,
    0x0000aea9,
    0x0000f400,
    0x000020ce,
    0x00002067,
#else // !defined(LARGE_STRUCT)
    0x0000ef15,
    0x0000d80e,
    0x0000e2d6,
    0x00007e2a,
    0x00006030,
    0x0000efc2,
    0x00004b6c,
    0x00007003,
    0x000010e8,
    0x00004fc7,
    0x0000f9e6,
    0x0000bd34,
    0x000097da,
    0x00003b43,
    0x0000011d,
    0x0000dcde,
    0x0000a9b8,
    0x00003288,
    0x000026a2,
    0x0000f5b8,
    0x0000b87d,
#endif // !defined(LARGE_STRUCT)
};
#endif // !defined(AVR8MEGA_EE)

#endif // defined(AVR8MEGA_FLASH) || defined(AVR8MEGA_EE)

/**
 * Get next CRC from global table.  Index internally managed as static variable
 * 
 * @param[in] clear, default false.  set to true to zero internally managed index before read
 * @return uint32_t crc value.  algorithm dependant
 */
uint32_t GetNextCRC(bool clear=false) {
    static uint32_t i = 0;
    uint32_t s = sizeof(gCRC32List) / sizeof(uint32_t);
    uint32_t c = 0;

    if (clear) {
        i = 0;
    }

    if (i<s) {
        c = gCRC32List[i++];
    }

    return c;
} // GetNextCRC(...)


/**
 * Check CRC, generates a software CRC and checks against table returned value, @ref getNextCRC
 * 
 * @return bool true on match otherwise false (assume error condition)
 */
bool CheckCRC() {
    bool ok = true;
    uint32_t crc32[2];

    // we check entire storage which has extra empty data before and after storage area to ensure we are not overwriting
    crc32[0] = swimp::Crc::Generate((uint32_t*)&bb, sizeof(bb)/sizeof(uint32_t));
    crc32[1] = GetNextCRC();
    std::cout << std::endl << "memory crc " << std::hex << std::setw(8) << std::setfill('0') << crc32[0] << std::endl;
#if !defined(RECORD_CRC)
    if (crc32[0] != crc32[1]) {
        std::cerr << "ERROR: bad crc. " << std::hex << std::setw(8) << std::setfill('0') << crc32[0] << " should be " << \
            std::hex << std::setw(8) << std::setfill('0') << crc32[1] << std::endl << std::endl;
        ok = false;
    }
#endif // !defined(RECORD_CRC)

    return ok;
} // CheckCRC()


/**
 * this is a test structure to use with persistent storage and would normally be application specific.
 * if large structure, then it exceeds a single page which is helpful in non-32bit architectures where 
 * the page sizes are much smaller.
 */
#pragma pack(push,1)
typedef struct {
    uint32_t enable;
    uint32_t os;
    uint8_t  str[5];    // C array size ensures ensures structure not a division of sizeof(uint32_t)
#if defined(LARGE_STRUCT)
    uint8_t  excess[TEST_PAGE_SIZE];
#endif // defined(LARGE_STRUCT)
}cfg_t;
#pragma pack(pop)

/**
 * Set entire persistent storage data to given value
 *
 * @param[in] uint32_t v value to write
 */
static void Pset(uint32_t v) {
    for(uint32_t i=0; i<FSIZE_U32; i++) {
        gpFlashU32Base[i] = v;
    }
} // Pset(...)


/**
 * Dump persistent storage using given pointer, size and page size
 *
 * @param[in] uint32_t *b buffer pointer
 * @param[in] uint32_t l buffer length (Bytes)
 * @param[in] uint32_t ps page size (Bytes)
 */
static void Pdump(uint32_t *b, uint32_t l, uint32_t ps) {
    uint32_t psu32 = ps/sizeof(uint32_t);

    for(uint32_t c=0, i=0, p=0; i<l; i++, c++) {
        if (c==psu32) {
            std::cout << std::endl << std::endl << "page " << p++ << std::endl;
            std::cout << std::endl << std::hex << std::setw(8) << std::setfill('0') << (&b[i]) << ": ";
            c=0;
        }else if ((c % 8)==0) {
            std::cout << std::endl << std::hex << std::setw(8) << std::setfill('0') << (&b[i]) << ": ";
        }

        std::cout << std::hex << std::setw(8) << std::setfill('0') << b[i] << " ";
    }
    std::cout << std::endl;
} // Pdump(...)


/**
 * Test harness entry point
 *
 * @param[in] int argc Argument count
 * @param[in] _TCHAR* argv[].  String argument pointer array
 * @return int Status
 * @retval 0 Success
 * @retval 1 Failure
 */
int _tmain(int argc, _TCHAR* argv[]) {
#if defined(AVR8MEGA_EE)
    // we could use nPersist::cStruct<cfg_t>::calculateStorageUnitSize() since its EE
    wrap::Ee<TEST_PAGE_SIZE> f;
#else
    wrap::Flash f;
#endif

    // Repeatable rand()'s
    srand(-1);

    // Output storage information
    std::cout << "page size   = " << f.GetPageSize() << " Bytes (" << (f.GetPageSize()/1024) << " KBytes)" << std::endl;
    std::cout << "memory size = " << f.GetSize() << " Bytes (" << (f.GetSize()/1024) << " KBytes)" << std::endl;
#if defined(AVR8MEGA_FLASH) || defined(STM32)
    std::cout << "start       = " << std::hex << std::setw(8) << std::setfill('0') << f.GetStart() << \
        ".  should be " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t*>(gpFlashU8) << std::endl;
    if (f.GetStart() != reinterpret_cast<uint32_t*>(gpFlashU8)) {
#else // !defined(AVR8MEGA_FLASH)
    std::cout << "start       = " << std::hex << std::setw(8) << std::setfill('0') << f.GetStart() << \
        ".  should be " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t*>(0) << std::endl;
    if (f.GetStart()>reinterpret_cast<uint32_t*>(f.GetSize())) {
#endif // !defined(AVR8MEGA_FLASH)
        std::cerr << std::endl << "ERROR: address incorrect " << f.GetSize() << std::endl << std::endl;
        return 1;
    }

#if defined(AVR8MEGA_FLASH) || defined(STM32)
    std::cout << "end         = " << std::hex << std::setw(8) << std::setfill('0') << f.GetEnd() << \
        ".  should be " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t*>(gpFlashU8 + f.GetSize()) << std::endl << std::endl;
    if (f.GetEnd() != reinterpret_cast<uint32_t*>(gpFlashU8 + f.GetSize())) {
#else // !defined(AVR8MEGA_FLASH)
    std::cout << "end         = " << std::hex << std::setw(8) << std::setfill('0') << f.GetEnd() << \
        ".  should be " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t*>(f.GetSize()) << std::endl << std::endl;
    if (f.GetEnd() != reinterpret_cast<uint32_t*>(f.GetSize())) {
#endif // !defined(AVR8MEGA_FLASH)
        std::cerr << std::endl << "ERROR: address incorrect" << std::endl << std::endl;
        return 1;
    }

    // Zero clear entire storage.  this is unlikely the erase state and should easily be seen in dumps
    Pset(0);

    // CRC entire storage + check
    if (!CheckCRC()) {
        return 1;
    }

#if !defined(USE_STORAGE_END)
    // Use cfg_t and f for persistent storage, n ware levels.  this may not be pages, it depends upon structure size and overhead
    persist::Struct<cfg_t> c(f, f.GetStart(), WARE_LEVELS);
#else // defined(USE_STORAGE_END)
    // Reverse math to what is in the constructor since we do have a fixed storage size for this test, normally you'd just specify the end address and check the 
    // ware levels to make sure it is sufficient however i want the crc's to be the same to make it easier on myself.
    uint32_t pssU32 = persist::Struct<cfg_t>::GetStorageUnitSize() / f.GetPageSize();
    if (persist::Struct<cfg_t>::GetStorageUnitSize() % f.GetPageSize()) {
        pssU32++;
    }
    pssU32 *= WARE_LEVELS * (f.GetPageSize() / sizeof(uint32_t));
    persist::Struct<cfg_t> c(f, f.GetStart(), f.GetStart() + pssU32);
#endif // defined(USE_STORAGE_END)

    // Output debugging information on structure storage
    std::cout << "structure size " << std::dec << sizeof(cfg_t) << " Bytes, padded with overhead " << c.GetStorageUnitSize() << " Bytes. as " << c.GetPages() << " pages.  Ware levels " << c.GetWareLevels() << std::endl;
    std::cout << "initial count " << c.GetCounter() << ", location " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t>(c.GetLocation()) << std::endl << std::endl;

#if defined(PERSISTSTRUCT_POINTERS)
    cfg_t *d = c.Get();
#else // !PERSISTSTRUCT_POINTERS
    cfg_t cd;
    cfg_t *d = &cd;

#endif // !PERSISTSTRUCT_POINTERS

    memset(d, 0, sizeof(cfg_t));

    // Initial load should always fail due to storage empty
#if defined(PERSISTSTRUCT_POINTERS)
    if (c.Load())
#else // !PERSISTSTRUCT_POINTERS
    if (c.Load(cd))
#endif // !PERSISTSTRUCT_POINTERS
    {
        std::cerr << std::endl << "ERROR: load cfg ok" << std::endl << std::endl;
        return 1;
    }else {
        std::cout << std::endl << "load cfg failed" << std::endl << std::endl;
    }

    // Initial structure test data
    d->enable = 1;
    d->os = 0x100;
    d->str[0] = 'A';
    d->str[1] = 'B';
    d->str[2] = 'C';
    d->str[3] = 'D';
    d->str[4] = 'E';

    // n load/save cycles
    for(uint32_t wr=0; wr<20; wr++) {
        // If cycle after 10, inject errors either write data or erase.  result should be visible in dumps
        if (wr>10) {
            if ((rand() % 100)>50) {
#if defined(STM32)
                f.InjectWriteError(true);
#else    // !STM32
                write_error_inject = true;
#endif    // !STM32
            }else {
#if defined(STM32)
                f.InjectEraseError(true);
#else    // !STM32
                erase_error_inject = true;
#endif    // !STM32
            }
        }

#if defined(PERSISTSTRUCT_POINTERS)
        if (c.Save(true))
#else // !PERSISTSTRUCT_POINTERS
        if (c.Save(cd,true))
#endif // !PERSISTSTRUCT_POINTERS
        {
#if defined(AVR8MEGA_EE)
            std::cout << std::endl << "save cfg ok.  location " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t>(c.GetLocation()) << ", counter " << c.GetCounter() << std::endl << std::endl;
#else // !defined(AVR8MEGA_EE)
            std::cout << std::endl << "save cfg ok.  location " << std::hex << std::setw(8) << std::setfill('0') << (c.GetLocation() - gpFlashU32) << ", counter " << c.GetCounter() << std::endl << std::endl;
#endif // !defined(AVR8MEGA_EE)
        }else {
            std::cerr << std::endl << "ERROR: save cfg failed" << std::endl << std::endl;

            // Should there be something already to load?
            if (wr>0)
            {
            #if defined(PERSISTSTRUCT_POINTERS)
                if (c.Load())
            #else // !PERSISTSTRUCT_POINTERS
                if (c.Load(cd))
            #endif // !PERSISTSTRUCT_POINTERS
                {
#if defined(AVR8MEGA_EE)
                    std::cout << std::endl << "current re-load cfg ok after save failed.  location " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t>(c.GetLocation()) << ", counter " << c.GetCounter() << std::endl << std::endl;
#else // !defined(AVR8MEGA_EE)
                    std::cout << std::endl << "current re-load cfg ok after save failed.  location " << std::hex << std::setw(8) << std::setfill('0') << (c.GetLocation() - gpFlashU32) << ", counter " << c.GetCounter() << std::endl << std::endl;
#endif // !defined(AVR8MEGA_EE)
                }else {
                    std::cerr << std::endl << "ERROR: current re-load cfg after save failed has also failed" << std::endl << std::endl;
                }
            }
            return 1;
        }

        // Prevent error injector
#if defined(STM32)
        f.InjectWriteError(false);
        f.InjectEraseError(false);
#else    // !STM32
        write_error_inject = false;
        erase_error_inject = false;
#endif    // !STM32


#if !defined(NO_DATA_DUMP)
        // Dump output.  first and last pages shouldn't be written to
        Pdump((uint32_t*)&bb.x[0], sizeof(bb.x)/sizeof(uint32_t), f.GetPageSize());
#endif // !defined(NO_DATA_DUMP)

        // Zero clear cfg_t struct to prove we load it
        memset(d, 0, sizeof(cfg_t));

#if defined(PERSISTSTRUCT_POINTERS)
        if (c.Load())
#else // !PERSISTSTRUCT_POINTERS
        if (c.Load(cd))
#endif // !PERSISTSTRUCT_POINTERS
        {
#if defined(AVR8MEGA_EE)
            std::cout << std::endl << "load cfg ok.  location " << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t>(c.GetLocation()) << ", counter " << c.GetCounter() << std::endl << std::endl;
#else // !defined(AVR8MEGA_EE)
            std::cout << std::endl << "load cfg ok.  location " << std::hex << std::setw(8) << std::setfill('0') << (c.GetLocation() - gpFlashU32) << ", counter " << c.GetCounter() << std::endl << std::endl;
#endif // !defined(AVR8MEGA_EE)
        }else {
            std::cerr << std::endl << "ERROR: load cfg failed" << std::endl << std::endl;
            return 1;
        }

        // Output cfg_t struct content (what we loaded)
        std::cout << "d->enable = " << d->enable << std::endl;
        std::cout << "d->os     = " << std::hex << std::setw(8) << std::setfill('0') << d->os << std::endl;
        std::cout << "d->str    = ";
        for(uint32_t i = 0; i<sizeof(d->str); i++) {
            if (d->str[i]>32 && d->str[i]<127) {
                std::cout << static_cast<char>(d->str[i]) << "   ";
            }else {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(d->str[i]) << std::endl;
            }
        }
        std::cout << std::endl;

        // CRC entire storage + check
        if (!CheckCRC()) {
            return 1;
        }

        // Change data ready for next save.  now in theory this is optional but it helps show differences if it fails
        std::cout << std::endl << "change data" << std::endl << std::endl;
        d->enable^=1;
        d->os++;
        uint8_t tmp = d->str[0];
        d->str[0] = d->str[1];
        d->str[1] = d->str[2];
        d->str[2] = d->str[3];
        d->str[3] = d->str[4];
        d->str[4] = tmp;
    } // for(uint32_t wr=0...)

    std::cout << "all tests complete" << std::endl;

    return 0;
} // int _tmain(...)
