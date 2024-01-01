/**
 * \file
 * H/W CRC API for STM32F103x
 * PROJECT: PStruct library
 * TARGET SYSTEM: Maple Mini
 */

#ifndef STM32F103CCRC_H
#define STM32F103CCRC_H

#if defined(ARDUINO_ARCH_STM32)

/* Is in users build_opt.h?  if not use software */
#if !defined(HAL_CRC_MODULE_ENABLED)
#include "../../sw/crc.h"
#endif /* !defined(HAL_CRC_MODULE_ENABLED) */

namespace stm32f103x {

#if defined(HAL_CRC_MODULE_ENABLED)
/**
 * A class offering h/w CRC module access via API for device STM32F103C8T6
 *
 * \note Class name intentionally generic
 * \attention Ether use OpenSTM32 HAL or for Arduino install library Arduino_Core_STM32 
 * https://github.com/stm32duino/Arduino_Core_STM32/
 * \attention HAL_CRC_MODULE_ENABLED macro must be defined to use this class
 */
class Crc {
#else // !defined(HAL_CRC_MODULE_ENABLED)
/**
 * \copydoc swimp::Crc
 */
class Crc : public swimp::Crc {
#endif // !defined(HAL_CRC_MODULE_ENABLED)
protected:
#if defined(HAL_CRC_MODULE_ENABLED)
    /**
     * Helper for OpenSTM32 get HAL handle of hardware CRC device
     *
     * \return Pointer to CRC device handle structure
     */
    static CRC_HandleTypeDef* GetHalHandlePtr() {
        static CRC_HandleTypeDef h_crc = { .Instance = reinterpret_cast<CRC_TypeDef*>(CRC_BASE) };

        return &h_crc;
    }
#endif // !defined(HAL_CRC_MODULE_ENABLED)

public:
    /**
     * Setup and enable hardware CRC module.  For Arduino with library Arduino_Core_STM32 this will 
     * be done for you
     */
    static void Setup() {
#if defined(HAL_CRC_MODULE_ENABLED)
        __HAL_RCC_CRC_CLK_ENABLE();
        HAL_CRC_Init( GetHalHandlePtr() );
#endif // defined(HAL_CRC_MODULE_ENABLED)
    }

#if defined(HAL_CRC_MODULE_ENABLED)
    /**
     * Generate CRC32 using on-chip hardware module
     *
     * \param[in] buffer Pointer to data
     * \param[in] length_u32 Length of data in sizeof(uint32_t) multiples
     * \return CRC16 numeric
     */
    static uint32_t Generate(const uint32_t *buffer, const uint16_t length_u32) {
        /* Algorithm     Poly        Init        RefIn   RefOut  XorOut     
         * CRC-32/BZIP2  0x04C11DB7  0xFFFFFFFF  false   false   0xFFFFFFFF
         */
        return HAL_CRC_Calculate( GetHalHandlePtr(), const_cast<uint32_t*>(buffer), static_cast<uint32_t>(length_u32) ) ^ 0xffffffffUL;
    }
#endif // !defined(HAL_CRC_MODULE_ENABLED)
}; // class Crc

} // namespace stm32f103x

#endif // defined(ARDUINO_ARCH_STM32)

#endif // !STM32F103CCRC_H
