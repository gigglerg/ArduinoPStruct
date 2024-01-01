/**
 * \file
 * Persistent structure with wear levelling
 * PROJECT: PStruct library
 * TARGET SYSTEM: Arduino, AVR, Maple Mini
 */

#ifndef PERSISTSTRUCT_H
#define PERSISTSTRUCT_H

#if defined(_MSC_VER)
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#endif // defined(_MSC_VER)


namespace persist {

/**
 * Macro should be defined in limited memory environments, giving access to private instance of custom data type T wrapped by class cDB otherwise
 * standard pratice is to hide this data requiring caller to provide own memory copy.
 */
//#define PERSISTSTRUCT_POINTERS


/**
 * Macro to calculate raw memory size based upon user structure, page size and required ware level
 *
 * \param[in] s user ADR/structure
 * \param[in] ps page size (Bytes)
 * \param[in] l ware levelling
 * \return required memory size (Bytes)
 */
#define PERSISTSTRUCT_SIZE(s,ps,l)              ((persist::Struct<s>::GetStorageUnitSize() / ps) + \
                                                    ((persist::Struct<s>::GetStorageUnitSize() % ps) ? 1 : 0) * ps * l)

/**
 * A class offering persistent storage access to a user supplied structure with ware levelling.
 * Internally the supplied user ADT &lt;T&gt; is wrapped with a header that includes retrieval 
 * meta data allowing multiple copies to be stored on media.  The latest is loaded or in case 
 * of corruption progressively older copies of ADT depending upon what was stored over time.
 *
 * \attention If your ADT fields are changed it is good practice to save it more than once so 
 * a load failure at least has a chance at returning a usable (new format) structure
 *
 * \tparam T ADT type name managed and storaged by PStruct instance
 */
template<typename T>
class Struct {
/*! \cond PRIVATE */
protected:
    /**
     * Wrapper for ADT written to persistent storage for type T
     */
    class Db {
    protected:
#pragma pack(push, 4) // Optimised for ARM
        /**
         * Data block ADT header used to manage data type T
         */
        struct tDbHead {
            uint32_t crc;        // Optimize for architecture?
            uint32_t counter;
            uint32_t bytes;
        };

        /**
         * Data block ADT structure.  Raw storage format written to media
         */
        struct tDb {
            union {
                struct {
                    tDbHead meta;
                    T        data;
                }f;
                // Length of write structure determined by the following n uint32_t's
                uint32_t u32[((sizeof(tDbHead) + sizeof(T)) / sizeof(uint32_t)) + (((sizeof(tDbHead) + sizeof(T)) % sizeof(uint32_t)) ? 1 : 0)];
            };
        };
#pragma pack(pop)
        /**
         * Internal data block used for read/write operations
         */
        tDb        db_;

    public:
        /**
         * Default constructor, clear internal data block
         */
        Db() {
            Clear();
        }


        /**
         * Get data block header size
         *
         * \return Size in Bytes
         */
        static constexpr uint32_t GetDbHeadSize() {
            return sizeof(struct tDbHead);
        } // GetDbHeadSize()


        /**
         * Get data block size
         *
         * \return Size in Bytes
         */
        static constexpr uint32_t GetDbSize() {
            return sizeof(tDb);
        } // GetDbSize()


        /**
         * Clear internal data block
         *
         * \param[in] set_word clear byte value, default -1
         */
        void Clear(const uint32_t set_word = 0xffffffffUL) {
            for(uint32_t i = sizeof(tDbHead) / sizeof(uint32_t); i<sizeof(db_.u32) / sizeof(uint32_t); i++) {
                db_.u32[i] = set_word;
            }
            db_.f.meta.bytes = db_.f.meta.crc = db_.f.meta.counter = 0;
        } // Clear()


#if defined(PERSISTSTRUCT_POINTERS)
        /**
         * Get internal data block pointer
         *
         * \attention While bad pratice it maybe necessary in resticted memory environments
         * \return ADT &lt;T&gt; pointer
         */
        T* Get() const {
            return const_cast<T*>(&db_.f.data);
        } // Get()


        /**
         * Update media as internal data block changes exist.  Assumes caller updated data directly via pointer from \ref Get
         *
         * \attention While bad pratice it maybe necessary in resticted memory environments
         */
        void Update(Media &m, bool first=false) {
            if (first) {
                db_.f.meta.counter = 0;
            }else {
                db_.f.meta.counter++;
            }
            db_.f.meta.bytes = sizeof(db_.u32);
            db_.f.meta.crc = CalculateCRC(m);
        } // Update(...)
#else // !PERSISTSTRUCT_POINTERS


        /**
         * Get copy of internal data block
         *
         * \param[in] t reference to destination ADT
         */
        void Get(T& t) const {
            t = db_.f.data;
        } // Get(...)
        

        /**
         * Update media with supplied data block ADT, assumes changes exist
         *
         * \param[in,out] m media instance reference
         * \param[in] t reference to source ADT
         * \param[in] first default false.  First call where internal write counter zeroed when true
         */
         void Update(Media &m, T& t, const bool first=false) {
            if (first) {
                db_.f.meta.counter = 0;
            }else {
                db_.f.meta.counter++;
            }
            db_.f.meta.bytes = sizeof(db_.u32);
            db_.f.data = t;
            db_.f.meta.crc = CalculateCRC(m);
        } // Update(...)
#endif // !PERSISTSTRUCT_POINTERS


        /**
         * Get counter.  Debug function, returns internal write or ware level counter
         *
         * \note Didn't want to expose the counter but no other easy way for caller to check if a record is newer
         * \return Read/write count n
         */
        uint32_t GetCounter() const {
            return db_.f.meta.counter;
        } // GetCounter()


        /**
         * Query, will internal data block ADT fit on storage media
         *
         * \param[in,out] m media instance reference
         * \param[in] pages n count of ADT
         * \retval true on fit success
         * \retval false, ADT won't fit
         */
        bool WillFit(Media &m, uint8_t pages) const {
            return sizeof(db_.u32) <= pages * m.GetPageSize();
        } // WillFit(...)


        /**
         * Read internal data block ADT from media at given location
         *
         * \param[in,out] m media instance reference
         * \param[in] location pointer to location on media, numeric may not represent a valid CPU address
         */
        bool Read(Media &m, uint32_t* location) {
            bool ok = false;
            const uint32_t *data = static_cast<uint32_t *>(&db_.u32[0]);

            // Load from storage + check crc?
            if (m.Read(location, data, sizeof(db_.u32)>>2) && IsValid(m)) {
                ok = true;
            }else {
                // Read from storage failed so not valid
                db_.f.meta.bytes = db_.f.meta.crc = 0;
            }
            
            return ok;
        } // Read(...)


        /**
         * Read internal data block ADR header.  Use the header information once read to decide if the entire 
         * data block should be read.
         *
         * \param[in,out] m media instance reference
         * \param[in] location pointer to location on media, numeric may not represent a valid CPU address
         * \retval true read successful
         * \retval false read failure
         */
        bool ReadHeader(Media &m, uint32_t* location) {
            bool ok = false;
            const uint32_t *data = static_cast<uint32_t *>(&db_.u32[0]);

            db_.f.meta.counter = 0;
            
            // Load header from storage + check?
            if (m.Read(location, data, sizeof(tDbHead) / sizeof(uint32_t)) && (db_.f.meta.bytes == sizeof(db_.u32))) {
                ok = true;
            }else {
                // Read from storage failed so not valid
                db_.f.meta.bytes = db_.f.meta.crc = 0;
            }
            
            return ok;
        } // ReadHeader(...)


        /**
         * Write internal data block ADT to media at location.  After write to media of entire ADT, CRC used to check
         *
         * \param[in,out] m media instance reference
         * \param[in] location pointer to location on media, numeric may not represent a valid CPU address
         * \retval true write success
         * \retval false write failure
         */
        bool Write(Media &m, uint32_t* location) {
            const uint32_t *data = static_cast<uint32_t *>(&db_.u32[0]);

            // Check crc? + Save to storage
            return IsValid(m) && m.Program(location, data, sizeof(db_.u32)>>2, m.GetPageSize()>>2, true);
        } // Write(...)


    protected:

        /**
         * Query, is internal data block ADT value.  Uses header raw data content to determine if loaded data valid
         *
         * \param[in,out] m media instance reference
         * \retval true valid
         * \retval false invalid
         */
        bool IsValid(Media &m) const {
            return ((db_.f.meta.bytes == sizeof(db_.u32)) && (CalculateCRC(m) == db_.f.meta.crc));
        } // IsValid(...)
    

        /**
         * Calculate CRC of internal data block ADT.  Algorithm is architecture dependant and includes the data
         * header.
         *
         * \param[in,out] m media instance reference
         * \return CRC numeric of data block
         */
        uint32_t CalculateCRC(Media &m) const {
            uint32_t crc = 0;
            const uint32_t *buffer = (const_cast<uint32_t *>(&db_.u32[sizeof(tDbHead) / sizeof(uint32_t)]));

            if (db_.f.meta.bytes == sizeof(db_.u32)) {
                crc = m.Crc(buffer, (db_.f.meta.bytes - sizeof(tDbHead)) / sizeof(uint32_t));
            }

            return crc;
        } // CalculateCRC(...)
    }; // class DB
/*! \endcond */

public:
    /**
     * Constructor based upon required ware level. You will have to load your ADT via \ref Load
     *
     * \param[in,out] m media instance reference
     * \param[in] start location or offset into media for first write.  Numeric may not represent 
     * a valid CPU address.
     * \param[in] ware_level ware levels N (maximum)
     */
    Struct(Media &m, uint32_t *start, uint8_t ware_level) : media_(m), start_(start), db_(), ware_level_(ware_level) {
        current_.loaded = false;
        current_.location = 0;
        pages_ = Struct::GetStorageUnitSize() / media_.GetPageSize();
        if (Struct::GetStorageUnitSize() % media_.GetPageSize()) {
            pages_++;
        }
        struct_pages_u32_ = pages_ * (media_.GetPageSize()>>2);
        pages_ *= ware_level_;
    } // Struct(...)


    /**
     * Constructor based upon start and end pointers covering range for storage. You will have to 
     * load your ADT via \ref Load
     *
     * \param[in,out] m media instance reference
     * \param[in] start location or offset into media for first write.  Numeric may not represent 
     * a valid CPU address.
     * \param[in] end location or offset into media for first write.  Numeric may not represent 
     * a valid CPU address.  Should be > start.
     */
    Struct(Media &m, uint32_t *start, uint32_t *end) : media_(m), start_(start), db_() {
        current_.loaded = false;
        current_.location = 0;
        uint32_t ps = Struct::GetStorageUnitSize() / media_.GetPageSize();
        if (Struct::GetStorageUnitSize() % media_.GetPageSize()) {
            ps++;
        }
        struct_pages_u32_ = ps * (media_.GetPageSize()>>2);
        pages_ = (reinterpret_cast<uint32_t>(end) - reinterpret_cast<uint32_t>(start)) / media_.GetPageSize();
        pages_-= pages_ % ps;
        ware_level_ = pages_ / ps;
    } // Struct(...)


    /**
     * Unload internal data block ADT.  Clears data and loaded state
     */
    void Unload() {
        db_.Clear();
        current_.loaded = false;
        current_.location = 0;
    } // Unload()


#if defined(PERSISTSTRUCT_POINTERS)
    /**
     * Load internal data block ADT from media
     *
     * \return bool Loaded status
     */
    bool Load() {
#else // !PERSISTSTRUCT_POINTERS
    /**
     * Load data block ADT from media
     *
     * \param[out] data block         ADT
     * \retval true loaded
     * \retval false not loaded
     */
    bool Load(T &data) {
#endif // !PERSISTSTRUCT_POINTERS
        bool ok = false;
        uint32_t *l;

        // Make sure size(T + header) <= storage size?
        if (db_.WillFit(media_, pages_)) {
            // Loaded already?
            if (current_.loaded) {
                // Reload current
                if (db_.Read(media_, current_.location)) {
#if !defined(PERSISTSTRUCT_POINTERS)
                    db_.Get(data);    // Take data
#endif // !PERSISTSTRUCT_POINTERS
                    ok = true;
                }else {
                    // Force cold load
                    current_.loaded = false;
                }
            }

            // Not loaded? (cold load?)
            if (!current_.loaded) {
                uint32_t c = 0, s = 0, i = pages_;

                // Attempt to find valid data block with largest counter (header only, crc may still be invalid)
                // this should be the last written block.
                l = start_;
                do {
                    if (db_.ReadHeader(media_, l)) {
                        if (!s || db_.GetCounter() > c) {
                            c = db_.GetCounter();
                            l = GetNextLocation(l);
                            s = 1;
                        }else {
                            // This next block has a lower or equal counter than at least the last one we found so assume
                            break;
                        }
                    }else {
                        l = GetNextLocation(l);
                    }
                }while(--i>0);
                
                // We find any?
                if (s) {
                    /* We should have the most recent header (last written).  now check crc to validate and work
                       backwards until a crc is valid and take. */
                    i = pages_;
                    do {
                        l = GetPreviousLocation(l);

                        // Load at l
                        if (db_.Read(media_, l)) {
                            current_.loaded = true;
                            current_.location = l;
#if !defined(PERSISTSTRUCT_POINTERS)
                            db_.Get(data);    // Take data
#endif // !PERSISTSTRUCT_POINTERS
                            ok = true;
                            break;
                        }
#if defined(_MSC_VER)
                        else {
                            std::cout << std::endl << "db_.read(...) failed @ " << std::hex << std::setw(8) << std::setfill('0') << (l - start_) << std::endl;
                        }
#endif // defined(_MSC_VER)
                    }while(--i>0);
                }
            }
        }

        return ok;
    } // Load(...)


#if defined(PERSISTSTRUCT_POINTERS)
    /**
     * Save internal data block ADT to media
     *
     * \note You can save an ADT more than once but remember only do so when absolutely required as each save 
     * reduces life
     *
     * \note If fields change in your ADT like because of a firmware update, save it more than once to ensure 
     * you have recover options for the latest structure layout.  You may want to add a version field to 
     * allow firmware compatibility
     *
     * \note If you want more than one ADT stored, create a wrap class and new instance of this class to handle it
     *
     * \param[in] not_loaded_force default false.  Required if not loaded to force save of given ADT to media.
     * Normally this parameter default would be used but when virgin media this flag is required for inital 
     * write.
     * \retval true on success
     * \retval false failed
     */
    bool Save(bool not_loaded_force=false) {
#else // !PERSISTSTRUCT_POINTERS
    /**
     * Save data block ADT to media
     *
     * \note You can save an ADT more than once but remember only do so when absolutely required as each save 
     * reduces life
     *
     * \note If fields change in your ADT like because of a firmware update, save it more than once to ensure 
     * you have recover options for the latest structure layout.  You may want to add a version field to 
     * allow firmware compatibility
     *
     * \note If you want more than one ADT stored, create a wrap class and new instance of this class to handle it
     *
     * \param[in] data block ADT.  Internally this will be wrapped to include a header
     * \param[in] not_loaded_force default false.  Required if not loaded to force save of given ADT to media
     * Normally this parameter default would be used but when virgin media this flag is required for inital 
     * write.
     * \retval true on success
     * \retval false failed
     */
    bool Save(T &data, bool not_loaded_force=false) {
#endif // !PERSISTSTRUCT_POINTERS
        bool done = false;
        bool attempt = false;
        uint32_t *l = NULL;        // No write

        // Make sure size(T + header) <= storage size?
        if (db_.WillFit(media_, pages_)) {
            // If not loaded and force (assume storage empty.  it might be the user invoked api incorrectly but ...)
            if (!current_.loaded) {
                if (not_loaded_force) {
                    l = start_;
#if defined(PERSISTSTRUCT_POINTERS)
                    db_.Update(media_, true);    // Counter=0
#else // !PERSISTSTRUCT_POINTERS
                    db_.Update(media_, data, true);    // Counter=0
#endif // !PERSISTSTRUCT_POINTERS
                    attempt = true;
                }
            }else {
                l = GetNextLocation(current_.location);    // Next
#if defined(PERSISTSTRUCT_POINTERS)
                db_.Update(media_);        // Counter++
#else // !PERSISTSTRUCT_POINTERS
                db_.Update(media_, data);    // Counter++
#endif // !PERSISTSTRUCT_POINTERS
                attempt = true;
            }
                
            // Attempt write?
            if (attempt) {
                uint32_t i = pages_;

                // Exclude overwritting current.  Rare situation where all write attempts fail we at least want something to load, i.e. the current
                if (current_.loaded) {
                    i--;
                }

                // Write attempt loop
                do {
                    // Write/verify at location l ok?
                    if (db_.Write(media_, l)) {
                        // OK and we're done...
                        done = true;
                        current_.loaded = true;    // Must be...
                        current_.location = l;
                        break;
                    }else {
#if defined(_MSC_VER)
                        std::cout << std::endl << "db_.write(...) failed @ " << std::hex << std::setw(8) << std::setfill('0') << (l - start_) << std::endl;
#endif // defined(_MSC_VER)
                        // Write/verify failed, move location
                        l = GetNextLocation(l);    // Next
                    }
                }while(--i>0);
            }
        }

        return done;
    } // Save(...)


#if defined(PERSISTSTRUCT_POINTERS)
    /**
     * Get internal data block.  This doe not include internal storage header and will be 
     * your ADT type T
     * 
     * \attention While bad pratice it maybe necessary in resticted memory environments
     * \return ADT &lt;T&gt; pointer
     */
    T* Get() const {
        return db_.Get();
    } // Get()
#endif // !PERSISTSTRUCT_POINTERS

    /**
     * Query internal data block ADT been loaded from media
     * 
     * \retval true loaded
     * \retval false not loaded
     */
    bool IsLoaded() const {
        return current_.loaded;
    } // IsLoaded()


    /**
     * Get internal data block ADT storage size
     *
     * \return Size, Bytes
     */
    static constexpr uint32_t GetStorageUnitSize() {
        return Db::GetDbSize();
    } // GetStorageUnitSize()


    /**
     * Get number of raw storage media pages required to write internal data block
     *
     * \return N pages
     */
    static constexpr uint32_t GetStorageUnitPages(const uint32_t page_size) {
        uint32_t pc = Struct::GetStorageUnitSize() / page_size;
        if (Struct::GetStorageUnitSize() % page_size) {
            pc++;
        }
        return page_size * pc;
    } // GetStorageUnitPages(...)


    /**
     * Get wave levels.  Essentially how many possible loads of previous saved ADTs
     * are possible.  As media used this figure will increase
     *
     * \note Use as debugging aid
     *
     * \return N
     */
    uint32_t GetWareLevels() const {
        return ware_level_;
    } // GetWareLevels()


    /**
     * Get raw storage media page count allocated for read/write operations
     *
     * \note Use as debugging aid
     *
     * \return N pages
     */
    uint32_t GetPages() const {
        return pages_;
    } // GetPages()


    /**
     * Get location on media of currently loaded or last written internal data block ADT
     *
     * \note Use as debugging aid
     *
     * \return Pointer.  Numeric may not represent a valid CPU address.
     */
    uint32_t* GetLocation() const {
        return current_.location;
    } // GetLocation()


    /**
     * Get ware levelling counter of currently loaded internal data block ADT
     *
     * \return N
     */
    uint32_t GetCounter() const {
        return db_.GetCounter();
    }

/*! \cond PRIVATE */
protected:
    /**
     * Get last storage or previous location from given location.  Used during loading to aid
     * finding a suitable block for use.
     *
     * \param l from location pointer.  Numeric may not represent a valid CPU address.
     */
    uint32_t* GetPreviousLocation(uint32_t *l) const {
        uint32_t *t = start_ + pages_ * (media_.GetPageSize()>>2) - struct_pages_u32_;    // top

        if (l > start_) {
            l-= struct_pages_u32_;
        }else {
            l = t;
        }

        return l;
    } // GetPreviousLocation(...)


    /**
     * Get next storage or next location from given location.  Used during loading and saving to aid
     * access to storage media.
     *
     * \param l from location pointer.  Numeric may not represent a valid CPU address.
     */
    uint32_t* GetNextLocation(uint32_t *l) const {
        uint32_t *t = start_ + pages_ * (media_.GetPageSize()>>2) - struct_pages_u32_;    // top
                                    
        if (l < t) {
            l+= struct_pages_u32_;
        }else {
            l = start_;
        }

        return l;
    } // GetNextLocation(...)


protected:
    struct {
        bool        loaded;
        uint32_t*    location;
    }current_;
    Media&        media_;
    uint32_t*    start_;
    uint32_t    struct_pages_u32_;
    uint32_t    pages_;
    uint32_t    ware_level_;
    Db            db_;
/*! \endcond */
}; // class Struct

} // namespace persist

#endif // PERSISTSTRUCT_H
