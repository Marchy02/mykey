#include "../cogs_mikai.h"

static void cogs_mikai_scene_read_popup_callback(void* context) {
    COGSMyKaiApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, 0);
}

void cogs_mikai_scene_read_on_enter(void* context) {
    COGSMyKaiApp* app = context;

    // Show popup for card detection
    cogs_mikai_show_loading_popup(app, "Reading Card", "Place COGES MyKey\non Flipper's back");
    popup_set_callback(app->popup, cogs_mikai_scene_read_popup_callback);

    // Attempt to read NFC card
    if(mykey_read_from_nfc(app)) {
        // Data initialization (credit calculation, encryption key) is now handled inside mykey_read_from_nfc
        cogs_mikai_show_success_popup(app, "Success!\nCard read successfully");
    } else {
        cogs_mikai_show_error_popup(app, "Error\nFailed to read card");
    }
}

bool cogs_mikai_scene_read_on_event(void* context, SceneManagerEvent event) {
    COGSMyKaiApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
        scene_manager_previous_scene(app->scene_manager);
    }

    return consumed;
}

void cogs_mikai_scene_read_on_exit(void* context) {
    COGSMyKaiApp* app = context;
    popup_reset(app->popup);
}
