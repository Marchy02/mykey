#include "../cogs_mikai.h"

static void cogs_mikai_scene_reset_popup_callback(void* context) {
    COGSMyKaiApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, 0);
}

void cogs_mikai_scene_reset_on_enter(void* context) {
    COGSMyKaiApp* app = context;

    if(!app->mykey.is_loaded) {
        popup_set_callback(app->popup, cogs_mikai_scene_reset_popup_callback);
        cogs_mikai_show_error_popup(app, "No card loaded\nRead a card first");
        return;
    }

    // Reset the card
    mykey_reset(&app->mykey);

    // Update cached values
    app->mykey.is_reset = mykey_is_reset(&app->mykey);
    app->mykey.current_credit = mykey_get_current_credit(&app->mykey);

    // Show confirmation - saved in memory, not written to card
    popup_set_callback(app->popup, cogs_mikai_scene_reset_popup_callback);
    cogs_mikai_show_success_popup(app, "Card Reset!\nReset in memory\nUse 'Write to Card'");
}

bool cogs_mikai_scene_reset_on_event(void* context, SceneManagerEvent event) {
    COGSMyKaiApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        consumed = true;
        // Search back to start scene and switch (forces menu rebuild)
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, COGSMyKaiSceneStart);
    }

    return consumed;
}

void cogs_mikai_scene_reset_on_exit(void* context) {
    COGSMyKaiApp* app = context;
    popup_reset(app->popup);
}
