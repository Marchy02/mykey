#include "cogs_mikai.h"
#include <furi.h>
#include <machine/endian.h>
#include <nfc/nfc.h>
#include <nfc/protocols/st25tb/st25tb.h>
#include <nfc/protocols/st25tb/st25tb_poller_sync.h>

bool mykey_read_from_nfc(COGSMyKaiApp* app) {
    FURI_LOG_I(TAG, "Reading SRIX4K from NFC...");

    bool success = false;
    Nfc* nfc = nfc_alloc();

    // Detect ST25TB card type
    St25tbType type;
    St25tbError error = st25tb_poller_sync_detect_type(nfc, &type);

    if(error != St25tbErrorNone) {
        FURI_LOG_E(TAG, "Failed to detect ST25TB card: %d", error);
        nfc_free(nfc);
        return false;
    }

    // Check if it's SRIX4K (ST25TBX512 or ST25TB04K or ST25TBX4K)
    if(type != St25tbTypeX512 && type != St25tbType04k && type != St25tbTypeX4k) {
        FURI_LOG_E(TAG, "Card is not SRIX4K compatible, type: %d", type);
        nfc_free(nfc);
        return false;
    }

    FURI_LOG_I(TAG, "Detected ST25TB card type: %d", type);

    // Allocate ST25TB data structure
    St25tbData* st25tb_data = st25tb_alloc();

    // Read entire card
    error = st25tb_poller_sync_read(nfc, st25tb_data);

    if(error != St25tbErrorNone) {
        FURI_LOG_E(TAG, "Failed to read ST25TB card: %d", error);
        st25tb_free(st25tb_data);
        nfc_free(nfc);
        return false;
    }

    // Extract UID (8 bytes for ST25TB)
    // ST25TB UID bytes are in order [0..7], we need to assemble them big-endian
    // to match libmikai: uid[0] is MSB (bits 56-63), uid[7] is LSB (bits 0-7)
    app->mykey.uid = 0;
    for(size_t i = 0; i < ST25TB_UID_SIZE && i < 8; i++) {
        app->mykey.uid |= ((uint64_t)st25tb_data->uid[i]) << ((7 - i) * 8);
    }

    FURI_LOG_I(TAG, "Card UID (big-endian): %016llX", app->mykey.uid);
    FURI_LOG_I(TAG, "UID bytes: %02X %02X %02X %02X %02X %02X %02X %02X",
        st25tb_data->uid[0], st25tb_data->uid[1], st25tb_data->uid[2], st25tb_data->uid[3],
        st25tb_data->uid[4], st25tb_data->uid[5], st25tb_data->uid[6], st25tb_data->uid[7]);

<<<<<<< HEAD
    // Copy blocks to MyKey data structure.
    // ST25TB stores blocks little-endian; bswap32 converts to the big-endian
    // format expected by the MIKAI algorithm.
=======
    // Copy blocks to MyKey data structure
    // ST25TB blocks need byte-swapping to match libmikai's big-endian format
>>>>>>> origin/master
    size_t num_blocks = st25tb_get_block_count(type);
    if(num_blocks > SRIX4K_BLOCKS) {
        num_blocks = SRIX4K_BLOCKS;
    }

<<<<<<< HEAD
=======
    // Backup current data to detect changes later
    uint32_t old_eeprom[SRIX4K_BLOCKS];
    memcpy(old_eeprom, app->mykey.eeprom, sizeof(old_eeprom));

>>>>>>> origin/master
    for(size_t i = 0; i < num_blocks; i++) {
        app->mykey.eeprom[i] = __bswap32(st25tb_data->blocks[i]);
    }

    // Keep a clean baseline so the write path can skip unchanged blocks.
    memcpy(app->mykey.eeprom_baseline, app->mykey.eeprom, sizeof(app->mykey.eeprom));

    FURI_LOG_I(TAG, "Blocks loaded (%zu) and baseline saved", num_blocks);

    // Derive the session encryption key from UID + vendor blocks.
    mykey_calculate_encryption_key(&app->mykey);

<<<<<<< HEAD
    // Update cached state.
    app->mykey.is_loaded   = true;
    app->mykey.is_modified = false;
    app->mykey.is_reset    = mykey_is_reset(&app->mykey);
    app->mykey.current_credit = mykey_get_current_credit(&app->mykey);

    // Silent auto-backup immediately after read — zero user friction.
    mykey_auto_backup(app);

    FURI_LOG_I(TAG, "Card ready. Credit: %u cents | Reset: %s",
=======
    // Update cached values
    app->mykey.is_loaded = true;
    app->mykey.is_modified = false;
    app->mykey.is_reset = mykey_is_reset(&app->mykey);
    app->mykey.current_credit = mykey_get_current_credit(&app->mykey);

    // Silent auto-backup (no popup, no interruption)
    mykey_auto_backup(app);

    FURI_LOG_I(TAG, "Card loaded. Credit: %d cents, Reset: %s",
>>>>>>> origin/master
               app->mykey.current_credit,
               app->mykey.is_reset ? "yes" : "no");

    success = true;

    st25tb_free(st25tb_data);
    nfc_free(nfc);

    return success;
}

bool mykey_write_to_nfc(COGSMyKaiApp* app) {
    FURI_LOG_I(TAG, "Writing to SRIX4K via NFC...");

    if(!app->mykey.is_loaded) {
        FURI_LOG_E(TAG, "No card data loaded, cannot write");
        return false;
    }

    bool success = true;
    Nfc* nfc = nfc_alloc();

    // Detect ST25TB card type
    St25tbType type;
    St25tbError error = st25tb_poller_sync_detect_type(nfc, &type);

    if(error != St25tbErrorNone) {
        FURI_LOG_E(TAG, "Failed to detect ST25TB card: %d", error);
        nfc_free(nfc);
        return false;
    }

    // Check if it's SRIX4K
    if(type != St25tbTypeX512 && type != St25tbType04k && type != St25tbTypeX4k) {
        FURI_LOG_E(TAG, "Card is not SRIX4K compatible, type: %d", type);
        nfc_free(nfc);
        return false;
    }

    size_t num_blocks = st25tb_get_block_count(type);
    if(num_blocks > SRIX4K_BLOCKS) {
        num_blocks = SRIX4K_BLOCKS;
    }

<<<<<<< HEAD
    // Write only blocks that actually changed relative to the baseline captured
    // at read time. Block 0 is the UID (read-only on SRIX4K) — always skip it.
    size_t written = 0;
    size_t skipped = 0;

    for(size_t i = 1; i < num_blocks; i++) {
        // Skip unchanged blocks to reduce NFC transaction time and wear.
        if(app->mykey.eeprom[i] == app->mykey.eeprom_baseline[i]) {
            skipped++;
            continue;
        }

=======
    // Write each block (skipping block 0 as it's typically the UID/Read-only)
    for(size_t i = 1; i < num_blocks; i++) {
>>>>>>> origin/master
        uint32_t block_to_write = __bswap32(app->mykey.eeprom[i]);
        
        // OPTIMIZATION: Skip writing if the block is already the same on the card
        // Note: This requires a fresh read before writing if we want to be 100% sure,
        // but since we usually read right before editing, it's a safe bet for performance.
        // For now, let's just write all blocks that are different from what we READ initially.
        
        error = st25tb_poller_sync_write_block(nfc, i, block_to_write);

        if(error != St25tbErrorNone) {
            FURI_LOG_E(TAG, "Block 0x%02zX write failed: %d", i, error);
            success = false;
<<<<<<< HEAD
            // Continue writing the remaining blocks; partial writes are better
            // than aborting mid-transaction and leaving the card inconsistent.
        } else {
            written++;
=======
>>>>>>> origin/master
        }
    }

    FURI_LOG_I(TAG, "Write done: %zu written, %zu skipped (unchanged)", written, skipped);

    if(!success) {
        FURI_LOG_W(TAG, "One or more blocks failed to write — card may be inconsistent");
    }

    nfc_free(nfc);

    return success;
}
