#include "../cogs_mikai.h"
#include <string.h>

enum {
    SetQuickAmountSceneEventInput,
};

// Same validator/parser as add_credit scene
static bool quick_validator(const char* text, FuriString* error, void* context) {
    UNUSED(context);
    if(strlen(text) == 0) return true;

    bool has_decimal = false;
    for(size_t i = 0; text[i] != '\0'; i++) {
        if(text[i] == '.' || text[i] == ',') {
            if(has_decimal) {
                furi_string_set(error, "One decimal only");
                return false;
            }
            has_decimal = true;
        } else if(text[i] < '0' || text[i] > '9') {
            furi_string_set(error, "Numbers and '.' only");
            return false;
        }
    }
    return true;
}

static bool quick_parse_euros(const char* text, uint16_t* cents) {
    if(!text || !cents) return false;

    uint32_t integer_part = 0, decimal_part = 0, decimal_digits = 0;
    bool in_decimal = false;

    for(size_t i = 0; text[i] != '\0'; i++) {
        if(text[i] == '.' || text[i] == ',') {
            in_decimal = true;
        } else if(text[i] >= '0' && text[i] <= '9') {
            if(in_decimal) {
                if(decimal_digits < 2) {
                    decimal_part = decimal_part * 10 + (text[i] - '0');
                    decimal_digits++;
                }
            } else {
                integer_part = integer_part * 10 + (text[i] - '0');
            }
        }
    }
    if(decimal_digits == 1) decimal_part *= 10;

    uint32_t total = integer_part * 100 + decimal_part;
    if(total == 0 || total > 99999) return false;

    *cents = (uint16_t)total;
    return true;
}

static void text_input_callback(void* context) {
    COGSMyKaiApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, SetQuickAmountSceneEventInput);
}

static void popup_callback(void* context) {
    COGSMyKaiApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, 0);
}

void cogs_mikai_scene_set_quick_amount_on_enter(void* context) {
    COGSMyKaiApp* app = context;

    // Pre-fill with current configured value
    snprintf(
        app->text_buffer,
        sizeof(app->text_buffer),
        "%u.%02u",
        app->settings.quick_charge_cents / 100,
        app->settings.quick_charge_cents % 100);

    TextInput* text_input = app->text_input;
    text_input_set_header_text(text_input, "Quick Charge Amount");
    text_input_set_validator(text_input, quick_validator, NULL);
    text_input_set_result_callback(
        text_input,
        text_input_callback,
        app,
        app->text_buffer,
        sizeof(app->text_buffer),
        false);

    view_dispatcher_switch_to_view(app->view_dispatcher, COGSMyKaiViewTextInput);
}

bool cogs_mikai_scene_set_quick_amount_on_event(void* context, SceneManagerEvent event) {
    COGSMyKaiApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SetQuickAmountSceneEventInput) {
            if(app->text_buffer[0] != '\0') {
                uint16_t cents = 0;
                if(quick_parse_euros(app->text_buffer, &cents)) {
                    app->settings.quick_charge_cents = cents;
                    mykey_settings_save(app);
                    text_input_reset(app->text_input);
                    memset(app->text_buffer, 0, sizeof(app->text_buffer));

                    // Update the menu label immediately
                    snprintf(
                        app->quick_charge_label,
                        sizeof(app->quick_charge_label),
                        "Quick: %u.%02u EUR",
                        cents / 100,
                        cents % 100);

                    popup_set_callback(app->popup, popup_callback);
                    cogs_mikai_show_success_popup(app, "Quick amount saved!");
                } else {
                    popup_set_callback(app->popup, popup_callback);
                    cogs_mikai_show_error_popup(app, "Invalid amount\nEnter 0.01-999.99");
                }
            }
            consumed = true;
        } else {
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, COGSMyKaiSceneStart);
            consumed = true;
        }
    }

    return consumed;
}

void cogs_mikai_scene_set_quick_amount_on_exit(void* context) {
    COGSMyKaiApp* app = context;
    text_input_reset(app->text_input);
    popup_reset(app->popup);
}
