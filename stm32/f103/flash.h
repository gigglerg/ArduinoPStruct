/**
 * \file
 * Flash API for STM32F103x
 * PROJECT: PStruct library
 * TARGET SYSTEM: Maple Mini
 */

#ifndef FLASHSTM32F103C_H
#define FLASHSTM32F103C_H

#if defined(ARDUINO_ARCH_STM32)

#include <cstring>
#if defined(_MSC_VER)
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#endif // defined(_MSC_VER)

namespace stm32f103x {

/* STM32F103C8T6 device information:
 * 64kb flash    0x00000000 - 0x0000ffff
 *                 0x08000000 - 0x0800ffff (shadow)
 * STM32F103CB  device information:
 * 128kb flash    0x00000000 - 0x0001ffff
 *                 0x08000000 - 0x0801ffff (shadow)
 * ardunio bootloader at start of flash, page size looks like 1K.  going to be a problem with devices that have more than 1bank as page size seems to change
 */

// define STM32F103X_FLASH_XXXX to match your mpu, preferably outside of this module
#if defined(_MSC_VER)
// Windows test harness memory
extern uint8_t* ::gpFlashU8;
#define STM32F103X_FLASH_SHADOW_START           ::gpFlashU8
#else // !defined(_MSC_VER)
#if !defined(STM32F103X_FLASH_SHADOW_START)
/**
 * Device NOR shadow flash start address (bank0)
 */
#define STM32F103X_FLASH_SHADOW_START           0x08000000
#endif // !defined(STM32F103X_FLASH_SHADOW_START)
#endif // !defined(_MSC_VER)
#if !defined(STM32F103X_FLASH_SIZE)
/**
 * Device flash size Bytes.  This is for bank0 only
 */
#define STM32F103X_FLASH_SIZE                   0x00010000
#endif // !defined(STM32F103X_FLASH_SIZE)

#if !defined(STM32F103X_FLASH_PAGE_SIZE)
/**
 * Device page size in Bytes.  This is for bank0 only
 */
#define STM32F103X_FLASH_PAGE_SIZE              1024    // Bytes
#endif // !defined(STM32F103X_FLASH_PAGE_SIZE)

#if !defined(STM32F103X_FLASH_PAGE_SIZE_U32)
/**
 * Device page size in sizeof(uint32_t) multiples.  This is for bank0 only
 */
#define STM32F103X_FLASH_PAGE_SIZE_U32          (STM32F103X_FLASH_PAGE_SIZE / sizeof(uint32_t))        // 1024Bytes or N 32bit words
#endif // !defined(STM32F103X_FLASH_PAGE_SIZE_U32)

#if !defined(STM32F103X_FLASH_NOR_ERASE_STATE)
/**
 * Device NOR flash erase state uint32_t numeric
 */
#define STM32F103X_FLASH_NOR_ERASE_STATE        0xffffffff
#endif  // !defined(STM32F103X_FLASH_NOR_ERASE_STATE)

/*! \cond PRIVATE */
#if !defined(HAL_FLASH_MODULE_ENABLED)
// RCC structure taken from STM32 chip headers
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_RegStruct;

// helpers for register access
#define STM32F103X_SET_REG(addr,val)            do { *(volatile uint32_t*)(addr)=val; } while(0)
#define STM32F103X_GET_REG(addr)                (*(volatile uint32_t*)(addr))

// reset and clock controller module (RCC)
#define STM32F103X_RCC                          ((uint32_t)0x40021000)

// flash module
#define STM32F103X_FLASH                        ((uint32_t)0x40022000)

// RCC module register offsets
#define STM32F103X_RCC_CR                       STM32F103X_RCC
#define STM32F103X_RCC_CFGR                     (STM32F103X_RCC + 0x04)
#define STM32F103X_RCC_CIR                      (STM32F103X_RCC + 0x08)
#define STM32F103X_RCC_AHBENR                   (STM32F103X_RCC + 0x14)
#define STM32F103X_RCC_APB2ENR                  (STM32F103X_RCC + 0x18)
#define STM32F103X_RCC_APB1ENR                  (STM32F103X_RCC + 0x1C)

// Flash module register offsets
#define STM32F103X_FLASH_ACR                    (STM32F103X_FLASH + 0x00)
#define STM32F103X_FLASH_KEYR                   (STM32F103X_FLASH + 0x04)
#define STM32F103X_FLASH_OPTKEYR                (STM32F103X_FLASH + 0x08)
#define STM32F103X_FLASH_SR                     (STM32F103X_FLASH + 0x0C)
#define STM32F103X_FLASH_CR                     (STM32F103X_FLASH + 0x10)
#define STM32F103X_FLASH_AR                     (STM32F103X_FLASH + 0x14)
#define STM32F103X_FLASH_OBR                    (STM32F103X_FLASH + 0x1C)
#define STM32F103X_FLASH_WRPR                   (STM32F103X_FLASH + 0x20)

// Flash module constants and magic numbers
#define STM32F103X_FLASH_KEY1                   0x45670123
#define STM32F103X_FLASH_KEY2                   0xCDEF89AB
#define STM32F103X_FLASH_RDPRT                  0x00A5
#define STM32F103X_FLASH_SR_BSY                 0x01
#define STM32F103X_FLASH_CR_PER                 0x02
#define STM32F103X_FLASH_CR_PG                  0x01
#define STM32F103X_FLASH_CR_START               0x40
#endif // !defined(HAL_FLASH_MODULE_ENABLED) 
/*! \endcond */

/**
 *    A class offering internal NOR flash access via API for device STM32F103C8T6
 *  Some of the source was referenced from Arduino Maple Mini Bootloader
 *
 * \attention Ether use OpenSTM32 HAL or for Arduino install library Arduino_Core_STM32 
 * https://github.com/stm32duino/Arduino_Core_STM32/
 * \attention Optionally define HAL_FLASH_MODULE_ENABLED macro to use this class
 */
class Flash {
public:
#if defined(_MSC_VER)
    static bool write_error_inject;
    static bool erase_error_inject;
#endif // defined(_MSC_VER)

    static const uint32_t page_size;    /// Device page size (Bytes)
    static const uint32_t flash_size;   /// Device flash size (Bytes), bank0 only
    static uint32_t* const flash_start; /// Device flash start address
    static uint32_t* const flash_end;   /// Device flash end address (top, non-accessible)

    /**
     * Program N pages starting at buffer with given data to sizeU32.   Will only program if data not already written 
     * and will only erase pages if they are not in erase state.  Data written is verified as part of write.
     * If start buffer location not paged aligned, extra pages will be erased.
     *
     * \param[in] buffer Pointer to destination flash address of program
     * \param[in] data Pointer to data to write
     * \param[in] size_u32 Data size of program / sizeof(uint32_t)
     * \param[in] page_size_u32 Device page size, multiples of sizeof(uint32_t).  Default \ref STM32F103X_FLASH_PAGE_SIZE_U32
     * \param[in] use_lock Boolean controls flash unlock and locked state.  If true will unlock and leave locked
     * \retval true on program success
     * \retval false failure
     */ 
    static bool Program(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32, \
                            const uint32_t page_size_u32 = STM32F103X_FLASH_PAGE_SIZE_U32, const bool use_lock=true) {
        bool done = true;

        // Buffer size and page size not 0?
        if (!size_u32 || !page_size_u32) {
            done = false;
        }

        // Do we need to program?
        if (done && !Verify(buffer, data, size_u32)) {
            uint32_t bam = reinterpret_cast<uint32_t>(buffer) & ~((page_size_u32<<2)-1);
            uint32_t be = reinterpret_cast<uint32_t>(buffer) + (size_u32<<2);
            
            if (use_lock) {
                Unlock();
            }

            // Page count
            int32_t pc = size_u32 / page_size_u32;

            if (size_u32 % page_size_u32) {
                pc++;    // Use next page, size not page aligned
            }
            // If actual end write is on another page, increment
            bam+=pc * page_size_u32<<2;
            if (be > bam) {
                pc++;    // Use next page, buffer end location not page aligned
            }

            // Erase all required pages for buffer program operation
            done = ErasePages(buffer, pc, page_size_u32);
            if (done) {
                // Write buffer to erased pages
                done = Write32Buffer(buffer, data, size_u32);
            }

            if (use_lock) {
                Lock();
            }
        }

        return done;
    } // Program(...)


    /**
     * Read page data starting at buffer to size_u32 into data
     *
     * \param[in] buffer Pointer to source flash address
     * \param[out] data Pointer to data, destination of read
     * \param[in] size_u32 Data size / sizeof(uint32_t)
     * \retval true on read success
     * \retval false on failure
     */
    static bool Read(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32) {
        memcpy( reinterpret_cast<void *>(const_cast<uint32_t *>(data)), reinterpret_cast<void *>(const_cast<uint32_t *>(buffer)), static_cast<int32_t>(size_u32)<<2 );

        return true;
    } // Read(...)

    
    /**
     * Verify given buffer in flash
     *
     * \param[in] buffer Pointer to source flash address
     * \param[in] data Pointer to data to verify against
     * \param[in] size_u32 Data size / sizeof(uint32_t)
     * \retval true when data represents what is on media
     * \retval false one or more differences between data and media
     */
    static bool Verify(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32) {
        for(uint32_t i=0; i<static_cast<uint32_t>(size_u32); i++) {
            if (buffer[i] != data[i]) {
                return false;
            }
        }

        return true;
    } // Verify(...)


    /**
     * Write buffer in 32bit words to flash.  Flash location prior must be in erase state and flash unlocked
     *
     * \param[in] buffer Pointer to destination flash address
     * \param[in] data Pointer to source data
     * \param[in] size_u32 Data size of program / sizeof(uint32_t)
     * \retval true write success
     * \retval false write failure
     */
    static bool Write32Buffer(const uint32_t *buffer, const uint32_t *data, const int32_t size_u32) {
        uint32_t *b = const_cast<uint32_t *>(buffer), *d = const_cast<uint32_t *>(data);
        int32_t s = size_u32;
        bool done = true;

        for(;s>0;s--, d++, b++) {
            done = Flash::Write32(b, d[0]);
            if (!done) {
                break;
            }
        }

        return done;
    } // Write32Buffer(...)


    /**
     * Write 32bit word to flash.  Flash location prior must be in erase state and flash unlocked
     *
     * \param[in] address Pointer to destination flash address
     * \param[in] data Source data
     * \retval true write success
     * \retval false write failure
     */
    static bool Write32(const uint32_t *address, const uint32_t data) {
        struct {
            union {
                uint32_t u32;
                struct {
                    uint16_t    l;
                    uint16_t    h;
                }hw;
            };
        } w = { data };
#if !defined(HAL_FLASH_MODULE_ENABLED)
        uint16_t *a = reinterpret_cast<uint16_t *>(const_cast<uint32_t *>(address));
#endif // !defined(HAL_FLASH_MODULE_ENABLED)
        bool done = false;

#if defined(_MSC_VER)
        a[1] = w.hw.h;
        a[0] = w.hw.l;

        // inject write error
        if (write_error_inject && ((rand() % 100)>50)) {
            std::cout << std::endl << "wrErr @ " << std::hex << std::setw(8) << std::setfill('0') << a << std::endl;
            if (((rand() % 100)>50)) {
                a[0] = static_cast<uint16_t>(rand() % 65535);
            }else {
                a[1] = static_cast<uint16_t>(rand() % 65535);
            }
            write_error_inject = false;
        }
#else // !defined(_MSC_VER)
#if !defined(HAL_FLASH_MODULE_ENABLED)
        uint32_t rwmVal = STM32F103X_GET_REG( STM32F103X_FLASH_CR );
        STM32F103X_SET_REG( STM32F103X_FLASH_CR, STM32F103X_FLASH_CR_PG );

        // Anything outstanding?
        while( STM32F103X_GET_REG( STM32F103X_FLASH_SR ) & STM32F103X_FLASH_SR_BSY ) { }

        // Write 16bits, high/low half words
        a[1] = w.hw.h;
        while( STM32F103X_GET_REG( STM32F103X_FLASH_SR ) & STM32F103X_FLASH_SR_BSY ) { }

        a[0] = w.hw.l;
        while( STM32F103X_GET_REG( STM32F103X_FLASH_SR ) & STM32F103X_FLASH_SR_BSY ) { }

        rwmVal &= 0xFFFFFFFE;
        STM32F103X_SET_REG( STM32F103X_FLASH_CR, rwmVal );
        
#else // defined(HAL_FLASH_MODULE_ENABLED)
        uint32_t a = reinterpret_cast<uint32_t>(const_cast<uint32_t *>(address));
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, a, static_cast<uint64_t>(data));

#endif // defined(HAL_FLASH_MODULE_ENABLED)
#endif // !defined(_MSC_VER)

        // verify
        if (address[0] == data) {
            done = true;
        }

        return done;
    } // Write32(...)


    /**
     * Lock flash
     *
     * \retval true locked
     * \retval false failed
     */        
    static bool Lock() {
        bool state = true;
        
        // Ensure all FPEC functions disabled and lock the FPEC
#if !defined(_MSC_VER)
#if !defined(HAL_FLASH_MODULE_ENABLED)
        STM32F103X_SET_REG( STM32F103X_FLASH_CR, 0x00000080 );

#else // defined(HAL_FLASH_MODULE_ENABLED)
        if (HAL_OK != HAL_FLASH_Lock()) {
            state = false;
        }
#endif // defined(HAL_FLASH_MODULE_ENABLED)
#endif // !defined(_MSC_VER)
        return state;
    } // Lock()


    /**
     * Unlock flash
     *
     * \retval true unlocked
     * \retval false failed
     */        
    static bool Unlock() {
        bool state = true;

#if !defined(_MSC_VER)
#if !defined(HAL_FLASH_MODULE_ENABLED)
        STM32F103X_SET_REG( STM32F103X_FLASH_KEYR, STM32F103X_FLASH_KEY1 );
        STM32F103X_SET_REG( STM32F103X_FLASH_KEYR, STM32F103X_FLASH_KEY2 );

#else // defined(HAL_FLASH_MODULE_ENABLED)
        if (HAL_OK != HAL_FLASH_Unlock()) {
            state = false;
        }
#endif // defined(HAL_FLASH_MODULE_ENABLED)
#endif // !defined(_MSC_VER)
        return state;
    } // Unlock()

        
    /**
     * Erase N pages from given start location.  The start location lower bits are masked off to pageSize.  Flash must be unlocked
     *
     * \param[in] page_address Pointer to destination flash address, start of page
     * \param[in] pages Count for erase
     * \param[in] page_size_u32 Page size in sizeof(uint32_t) multiples, default \ref STM32F103X_FLASH_PAGE_SIZE_U32
     * \retval true erase success
     * \retval false erase failure
     */
    static bool ErasePages(const uint32_t *page_address, const int32_t pages, const uint32_t page_size_u32 = STM32F103X_FLASH_PAGE_SIZE_U32) {
        uint32_t *pa = const_cast<uint32_t *>(page_address);
        int32_t p = pages;
        bool done = true;

        while(p>0) {
            done = Flash::ErasePage(pa, page_size_u32);
            if (!done) {
                break;
            }
            pa+=page_size_u32;
            p--;
        }

        return done;
    } // ErasePages(...)


    /**
     * Erase page by given start location.  The start location lower bits are masked off to pageSize.  Flash must be unlocked
     *
     * \param[in] page_address Pointer to destination flash address, start of page
     * \param[in] page_size_u32 Page size in sizeof(uint32_t) multiples, default \ref STM32F103X_FLASH_PAGE_SIZE_U32
     * \retval true on erase page success
     * \retval false erase failure
     */            
    static bool ErasePage(const uint32_t *page_address, const uint32_t page_size_u32 = STM32F103X_FLASH_PAGE_SIZE_U32) {
        bool done = Flash::CheckErasePage(page_address, page_size_u32);

        // Do we need to erase?
        if (!done) {
#if defined(_MSC_VER)
            uint32_t *pa = reinterpret_cast<uint32_t *>(reinterpret_cast<uint32_t>(const_cast<uint32_t *>(page_address)) & ~((page_size_u32<<2)-1));
            for(uint32_t i = 0; i < page_size_u32; i++) {
                pa[i] = STM32F103X_FLASH_NOR_ERASE_STATE;

                // Inject erase error
                if (erase_error_inject && ((rand() % 100)>50)) {
                    std::cout << std::endl << "ErErr @ " << std::hex << std::setw(8) << std::setfill('0') << &pa[i] << std::endl;
                    pa[i] = STM32F103X_FLASH_NOR_ERASE_STATE ^ STM32F103X_FLASH_NOR_ERASE_STATE;
                    erase_error_inject = false;
                }
            }
#else // !defined(_MSC_VER)
#if !defined(HAL_FLASH_MODULE_ENABLED)
            // Wait for anything outstanding
            while( STM32F103X_GET_REG( STM32F103X_FLASH_SR ) & STM32F103X_FLASH_SR_BSY ) { }
            
            uint32_t pageAddressMasked = reinterpret_cast<uint32_t>(page_address) & ~((page_size_u32<<2)-1);
            uint32_t rwmVal = STM32F103X_GET_REG( STM32F103X_FLASH_CR );

            rwmVal = STM32F103X_FLASH_CR_PER;
            STM32F103X_SET_REG( STM32F103X_FLASH_CR, rwmVal );
            while( STM32F103X_GET_REG( STM32F103X_FLASH_SR ) & STM32F103X_FLASH_SR_BSY ) { }

            STM32F103X_SET_REG( STM32F103X_FLASH_AR, pageAddressMasked );
            STM32F103X_SET_REG( STM32F103X_FLASH_CR, STM32F103X_FLASH_CR_START | STM32F103X_FLASH_CR_PER);
            while( STM32F103X_GET_REG( STM32F103X_FLASH_SR) & STM32F103X_FLASH_SR_BSY ) { }

            rwmVal = 0x00;
            STM32F103X_SET_REG( STM32F103X_FLASH_CR, rwmVal );
            
#else // defined(HAL_FLASH_MODULE_ENABLED)
            
            uint32_t pageError = 0;
            FLASH_EraseInitTypeDef eraseInit = {
                .TypeErase = FLASH_TYPEERASE_PAGES,
                .Banks = FLASH_BANK_1,
                .PageAddress = reinterpret_cast<uint32_t>(page_address) & ~((page_size_u32<<2)-1),
                .NbPages = 1
            };
            if (HAL_OK != HAL_FLASHEx_Erase(&eraseInit, &pageError)) {
                // Doesnt matter we check anyway...
            }
#endif // defined(HAL_FLASH_MODULE_ENABLED)
#endif // !defined(_MSC_VER)
            done = Flash::CheckErasePage(page_address, page_size_u32);
        }

        return done;
    } // ErasePage(...)


    /**
     * Check page is in erase state by given start location.  The start location lower bits are masked off to pageSize
     *
     * \param[in] page_address Pointer to destination flash address, start of page
     * \param[in] page_size_u32 Page size in sizeof(uint32_t) multiples, default \ref STM32F103X_FLASH_PAGE_SIZE_U32
     * \return bool Status
     * \retval true when page in erase state
     * \retval false not all page data in erase state
     */            
    static bool CheckErasePage(const uint32_t *page_address, const uint32_t page_size_u32 = STM32F103X_FLASH_PAGE_SIZE_U32) {
        bool ok = true;
        uint32_t l = page_size_u32, *pf = reinterpret_cast<uint32_t *>(reinterpret_cast<uint32_t>(const_cast<uint32_t *>(page_address)) & ~((page_size_u32<<2)-1));

        for(uint32_t i=0; i<l; i++) {
            if (STM32F103X_FLASH_NOR_ERASE_STATE != pf[i]) {
                ok = false;
                break;
            }
        }

        return ok;
    } // CheckErasePage(...)

}; // class Flash

const uint32_t Flash::page_size = STM32F103X_FLASH_PAGE_SIZE;
const uint32_t Flash::flash_size = STM32F103X_FLASH_SIZE;
uint32_t* const Flash::flash_start = reinterpret_cast<uint32_t* const>(STM32F103X_FLASH_SHADOW_START);
uint32_t* const Flash::flash_end = static_cast<uint32_t * const>(Flash::flash_start + (Flash::flash_size>>2));
#if defined(_MSC_VER)
bool Flash::write_error_inject = false;
bool Flash::erase_error_inject = false;
#endif // defined(_MSC_VER)

} // namespace stm32f103x

#endif // defined(ARDUINO_ARCH_STM32)

#endif // !FLASHSTM32F103C_H
