#include "../cogs_mikai.h"
#include <furi_hal_rtc.h>

void cogs_mikai_scene_quick_charge_on_enter(void* context) {
    COGSMyKaiApp* app = context;

    if(!app->mykey.is_loaded) {
        cogs_mikai_show_error_popup(app, "No card loaded\nRead a card first");
        scene_manager_previous_scene(app->scene_manager);
        return;
    }

    cogs_mikai_show_loading_popup(app, "Quick Charge", "Setting amount...");

    DateTime datetime;
    furi_hal_rtc_get_datetime(&datetime);

    uint16_t cents = app->settings.quick_charge_cents;
    bool success = mykey_set_cents(
        &app->mykey,
        cents,
        datetime.day,
        datetime.month,
        datetime.year - 2000);

    if(success) {
        cogs_mikai_show_loading_popup(app, "Quick Charge", "Writing to card...");
        if(mykey_write_to_nfc(app)) {
            // Build success message with the actual amount
            char msg[48];
            snprintf(msg, sizeof(msg), "Done! %u.%02u EUR set", cents / 100, cents % 100);
            cogs_mikai_show_success_popup(app, msg);
        } else {
            cogs_mikai_show_error_popup(app, "Failed to write\nto card");
        }
    } else {
        cogs_mikai_show_error_popup(app, "Failed to set\ncredit");
    }

    scene_manager_search_and_switch_to_previous_scene(app->scene_manager, COGSMyKaiSceneStart);
}

bool cogs_mikai_scene_quick_charge_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void cogs_mikai_scene_quick_charge_on_exit(void* context) {
    UNUSED(context);
}
