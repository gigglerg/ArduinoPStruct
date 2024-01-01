/**
 * \file
 * MCU wrapper classes for STM32F103X peripherals
 * PROJECT: PStruct library
 * TARGET SYSTEM: Maple Mini
 */

#ifndef CMEDIAWRAP_H
#define CMEDIAWRAP_H

#if defined(ARDUINO_ARCH_STM32)

#include "media.h"                // Persist base

// These are what we are wrapping...
#include "stm32/f103/flash.h"
#include "stm32/f103/crc.h"

namespace wrap {
/**
 * Wrapper for MCU specialised API to flash module and CRC.  Each media type 
 * requires a different wrapper
 */
class Flash : public persist::Media, protected stm32f103x::Crc {
public:
    uint32_t GetPageSize() const {
        return stm32f103x::Flash::page_size;
    }
        
    uint32_t GetSize() const {
        return stm32f103x::Flash::flash_size;
    }
        
    uint32_t* const GetStart() const {
        return stm32f103x::Flash::flash_start;
    }
        
    uint32_t* const GetEnd() const {
        return stm32f103x::Flash::flash_end;
    }

    bool Program(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32,
                            const uint32_t page_size_u32, const bool use_lock) {
        // For realtime os, add lock as required
        return stm32f103x::Flash::Program( buffer, data, size_u32, page_size_u32, use_lock );
    }
        
    bool Read(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32) {
        // For realtime os, add lock as required
        return stm32f103x::Flash::Read( buffer, data, size_u32 );
    }

    uint32_t Crc(const uint32_t *buffer, const uint16_t size_u16) {
        return stm32f103x::Crc::Generate(buffer, static_cast<uint32_t>(size_u16));
    }

#if defined(_MSC_VER)
    void InjectWriteError(const bool state) const {
        stm32f103x::Flash::write_error_inject = state;
    }

    void InjectEraseError(const bool state) const {
        stm32f103x::Flash::erase_error_inject = state;
    }
#endif // defined(_MSC_VER)
}; // class Flash
} // namespace wrap

#endif // ARDUINO_ARCH_STM32

#endif // !CMEDIAWRAP_H
