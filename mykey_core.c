#include "cogs_mikai.h"
#include <furi.h>
#include <string.h>
#include <machine/endian.h>
#include <storage/storage.h>

// Encode or decode a MyKey block using XOR bit manipulation.
<<<<<<< HEAD
// Core MIKAI algorithm: 3-stage symmetric bit-permutation within a 32-bit word.
// Applying it twice is a no-op (self-inverse), so encode == decode.
static inline void encode_decode_block(uint32_t* block) {
    // Stage 1: rotate paired nibble groups outward
    *block ^= (*block & 0x00C00000) << 6 | (*block & 0x0000C000) << 12 | (*block & 0x000000C0) << 18 |
              (*block & 0x000C0000) >> 6 | (*block & 0x00030000) >> 12 | (*block & 0x00000300) >> 6;
    // Stage 2: rotate high nibble groups inward
    *block ^= (*block & 0x30000000) >> 6 | (*block & 0x0C000000) >> 12 | (*block & 0x03000000) >> 18 |
              (*block & 0x00003000) << 6 | (*block & 0x00000030) << 12 | (*block & 0x0000000C) << 6;
    // Stage 3: mirror of stage 1 to close the permutation
=======
// This is the core MIKAI algorithm. It rearranges bits within the 32-bit block.
static inline void encode_decode_block(uint32_t* block) {
    // Stage 1: Initial bit swapping
    *block ^= (*block & 0x00C00000) << 6 | (*block & 0x0000C000) << 12 | (*block & 0x000000C0) << 18 |
              (*block & 0x000C0000) >> 6 | (*block & 0x00030000) >> 12 | (*block & 0x00000300) >> 6;
    // Stage 2: Middle bit swapping
    *block ^= (*block & 0x30000000) >> 6 | (*block & 0x0C000000) >> 12 | (*block & 0x03000000) >> 18 |
              (*block & 0x00003000) << 6 | (*block & 0x00000030) << 12 | (*block & 0x0000000C) << 6;
    // Stage 3: Repeat initial swap (symmetric property)
>>>>>>> origin/master
    *block ^= (*block & 0x00C00000) << 6 | (*block & 0x0000C000) << 12 | (*block & 0x000000C0) << 18 |
              (*block & 0x000C0000) >> 6 | (*block & 0x00030000) >> 12 | (*block & 0x00000300) >> 6;
}

// Public wrapper for debug purposes
void mykey_encode_decode_block(uint32_t* block) {
    encode_decode_block(block);
}

// Calculate checksum of a generic block
static inline void calculate_block_checksum(uint32_t* block, const uint8_t block_num) {
    uint8_t checksum = 0xFF - block_num - (*block & 0x0F) - (*block >> 4 & 0x0F) -
                       (*block >> 8 & 0x0F) - (*block >> 12 & 0x0F) - (*block >> 16 & 0x0F) -
                       (*block >> 20 & 0x0F);

    // Clear first byte and set to checksum value (bits 24-31)
    *block &= 0x00FFFFFF;
    *block |= (uint32_t)checksum << 24;
}

// Helper to update a block with checksum, encoding and optional XOR
static void update_block_optimized(
    MyKeyData* key,
    uint8_t block_num,
    uint32_t value,
    bool encode,
    uint32_t xor_mask) {
    uint32_t block = value;
    calculate_block_checksum(&block, block_num);
    if(encode) {
        encode_decode_block(&block);
    }
    block ^= xor_mask;
    key->eeprom[block_num] = block;
}

// Return the number of days between 1/1/1995 and a specified date
static uint32_t days_difference(uint8_t day, uint8_t month, uint16_t year) {
    if(month < 3) {
        year--;
        month += 12;
    }
    return year * 365 + year / 4 - year / 100 + year / 400 + (month * 153 + 3) / 5 + day - 728692;
}

// Get current transaction offset from block 0x3C
static uint8_t get_current_transaction_offset(MyKeyData* key) {
    uint32_t block3C = key->eeprom[MYKEY_BLOCK_TX_POINTER];

    if(block3C == 0xFFFFFFFF) {
        return 0x07;
    }

    uint32_t current = block3C ^ (key->eeprom[MYKEY_BLOCK_KEY_ID] & 0x00FFFFFF);
    encode_decode_block(&current);

    uint8_t offset = (current >> 16) & 0xFF;
    return (offset > 0x07) ? 0x07 : offset;
}

// Calculate the encryption key and save the result in key struct
void mykey_calculate_encryption_key(MyKeyData* key) {
    FURI_LOG_I(TAG, "=== Encryption Key Calculation ===");
    FURI_LOG_I(TAG, "UID (as stored): 0x%016llX", key->uid);

    // OTP calculation (reverse block 6 + 1, incremental. 1,2,3, etc.)
    uint32_t block6 = key->eeprom[0x06];
    FURI_LOG_I(TAG, "Block 0x06 raw: 0x%08lX", block6);

    uint32_t block6_reversed = __bswap32(block6);
    FURI_LOG_I(TAG, "Block 0x06 reversed: 0x%08lX", block6_reversed);

    uint32_t otp = ~block6_reversed + 1;
    FURI_LOG_I(TAG, "OTP (~reversed + 1): 0x%08lX", otp);

    // Encryption key calculation
    // MK = UID * VENDOR
    // SK (Encryption key) = MK * OTP
    uint32_t block18_raw = key->eeprom[0x18];
    uint32_t block19_raw = key->eeprom[0x19];
    FURI_LOG_I(TAG, "Block 0x18 raw: 0x%08lX", block18_raw);
    FURI_LOG_I(TAG, "Block 0x19 raw: 0x%08lX", block19_raw);

    uint32_t block18 = block18_raw;
    uint32_t block19 = block19_raw;
    encode_decode_block(&block18);
    encode_decode_block(&block19);
    FURI_LOG_I(TAG, "Block 0x18 decoded: 0x%08lX", block18);
    FURI_LOG_I(TAG, "Block 0x19 decoded: 0x%08lX", block19);

    uint64_t vendor = (((uint64_t)block18 << 16) | (block19 & 0x0000FFFF)) + 1;
    FURI_LOG_I(TAG, "Vendor: 0x%llX", vendor);

    // Calculate encryption key: UID * vendor * OTP
    // UID is now correctly stored in big-endian format, no swapping needed
    key->encryption_key = (key->uid * vendor * otp) & 0xFFFFFFFF;
    FURI_LOG_I(TAG, "Encryption Key: 0x%08lX", key->encryption_key);
    FURI_LOG_I(TAG, "===================================");
}

// Check if MyKey is reset (no vendor bound)
bool mykey_is_reset(MyKeyData* key) {
    static const uint32_t block18_reset = 0x8FCD0F48;
    static const uint32_t block19_reset = 0xC0820007;
    return key->eeprom[0x18] == block18_reset && key->eeprom[0x19] == block19_reset;
}

// Get block value
uint32_t mykey_get_block(MyKeyData* key, uint8_t block_num) {
    if(block_num >= SRIX4K_BLOCKS) return 0;
    return key->eeprom[block_num];
}

// Modify block
void mykey_modify_block(MyKeyData* key, uint32_t block, uint8_t block_num) {
    if(block_num < SRIX4K_BLOCKS) {
        key->eeprom[block_num] = block;
    }
}

// Extract current credit using libmikai method (block 0x21) - PRIMARY METHOD
uint16_t mykey_get_current_credit(MyKeyData* key) {
    FURI_LOG_I(TAG, "=== Credit Decoding (libmikai method) ===");

    // Use libmikai approach: read from block 0x21
    uint32_t block21_raw = key->eeprom[0x21];
    FURI_LOG_I(TAG, "Block 0x21 raw: 0x%08lX", block21_raw);
    FURI_LOG_I(TAG, "  Bytes: [%02X %02X %02X %02X]",
        (uint8_t)(block21_raw & 0xFF),
        (uint8_t)((block21_raw >> 8) & 0xFF),
        (uint8_t)((block21_raw >> 16) & 0xFF),
        (uint8_t)((block21_raw >> 24) & 0xFF));

    FURI_LOG_I(TAG, "Encryption key: 0x%08lX", key->encryption_key);

    uint32_t after_xor = block21_raw ^ key->encryption_key;
    FURI_LOG_I(TAG, "After XOR: 0x%08lX", after_xor);

    uint32_t current_credit = after_xor;
    encode_decode_block(&current_credit);
    FURI_LOG_I(TAG, "After encode_decode: 0x%08lX", current_credit);

    uint16_t credit_lower = current_credit & 0xFFFF;
    uint16_t credit_upper = (current_credit >> 16) & 0xFFFF;
    FURI_LOG_I(TAG, "Lower 16 bits: %u cents (%u.%02u EUR)",
        credit_lower, credit_lower / 100, credit_lower % 100);
    FURI_LOG_I(TAG, "Upper 16 bits: %u cents (%u.%02u EUR)",
        credit_upper, credit_upper / 100, credit_upper % 100);
    FURI_LOG_I(TAG, "=========================================");

    return credit_lower;
}

// Get credit from transaction history (for comparison/debugging)
uint16_t mykey_get_credit_from_history(MyKeyData* key) {
    uint32_t block3C = key->eeprom[0x3C];
    if(block3C == 0xFFFFFFFF) {
        return 0xFFFF; // No history available
    }

    // Decrypt block 0x3C to get starting offset
    uint32_t decrypted_3C = block3C ^ key->eeprom[0x07];
    uint32_t starting_offset = ((decrypted_3C & 0x30000000) >> 28) |
                               ((decrypted_3C & 0x00100000) >> 18);

    if(starting_offset >= 8) {
        return 0xFFFF; // Invalid offset
    }

    // Get most recent transaction (offset 8 in the circular buffer)
    // Blocks are already in big-endian format, credit is in lower 16 bits
    uint32_t txn_block = key->eeprom[0x34 + ((starting_offset + 8) % 8)];
    uint16_t credit = txn_block & 0xFFFF;

    FURI_LOG_D(TAG, "Credit from transaction history: %d cents", credit);
    return credit;
}

// Add N cents to MyKey actual credit
bool mykey_add_cents(MyKeyData* key, uint16_t cents, uint8_t day, uint8_t month, uint8_t year) {
    FURI_LOG_I(TAG, "=== Adding %u cents (%u.%02u EUR) ===", cents, cents / 100, cents % 100);

    if(mykey_is_reset(key)) {
        FURI_LOG_E(TAG, "Key is reset, cannot add credit");
        return false;
    }

    uint32_t block6 = key->eeprom[0x06];
    if(block6 == 0 || block6 == 0xFFFFFFFF) {
        FURI_LOG_E(TAG, "Key has no vendor");
        return false;
    }

    uint16_t precedent_credit;
    uint16_t actual_credit = mykey_get_current_credit(key);
    uint8_t current = get_current_transaction_offset(key);

    static const uint16_t RECHARGE_STEPS[] = {200, 100, 50, 20, 10, 5};

    do {
        precedent_credit = actual_credit;
        uint16_t step = 0;

        for(size_t i = 0; i < sizeof(RECHARGE_STEPS) / sizeof(RECHARGE_STEPS[0]); i++) {
            if(cents >= RECHARGE_STEPS[i]) {
                step = RECHARGE_STEPS[i];
                break;
            }
        }

        if(step == 0) {
            step = cents;
            cents = 0;
        } else {
            cents -= step;
        }
        actual_credit += step;

        current = (current == 7) ? 0 : current + 1;
        uint32_t txn_block = (uint32_t)day << 27 | (uint32_t)month << 23 |
                             (uint32_t)year << 16 | actual_credit;
        key->eeprom[MYKEY_BLOCK_TX_HISTORY_START + current] = txn_block;

        FURI_LOG_I(TAG, "Transaction %u: %u cents at block 0x%02X",
                   current, actual_credit, MYKEY_BLOCK_TX_HISTORY_START + current);
    } while(cents > 0);

    // Save credit to primary and secondary blocks
    update_block_optimized(key, MYKEY_BLOCK_CREDIT_PRIMARY, actual_credit, true, key->encryption_key);
    update_block_optimized(key, MYKEY_BLOCK_CREDIT_SECONDARY, actual_credit, true, key->encryption_key);

    // Save precedent credit to backup blocks
    update_block_optimized(key, MYKEY_BLOCK_CREDIT_PREV_1, precedent_credit, true, 0);
    update_block_optimized(key, MYKEY_BLOCK_CREDIT_PREV_2, precedent_credit, true, 0);

    // Save transaction pointer to block 3C
    update_block_optimized(key, MYKEY_BLOCK_TX_POINTER, (uint32_t)current << 16, true, key->eeprom[MYKEY_BLOCK_KEY_ID] & 0x00FFFFFF);

    // Increment operation counter
    uint32_t op_count = (key->eeprom[MYKEY_BLOCK_OP_COUNTER] & 0x00FFFFFF) + 1;
    key->eeprom[MYKEY_BLOCK_OP_COUNTER] = (key->eeprom[MYKEY_BLOCK_OP_COUNTER] & 0xFF000000) | (op_count & 0x00FFFFFF);

    key->is_modified = true;
    return true;
}

// Reset credit history and charge N cents
bool mykey_set_cents(MyKeyData* key, uint16_t cents, uint8_t day, uint8_t month, uint8_t year) {
    uint32_t dump[10];
    memcpy(dump, &key->eeprom[MYKEY_BLOCK_CREDIT_PRIMARY], SRIX_BLOCK_LENGTH);
    memcpy(dump + 1, &key->eeprom[MYKEY_BLOCK_TX_HISTORY_START], 9 * SRIX_BLOCK_LENGTH);

    update_block_optimized(key, MYKEY_BLOCK_CREDIT_PRIMARY, 0, true, key->encryption_key);
    memset(&key->eeprom[MYKEY_BLOCK_TX_HISTORY_START], 0xFF, 9 * SRIX_BLOCK_LENGTH);

    if(!mykey_add_cents(key, cents, day, month, year)) {
        memcpy(&key->eeprom[MYKEY_BLOCK_CREDIT_PRIMARY], dump, SRIX_BLOCK_LENGTH);
        memcpy(&key->eeprom[MYKEY_BLOCK_TX_HISTORY_START], dump + 1, 9 * SRIX_BLOCK_LENGTH);
        return false;
    }

    return true;
}

// Reset a MyKey to associate it with another vendor
void mykey_reset(MyKeyData* key) {
    FURI_LOG_I(TAG, "Resetting card to default vendor...");

    // 1. Calculate production date and elapsed days
    uint32_t production_date = key->eeprom[MYKEY_BLOCK_PRODUCTION_DATE];
    uint8_t pday = (production_date >> 28 & 0x0F) * 10 + (production_date >> 24 & 0x0F);
    uint8_t pmonth = (production_date >> 20 & 0x0F) * 10 + (production_date >> 16 & 0x0F);
    uint16_t pyear = (production_date & 0x0F) * 1000 + (production_date >> 4 & 0x0F) * 100 +
                    (production_date >> 12 & 0x0F) * 10 + (production_date >> 8 & 0x0F);
    uint32_t elapsed = days_difference(pday, pmonth, pyear);

    // 2. Production Date and key ID dependent blocks
    uint32_t key_id_full = key->eeprom[MYKEY_BLOCK_KEY_ID];
    uint32_t date_block = ((key_id_full & 0xFF000000) >> 8) |
                          (((elapsed / 1000 % 10) << 12) + ((elapsed / 100 % 10) << 8)) |
                          (((elapsed / 10 % 10) << 4) + (elapsed % 10));

    uint32_t date_bytes = (production_date & 0x0000FF00) << 8 |
                          (production_date & 0x00FF0000) >> 8 | (production_date & 0xFF000000) >> 24;

<<<<<<< HEAD
    // 3. Update the four mirror groups of 4 blocks each.
    // Groups start at: 0x10, 0x14, 0x3F, 0x43
    // Layout within each group: [date_block, key_id, op_counter, flags]
    static const uint8_t group_bases[] = {0x10, 0x14, 0x3F, 0x43};

    for(size_t m = 0; m < COUNT_OF(group_bases); m++) {
        uint8_t b = group_bases[m];
        update_block_optimized(key, b + 0, date_block,   false, 0);
        update_block_optimized(key, b + 1, key_id_full,  false, 0);
        update_block_optimized(key, b + 2, 1,            false, 0); // op counter = 1
        update_block_optimized(key, b + 3, 0x00040013,   false, 0);
=======
    // 3. Update blocks using optimized helper
    // Blocks 0x10, 0x11, 0x12, 0x13... and their mirrors
    const uint8_t mirror_offsets[] = {0x00, 0x04, 0x2F, 0x33}; // 0x10->0x10, 0x10->0x14, 0x10->0x3F, 0x10->0x43

    for(size_t m = 0; m < 4; m++) {
        uint8_t base = (m < 2) ? 0x10 : 0x3F;
        uint8_t off = mirror_offsets[m];

        update_block_optimized(key, base + (off % 4) + 0, date_block, false, 0); // 10, 14, 3F, 43
        update_block_optimized(key, base + (off % 4) + 1, key_id_full, false, 0); // 11, 15, 40, 44
        update_block_optimized(key, base + (off % 4) + 2, 1, false, 0); // 12, 16, 41, 45 (Op counter)
        update_block_optimized(key, base + (off % 4) + 3, 0x00040013, false, 0); // 13, 17, 42, 46
>>>>>>> origin/master
    }

    // Vendor and generic blocks
    update_block_optimized(key, MYKEY_BLOCK_VENDOR_1, 0x0000FEDC, true, 0);
    update_block_optimized(key, 0x1C, 0x0000FEDC, true, 0);
    update_block_optimized(key, 0x47, 0x0000FEDC, true, 0);
    update_block_optimized(key, 0x4B, 0x0000FEDC, true, 0);

    update_block_optimized(key, MYKEY_BLOCK_VENDOR_2, 0x00000123, true, 0);
    update_block_optimized(key, 0x1D, 0x00000123, true, 0);
    update_block_optimized(key, 0x48, 0x00000123, true, 0);
    update_block_optimized(key, 0x4C, 0x00000123, true, 0);

    // Production date mirror
    update_block_optimized(key, 0x22, date_bytes, true, 0);
    update_block_optimized(key, 0x26, date_bytes, true, 0);
    update_block_optimized(key, 0x51, date_bytes, true, 0);
    update_block_optimized(key, 0x55, date_bytes, true, 0);

    // Other mirror blocks
    const uint8_t other_blocks[] = {0x1A, 0x1B, 0x1E, 0x1F, 0x23, 0x27, 0x49, 0x4A, 0x4D, 0x4E, 0x50, 0x52, 0x54, 0x56};
    for(size_t i = 0; i < sizeof(other_blocks); i++) {
        update_block_optimized(key, other_blocks[i], 0, true, 0);
    }

    const uint8_t generic_blocks[] = {0x20, 0x24, 0x4F, 0x53};
    for(size_t i = 0; i < sizeof(generic_blocks); i++) {
        update_block_optimized(key, generic_blocks[i], 0x00010000, true, 0);
    }

    // Reset credit
    mykey_calculate_encryption_key(key);
    update_block_optimized(key, MYKEY_BLOCK_CREDIT_PRIMARY, 0, true, key->encryption_key);
    update_block_optimized(key, MYKEY_BLOCK_CREDIT_SECONDARY, 0, true, key->encryption_key);

    // Clear history
    memset(&key->eeprom[MYKEY_BLOCK_TX_HISTORY_START], 0xFF, 9 * SRIX_BLOCK_LENGTH);

    key->is_modified = true;
}


// Save raw card data to file for debugging
bool mykey_save_raw_data(COGSMyKaiApp* app, const char* path) {
    if(!app->mykey.is_loaded) {
        return false;
    }

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(!storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    // Write header
    FuriString* line = furi_string_alloc();
    furi_string_printf(line, "MyKey Raw Data Dump\n");
    furi_string_cat_printf(line, "UID: %016llX\n", (unsigned long long)app->mykey.uid);
    furi_string_cat_printf(line, "Encryption Key: 0x%08lX\n\n", app->mykey.encryption_key);
    storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line));

    // Write all blocks
    for(size_t i = 0; i < SRIX4K_BLOCKS; i++) {
        furi_string_printf(line, "Block 0x%02zX: 0x%08lX\n", i, app->mykey.eeprom[i]);
        storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line));
    }

    furi_string_free(line);
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return true;
}
