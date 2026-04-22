#include "../cogs_mikai.h"

void cogs_mikai_scene_about_on_enter(void* context) {
    COGSMyKaiApp* app = context;
    Widget* widget = app->widget;

    // Title: fork name
    widget_add_string_element(widget, 64, 2,  AlignCenter, AlignTop, FontPrimary,   "MyKey Marchy");
    widget_add_string_element(widget, 64, 14, AlignCenter, AlignTop, FontSecondary, "v1.1-marchy");

    // Separator
    widget_add_string_element(widget, 64, 24, AlignCenter, AlignTop, FontSecondary, "COGES MyKey NFC Tool");

    // Credits — mandatory per license
    widget_add_string_element(widget, 64, 36, AlignCenter, AlignTop, FontSecondary, "Fork by: Marchy");
    widget_add_string_element(widget, 64, 46, AlignCenter, AlignTop, FontSecondary, "Original: luhf");
    widget_add_string_element(widget, 64, 56, AlignCenter, AlignTop, FontSecondary, "Algo: MIKAI team");

    view_dispatcher_switch_to_view(app->view_dispatcher, COGSMyKaiViewWidget);
}

bool cogs_mikai_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void cogs_mikai_scene_about_on_exit(void* context) {
    COGSMyKaiApp* app = context;
    widget_reset(app->widget);
}
