#include "cogs_mikai.h"
#include <furi.h>
#include <furi_hal_rtc.h>
#include <storage/storage.h>
#include <string.h>

#define SETTINGS_PATH "/ext/apps_data/cogs_mikai/settings.mykconf"
#define BACKUP_DIR "/ext/apps_data/cogs_mikai/backups"
#define SETTINGS_HEADER "MYKEY_SETTINGS_V1"
#define QUICK_CHARGE_DEFAULT 1900

// ──────────────────────────────────────────────
//  Settings: Load
// ──────────────────────────────────────────────
void mykey_settings_load(COGSMyKaiApp* app) {
    // Apply defaults first
    app->settings.quick_charge_cents = QUICK_CHARGE_DEFAULT;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t file_size = storage_file_size(file);
        char* buf = malloc(file_size + 1);

        if(buf) {
            size_t bytes_read = storage_file_read(file, buf, file_size);
            buf[bytes_read] = '\0';

            if(strncmp(buf, SETTINGS_HEADER, strlen(SETTINGS_HEADER)) == 0) {
                char* ptr = strstr(buf, "QUICK_CHARGE: ");
                if(ptr) {
                    uint32_t value = 0;
                    ptr += strlen("QUICK_CHARGE: ");
                    while(*ptr >= '0' && *ptr <= '9') {
                        value = value * 10 + (*ptr++ - '0');
                    }
                    if(value > 0 && value <= 99999) {
                        app->settings.quick_charge_cents = (uint16_t)value;
                    }
                }
            }
            free(buf);
        }
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    FURI_LOG_I(TAG, "Settings loaded: quick_charge=%u cents", app->settings.quick_charge_cents);
}

// ──────────────────────────────────────────────
//  Settings: Save
// ──────────────────────────────────────────────
void mykey_settings_save(COGSMyKaiApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, "/ext/apps_data/cogs_mikai");

    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FuriString* line = furi_string_alloc();
        furi_string_printf(
            line,
            "%s\nQUICK_CHARGE: %u\n",
            SETTINGS_HEADER,
            app->settings.quick_charge_cents);
        storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line));
        furi_string_free(line);
        storage_file_close(file);
        FURI_LOG_I(TAG, "Settings saved: quick_charge=%u cents", app->settings.quick_charge_cents);
    } else {
        FURI_LOG_W(TAG, "Failed to save settings");
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

// ──────────────────────────────────────────────
//  Auto-Backup (silent, no popups)
// ──────────────────────────────────────────────
void mykey_auto_backup(COGSMyKaiApp* app) {
    if(!app->mykey.is_loaded) return;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, "/ext/apps_data/cogs_mikai");
    storage_simply_mkdir(storage, BACKUP_DIR);

    // Build filename: backups/YYYYMMDD_HHMMSS_LLLLLLLL.myk
    DateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);

    FuriString* path = furi_string_alloc();
    furi_string_printf(
        path,
        "%s/%04d%02d%02d_%02d%02d%02d_%08lX.myk",
        BACKUP_DIR,
        datetime.year,
        datetime.month,
        datetime.day,
        datetime.hour,
        datetime.minute,
        datetime.second,
        (uint32_t)(app->mykey.uid & 0xFFFFFFFF));

    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, furi_string_get_cstr(path), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        const char* header = "COGES_MYKEY_V1\n";
        storage_file_write(file, header, strlen(header));

        FuriString* line = furi_string_alloc();

        furi_string_printf(line, "UID: %016llX\n", app->mykey.uid);
        storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line));

        furi_string_printf(line, "ENCRYPTION_KEY: %08lX\n", app->mykey.encryption_key);
        storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line));

        for(size_t i = 0; i < SRIX4K_BLOCKS; i++) {
            furi_string_printf(line, "BLOCK_%03zu: %08lX\n", i, app->mykey.eeprom[i]);
            storage_file_write(file, furi_string_get_cstr(line), furi_string_size(line));
        }

        furi_string_free(line);
        storage_file_close(file);
        FURI_LOG_I(TAG, "Auto-backup saved: %s", furi_string_get_cstr(path));
    } else {
        FURI_LOG_W(TAG, "Auto-backup failed: %s", furi_string_get_cstr(path));
    }

    furi_string_free(path);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}
