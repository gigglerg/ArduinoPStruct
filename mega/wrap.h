/**
 * \file
 * MCU wrapper classes for AVR8 Mega peripherals
 * PROJECT: PStruct library
 * TARGET SYSTEM: Arduino, AVR
 */

#ifndef CMEDIAWRAP_H
#define CMEDIAWRAP_H

#if defined(ARDUINO_ARCH_AVR)

#include "media.h"                // nPersist base

// these are what we are wrapping...
#include "mega/flash.h"
#include "sw/crc.h"


namespace wrap {

/**
 * Wrapper for MCU specialised API to EEPROM module and CRC.  Each media type 
 * requires a different wrapper
 *
 * \tparam PSZ EEPROM page size (Bytes).  This is made up for the AVR EEPROM to suit structure storage size
 */
template<uint16_t PSZ>
class Ee : public persist::Media, protected swimp::Crc {
public:
    uint32_t GetPageSize() const {
        return PSZ;
    }

    uint32_t GetSize() const {
        return E2END+1;
    }

    uint32_t* const GetStart() const {
        return 0;
    }

    uint32_t* const GetEnd() const {
        return GetStart() + (GetSize()>>2);
    }

    bool Program(const uint32_t* buffer, const uint32_t* data, const int16_t size_u32,
                                const uint32_t page_size_u32, const bool use_lock) {
        // For realtime os, add lock as required
        bool ok = true;
        uint32_t *b = const_cast<uint32_t*>(buffer);
        uint32_t l = static_cast<uint32_t>(size_u32), i, v;

        (void)page_size_u32;
        (void)use_lock;

        // Program/verify loop, bomb out on fail
        for(i=0; i<l; i++) {
            eeprom_update_dword(&b[i],data[i]);
            v = eeprom_read_dword(&b[i]);
            if (v != data[i]) {
                ok = false;
                break;
            }
        }

        return ok;
    } // Program(...)
        
    bool Read(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32) {
        eeprom_read_block(const_cast<uint32_t*>(data), const_cast<uint32_t*>(buffer), size_u32*sizeof(uint32_t));

        return true;
    } // Read(...)

    uint32_t Crc(const uint32_t *buffer, const uint16_t size_u16) {
        return swimp::Crc::Generate(buffer, static_cast<uint32_t>(size_u16));
    }
}; // class Ee


/**
 * Wrapper for MCU specialised API to flash module and CRC.  Each media type 
 * requires a different wrapper
 */
class Flash : public persist::Media, protected swimp::Crc {
    public:
        uint32_t GetPageSize() const {
            return SPM_PAGESIZE;
        }
        
        uint32_t GetSize() const {
            return avr8mega::Flash::flash_size;
        }
        
        uint32_t* const GetStart() const {
            return reinterpret_cast<uint32_t *>(const_cast<uint16_t*>(avr8mega::Flash::flash_start));
        }
        
        uint32_t* const GetEnd() const {
            return reinterpret_cast<uint32_t *>(const_cast<uint16_t*>(avr8mega::Flash::flash_end));
        }

        bool Program(const uint32_t* buffer, const uint32_t* data, const int16_t size_u32,
                                const uint32_t page_size_u32, const bool use_lock) {
            return avr8mega::Flash::Program(reinterpret_cast<uint16_t*>(const_cast<uint32_t*>(buffer)), \
                                            reinterpret_cast<uint16_t*>(const_cast<uint32_t*>(data)), size_u32*sizeof(uint32_t), page_size_u32*sizeof(uint32_t));
        }
        
        bool Read(const uint32_t* buffer, const uint32_t *data, const int16_t sizeu32) {
            return avr8mega::Flash::Read(reinterpret_cast<uint16_t*>(const_cast<uint32_t*>(buffer)), \
                                            reinterpret_cast<uint16_t*>(const_cast<uint32_t*>(data)), sizeu32*sizeof(uint32_t));
        }
        
        uint32_t Crc(const uint32_t *buffer, const uint16_t length_u32) {
            return swimp::Crc::Generate(buffer, static_cast<uint32_t>(length_u32));
        }
}; // class Flash
} // namespace wrap

#endif // ARDUINO_ARCH_AVR

#endif // !CMEDIAWRAP_H
