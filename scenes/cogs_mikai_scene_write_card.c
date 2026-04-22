#include "../cogs_mikai.h"

static void cogs_mikai_scene_write_card_popup_callback(void* context) {
    COGSMyKaiApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, 0);
}

void cogs_mikai_scene_write_card_on_enter(void* context) {
    COGSMyKaiApp* app = context;

    if(!app->mykey.is_loaded) {
        popup_set_callback(app->popup, cogs_mikai_scene_write_card_popup_callback);
        cogs_mikai_show_error_popup(app, "No card loaded");
        return;
    }

    if(!app->mykey.is_modified) {
        popup_set_callback(app->popup, cogs_mikai_scene_write_card_popup_callback);
        cogs_mikai_show_error_popup(app, "No changes detected\nCard data not modified");
        return;
    }

    cogs_mikai_show_loading_popup(app, "Writing...", "Place card on reader");
    popup_set_callback(app->popup, cogs_mikai_scene_write_card_popup_callback);

    if(mykey_write_to_nfc(app)) {
        app->mykey.is_modified = false;
        cogs_mikai_show_success_popup(app, "Success!\nCard updated");
    } else {
        cogs_mikai_show_error_popup(app, "Error\nWrite failed\nTry again");
    }
}

bool cogs_mikai_scene_write_card_on_event(void* context, SceneManagerEvent event) {
    COGSMyKaiApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, COGSMyKaiSceneStart);
        consumed = true;
    }

    return consumed;
}

void cogs_mikai_scene_write_card_on_exit(void* context) {
    COGSMyKaiApp* app = context;
    popup_reset(app->popup);
}
