/**
 * \file
 * Generic persistent storage base class
 * PROJECT: PStruct library
 * TARGET SYSTEM: Arduino, AVR, Maple Mini
 */

#ifndef PERSISTMEDIA_H
#define PERSISTMEDIA_H


namespace persist {

/**
 * Media description.  Base class offering interface to persistent storage media via API, device independent.
 * Each supported media type will implement this abstract class.
 */
class Media {
public:
    /**
     * Get media page size
     *
     * \return Bytes
     */
    virtual uint32_t GetPageSize() const = 0;


    /**
     * Get media storage size.  Based upon media internal start and end pointers
     * 
     * \return Bytes
     */
    virtual uint32_t GetSize() const = 0;

    
    /**
     * Get start location of media
     *
     * \return Pointer.  Numeric may not represent a valid CPU address
     */
    virtual uint32_t* const GetStart() const = 0;


    /**
     * Get end location of media
     *
     * \return Pointer.  Numeric may not represent a valid CPU address
     */
    virtual uint32_t* const GetEnd() const = 0;


    /**
     * Program media with given data.  This may be multi stage where in the case of flash, page 
     * erase(s) are required.
     *
     * \note Implement media mutex or critical section within your own wrapper class if using 
     * an OS or tasker
     *
     * \param[in] buffer pointer to write location on media
     * \param[in] data pointer to source data to program
     * \param[in] size_u32 size of source data, sizeof(uint32_t) multiples
     * \param[in] page_size_u32 page size, sizeof(uint32_t) multiples
     * \param[in] use_lock architecture specific memory region lock.  If true will leave
     * memory locked afterwards
     * \retval true on success
     * \retval false on failure.  This could be an erase or program operation that failed
     */
    virtual bool Program(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32,
                            const uint32_t page_size_u32, const bool use_lock) = 0;
            

    /**
     * Read media data
     *
     * \note Implement media mutex or critical section within your own wrapper class if using 
     * an OS or tasker
     *
     * \param[in] buffer pointer to source location on media
     * \param[in] data pointer to destination for read data
     * \param[in] size_u32 size of source data, sizeof(uint32_t) multiples
     * \retval true on success
     * \retval false on failure.  Anything from no access to media to bad buffer address
     */
    virtual bool Read(const uint32_t *buffer, const uint32_t *data, const int16_t size_u32) = 0;


    /**
     * CRC generator helper.  Primarily used as part of low level storage validation and 
     * has been exposed to allow hardware implementations or alternative algorithm.
     *
     * \param[in] buffer pointer to source data (in memory)
     * \param[in] size_u16 size of source data, sizeof(uint32_t) multiples
     * \return CRC numeric (algorithm specific)
     */            
    virtual uint32_t Crc(const uint32_t *buffer, const uint16_t size_u16) = 0;

}; // class Media

} // namespace persist

#endif // PERSISTMEDIA_H
