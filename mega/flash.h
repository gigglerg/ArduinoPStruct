/**
 * \file
 * Flash API for AVR (Atmega devices)
 * PROJECT: PStruct library
 * TARGET SYSTEM: Arduino, AVR
 */

#ifndef FLASHAVR_H
#define FLASHAVR_H

#if defined(ARDUINO_ARCH_AVR)

#include <stdint.h>
#if defined(_MSC_VER)
#include <cstdlib>
#include <iostream>
#include <iomanip>
#else // defined(!_MSC_VER)
//#include <avr/boot.h>
#include "optiboot.h"
#include <avr/pgmspace.h>
#endif // defined(!_MSC_VER)


namespace avr8mega {
#if defined(_MSC_VER)
// Windows test harness memory
extern uint8_t* ::gpFlashU8;
/**
 * Device NOR flash start address (bank0)
 */
#define AVR_FLASH_START                         ::gpFlashU8

/**
 * Device flash end address Bytes.  This is for bank0 only
 */
#define FLASHEND                                (FSIZE_U8-1)
#define SPM_PAGESIZE                            TEST_PAGE_SIZE
#else // !defined(_MSC_VER)
#if !defined(AVR_FLASH_START)
    #define AVR_FLASH_START                     0x0000U
#endif // !defined(AVR_FLASH_START)
#endif // !defined(_MSC_VER)

#if !defined(AVR_FLASH_SIZE)
/**
 * Device flash size Bytes.  This is for bank0 only
 */
#define AVR_FLASH_SIZE                          (1+FLASHEND)
#endif // !defined(AVR_FLASH_SIZE)

#if !defined(AVR_FLASH_PAGE_SIZE)
/**
 * Device page size in Bytes.  This is for bank0 only
 */
#define AVR_FLASH_PAGE_SIZE                     (SPM_PAGESIZE)    // Bytes
#endif // !defined(AVR_FLASH_PAGE_SIZE)

#if !defined(AVR_FLASH_NOR_ERASE_STATE)
/**
 * Device NOR flash erase state uint32_t numeric
 */
#define AVR_FLASH_NOR_ERASE_STATE               0xffffU
#endif  // !defined(AVR_FLASH_NOR_ERASE_STATE)

#if !defined(_MSC_VER)
/**
 * Device flash size Bytes
 */
#define    USHRT_MAX_FLASH                      USHRT_MAX
#endif // !defined(_MSC_VER)

/**
 * A driver class offering flash access via API for AVR (MEGA) to allow for persistent data
 *
 * \attention This class requires Arduino bootloader and library Optiboot_flasher.  Your device 
 * should have the Optiboot_flasher written to boot loader section of AVR flash.  This is 
 * required due to SPI instruction and fuse settings.
 * https://github.com/MCUdude/MiniCore/tree/master/avr/libraries/Optiboot_flasher
 */
class Flash {
public:
    static const uint16_t page_size;    /// Device page size (Bytes)
    static const uint16_t flash_size;   /// Device flash size (Bytes), bank0 only
    static uint16_t* const flash_start; /// Device flash start address
    static uint16_t* const flash_end;   /// Device flash end address (top, non-accessible)

    /**
     * Program N pages starting at buffer with given data to sizeU8.   Will only program if data not already written 
     * and will only erase pages if they are not in erase state.  Data written is verified as part of write.
     * If start buffer location not paged aligned, extra pages will be erased.
     *
     * \param[in] buffer Pointer to destination flash address of program
     * \param[in] data Pointer to data to write
     * \param[in] size_u8 Data size of program / sizeof(uint8_t)
     * \param[in] page_size_u8 Device page size, multiples of sizeof(uint8_t).  Default \ref AVR_FLASH_PAGE_SIZE
     * \retval true on program success
     * \retval false failure
     */
    static bool Program(const uint16_t* buffer, const uint16_t* data, const int16_t size_u8, \
                            const uint16_t page_size_u8 = AVR_FLASH_PAGE_SIZE) {
        bool done = true;
        uint8_t sreg = SREG;

        // buffer size and page size not 0?
        if (!size_u8 || !page_size_u8) {
            done = false;
        }

        // disable interrupts + wait for any ee transaction
        cli();
        
        // do we need to program?
        if (done && !Verify(buffer, data, size_u8)) {
            uint32_t bam = reinterpret_cast<uint32_t>(buffer) & ~((page_size>>1)-1);
            uint32_t be = reinterpret_cast<uint32_t>(buffer) + (size_u8>>1);
            
            // page count
            uint16_t pc = size_u8 / page_size_u8;

            if (size_u8 % page_size_u8) {
                pc++;    // use next page, size not page aligned
            }
            // if actual end write is on another page, increment
            bam+=pc * page_size_u8;
            if (be > bam) {
                pc++;    // use next page, buffer end location not page aligned
            }
            
            // erase all required pages for buffer program operation
            done = ErasePages(buffer, pc, page_size_u8);
            if (done) {
                // write buffer to erased pages
                done = Write16Buffer(buffer, data, size_u8);
            }
        }

        // re-enable interrupts (if they were ever enabled)
        SREG = sreg;

        return done;
    } // Program(...)


    /**
     * Read page data starting at buffer to size into data
     *
     * \param[in] buffer Pointer to source flash address
     * \param[out] data Pointer to data, destination of read
     * \param[in] size_u8 Data size / sizeof(uint8_t)
     * \retval true on read success
     * \retval false on failure
     */
    static bool Read(const uint16_t* buffer, uint16_t* data, const uint16_t size_u8) {
        uint16_t s = size_u8>>1;
#if !defined(_MSC_VER)
        uint16_t pf = reinterpret_cast<uint16_t>(const_cast<uint16_t*>(buffer));
#else
        uint32_t pf = reinterpret_cast<uint32_t>(const_cast<uint16_t*>(buffer));
#endif
        for(uint16_t i=0; i<s; i++, pf+=2) {
#if defined(pgm_read_word_far)
            data[i] = pgm_read_word_far(pf);
#else // !defined(pgm_read_word_far)
            data[i] = pgm_read_word(pf);
#endif // !defined(pgm_read_word_far)
        }
                
        return true;
    } // Read(...)


    /**
     * Verify given buffer in flash
     *
     * \param[in] buffer Pointer to source flash address
     * \param[in] data Pointer to data to verify against
     * \param[in] size_u8 Data size / sizeof(uint8_t)
     * \retval true when data represents what is on media
     * \retval false one or more differences between data and media
     */
    static bool Verify(const uint16_t* buffer, const uint16_t* data, const uint16_t size_u8) {
        uint16_t d, l = static_cast<uint16_t>(size_u8>>1);
#if !defined(_MSC_VER)
        uint16_t pf = reinterpret_cast<uint16_t>(const_cast<uint16_t*>(buffer));
#else
        uint32_t pf = reinterpret_cast<uint32_t>(const_cast<uint16_t*>(buffer));
#endif

        for(uint16_t i=0; i<l; i++, pf+=2) {
#if defined(pgm_read_word_far)
            d = pgm_read_word_far(pf);
#else // !defined(pgm_read_word_far)
            d = pgm_read_word(pf);
#endif // !defined(pgm_read_word_far)
            if (d != data[i]) {
                return false;
            }
        }
            
        return true;
    } // Verify(...)


#if defined(_MSC_VER)
    static void PrintBuffer(char *s, const uint16_t *b, uint16_t l) {
        std::cout << std::endl << s << std::endl << std::hex << std::setw(8) << std::setfill('0') << b << ": ";
        for(uint32_t i=0, p=1; i<l; i++) {
            if (i!=0 && (i % 16)==0) {
                std::cout << std::endl << std::hex << std::setw(8) << std::setfill('0') << (&b[i]) << ": ";
            }
            std::cout << std::hex << std::setw(4) << std::setfill('0') << b[i] << " ";
        }
        std::cout << std::endl;
    } // PrintBuffer(...)
#endif // defined(_MSC_VER)


    /**
     * Write buffer in 16bit words to flash.  Flash location prior must be in erase state and flash unlocked
     *
     * \param[in] buffer Pointer to destination flash address
     * \param[in] data Pointer to source data
     * \param[in] size_u8 Data size of program / sizeof(uint8_t)
     * \param[in] page_size_u8 Device page size, multiples of sizeof(uint8_t).  Default \ref AVR_FLASH_PAGE_SIZE
     * \retval true write success
     * \retval false write failure
     */
    static bool Write16Buffer(const uint16_t* buffer, const uint16_t* data, const uint16_t size_u8, const uint16_t page_size_u8 = AVR_FLASH_PAGE_SIZE) {
        uint16_t *b = const_cast<uint16_t*>(buffer), *pb;
        uint16_t *d = const_cast<uint16_t *>(data);
        uint16_t s = size_u8>>1, ps;

        for(;s>0;) {
            for(pb = b, ps = page_size_u8/sizeof(uint16_t);ps>0; s--, d++, b++, ps--) {
                if (!s) {
                    break;    // final page, end of data
                }
                optiboot_page_fill(reinterpret_cast<optiboot_addr_t>(b), d[0]);
            }

            // write buffer to flash + wait for completion
            optiboot_page_write(reinterpret_cast<optiboot_addr_t>(pb));
        }
        
        // veriy data
        return Verify(buffer, data, size_u8);
    } // Write16Buffer(...)


    /**
     * Erase N pages from given start location.  The start location lower bits are masked off to pageSize.  Flash must be unlocked
     *
     * \param[in] page_address Pointer to destination flash address, start of page
     * \param[in] pages Count for erase
     * \param[in] page_size_u8 Page size in sizeof(uint8_t) multiples, default \ref AVR_FLASH_PAGE_SIZE
     * \retval true erase success
     * \retval false erase failure
     */
    static bool ErasePages(const uint16_t* page_address, const uint16_t pages, const uint16_t page_size_u8 = AVR_FLASH_PAGE_SIZE) {
        uint16_t * pa = const_cast<uint16_t*>(page_address);
        uint16_t p = pages;
        bool done = true;
        
        while(p>0) {
            done = Flash::ErasePage(pa, page_size_u8);
            if (!done) {
                break;
            }
            pa+=page_size_u8/sizeof(uint16_t);
            p--;
        }

        return done;
    } // ErasePages(...)


    /**
     * Erase page by given start location.  The start location lower bits are masked off to pageSize.  Flash must be unlocked
     *
     * \param[in] page_address Pointer to destination flash address, start of page
     * \param[in] page_size_u8 Page size in sizeof(uint32_t) multiples, default \ref AVR_FLASH_PAGE_SIZE
     * \retval true on erase page success
     * \retval false erase failure
     */            
    static bool ErasePage(const uint16_t* page_address, const uint16_t page_size_u8 = AVR_FLASH_PAGE_SIZE) {
        bool done;

        boot_spm_busy_wait();
        done = Flash::CheckErasePage(page_address, page_size_u8);

        // do we need to erase?
        if (!done) {
            // erase page + wait
            optiboot_page_erase(reinterpret_cast<optiboot_addr_t>(page_address));
            
            done = Flash::CheckErasePage(page_address, page_size_u8);
        }

        return done;
    } // ErasePage(...)


    /**
     * Check page is in erase state by given start location.  The start location lower bits are masked off to pageSize
     *
     * \param[in] page_address Pointer to destination flash address, start of page
     * \param[in] page_size_u8 Page size in sizeof(uint8_t) multiples, default \ref AVR_FLASH_PAGE_SIZE
     * \return bool Status
     * \retval true when page in erase state
     * \retval false not all page data in erase state
     */            
    static bool CheckErasePage(const uint16_t* page_address, const uint16_t page_size_u8 = AVR_FLASH_PAGE_SIZE) {
        bool ok = true;
        uint32_t pf = reinterpret_cast<uint32_t>(const_cast<uint16_t*>(page_address))>>0 & ~((page_size_u8)-1);
        uint16_t d;
        
        for(uint16_t i=0; i<page_size_u8; i+=2) {
#if defined(pgm_read_word_far)
            d = pgm_read_word_far(pf+i);
#else // !defined(pgm_read_word_far)
            d = pgm_read_word(pf+i);
#endif // !defined(pgm_read_word_far)
            if (static_cast<uint16_t>(AVR_FLASH_NOR_ERASE_STATE) != d) {
                ok = false;
                break;
            }
        }

        return ok;
    } // CheckErasePage(...)

}; // class Flash

const uint16_t Flash::page_size = AVR_FLASH_PAGE_SIZE;
const uint16_t Flash::flash_size = AVR_FLASH_SIZE;
uint16_t* const Flash::flash_start = reinterpret_cast<uint16_t* const>(AVR_FLASH_START);
uint16_t* const Flash::flash_end = static_cast<uint16_t* const>(Flash::flash_start + (Flash::flash_size>>1));
} // namespace avr8mega

#endif // defined(ARDUINO_ARCH_AVR)

#endif // !FLASHAVR_H
