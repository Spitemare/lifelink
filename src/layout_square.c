#include <pebble.h>

#include "effect_layer.h"
#include "layout.h"
#include "player.h"
#include "round_time_layer.h"
#include "storage.h"

#ifdef PBL_RECT

#define _PLAYER_LIFE_BUFFER_SIZE 4

typedef struct GroupLayerExtras {
    char life_buf[_PLAYER_LIFE_BUFFER_SIZE];
} GroupLayerExtras;

typedef struct Extras {
    EffectLayer *effect_layer;
    PropertyAnimation *animation;
    RoundTimeLayer *round_time_layer;
} Extras;

static Layout* layout_group_find_layout(LayoutGroup *layout_group, Player *player) {
    if (layout_group->player_one_layout->player == player)
        return layout_group->player_one_layout;
    else
        return layout_group->player_two_layout;
}

static void layout_update_life_layer(Layout *layout) {
    Player *player = layout->player;
    char *buf = ((GroupLayerExtras *) layer_get_data(layout->group_layer))->life_buf;
    snprintf(buf, sizeof(char) * _PLAYER_LIFE_BUFFER_SIZE, "%d", player->life);
    text_layer_set_text(layout->life_layer, buf);
}

static void layout_update_name_layer(Layout *layout) {
    Player *player = layout->player;
    text_layer_set_text(layout->name_layer, player->name);
}

static Layout *layout_create(Player *player) {
    Layout *layout = malloc(sizeof(Layout));
    layout->player = player;
    layout->group_layer = layer_create_with_data(GRectZero, sizeof(GroupLayerExtras));

    layout->life_layer = text_layer_create(GRectZero);
    text_layer_set_font(layout->life_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
    text_layer_set_text_alignment(layout->life_layer, GTextAlignmentRight);
    text_layer_set_background_color(layout->life_layer, GColorClear);
    text_layer_set_text_color(layout->life_layer, GColorWhite);
    layout_update_life_layer(layout);
    layer_add_child(layout->group_layer, text_layer_get_layer(layout->life_layer));

    layout->name_layer = text_layer_create(GRectZero);
    text_layer_set_text_alignment(layout->name_layer, GTextAlignmentRight);
    text_layer_set_background_color(layout->name_layer, GColorClear);
    text_layer_set_text_color(layout->name_layer, GColorWhite);
    layout_update_name_layer(layout);
    layer_add_child(layout->group_layer, text_layer_get_layer(layout->name_layer));

    return layout;
}

LayoutGroup *layout_group_create(Player *player_one, Player* player_two) {
    Layout *player_one_layout;
    Layout *player_two_layout;
    EffectLayer *effect_layer;
    RoundTimeLayer *round_time_layer;
    LayoutGroup *layout_group = malloc(sizeof(LayoutGroup));
    Extras *extras = malloc(sizeof(Extras));
    layout_group->extras = extras;

    player_one_layout = layout_create(player_one);
    player_two_layout = layout_create(player_two);
    layout_group->player_one_layout = player_one_layout;
    layout_group->player_two_layout = player_two_layout;

    effect_layer = effect_layer_create(GRectZero);
    effect_layer_add_effect(effect_layer, effect_invert, NULL);
    extras->effect_layer = effect_layer;

    round_time_layer = round_time_layer_create();
    if (persist_exists(STORAGE_ROUND_TIME_KEY) && persist_exists(STORAGE_LAST_RUN_KEY)) {
         int32_t time_left = persist_read_int(STORAGE_ROUND_TIME_KEY);
         int32_t last_run = persist_read_int(STORAGE_LAST_RUN_KEY) * 1000;
         int32_t now = time(NULL) * 1000;
         time_left -= (now - last_run);
         if (time_left > 0) round_time_layer_set_time_left(round_time_layer, time_left);
    }
    extras->round_time_layer = round_time_layer;

    return layout_group;
}

static void layout_destroy(Layout *layout) {
    text_layer_destroy(layout->life_layer);
    text_layer_destroy(layout->name_layer);
    layer_destroy(layout->group_layer);

    free(layout);
}

void layout_group_destroy(LayoutGroup *layout_group) {
    layout_destroy(layout_group->player_one_layout);
    layout_destroy(layout_group->player_two_layout);

    Extras *extras = (Extras *) layout_group->extras;
    persist_write_int(STORAGE_ROUND_TIME_KEY, extras->round_time_layer->time_left);
    persist_write_int(STORAGE_LAST_RUN_KEY, time(NULL));

    effect_layer_destroy(extras->effect_layer);
    round_time_layer_destroy(extras->round_time_layer);

    free(layout_group->extras);
    free(layout_group);
}

static void layout_mark_dirty(Layout *layout) {
    layout_update_life_layer(layout);
    layout_update_name_layer(layout);
    layer_mark_dirty(layout->group_layer);
}

void layout_group_mark_dirty(LayoutGroup *layout_group) {
    layout_mark_dirty(layout_group->player_one_layout);
    layout_mark_dirty(layout_group->player_two_layout);
}

static void layout_set_frame(Layout *layout, GRect frame) {
    int16_t life_height = frame.size.h * 0.6;

    layer_set_frame(layout->group_layer, frame);
    layer_set_frame(text_layer_get_layer(layout->life_layer), GRect(0, 0, frame.size.w - 5, life_height));
    layer_set_frame(text_layer_get_layer(layout->name_layer), GRect(0, life_height, frame.size.w - 5, frame.size.h - life_height));
}

void layout_group_add_to_window(LayoutGroup *layout_group, Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    int16_t width = bounds.size.w - ACTION_BAR_WIDTH;
    int16_t height = (bounds.size.h - ROUND_TIME_LAYER_HEIGHT) / 2;

    layout_set_frame(layout_group->player_one_layout, GRect(0, 0, width, height));
    layout_set_frame(layout_group->player_two_layout, GRect(0, height + ROUND_TIME_LAYER_HEIGHT, width, height));

    Extras *extras = (Extras *) layout_group->extras;
    effect_layer_set_frame(extras->effect_layer, layer_get_frame(layout_group->player_one_layout->group_layer));
    round_time_layer_set_frame(extras->round_time_layer, GRect(0, height, width, ROUND_TIME_LAYER_HEIGHT));

    layer_add_child(window_layer, layout_group->player_one_layout->group_layer);
    layer_add_child(window_layer, layout_group->player_two_layout->group_layer);
    layer_add_child(window_layer, round_time_layer_get_layer(extras->round_time_layer));
    layer_add_child(window_layer, effect_layer_get_layer(extras->effect_layer));
}

static void animation_stopped(Animation *animation, bool finished, void *data) {
#ifdef PBL_PLATFORM_APLITE
    Extras *extras = (Extras *) data;
    property_animation_destroy(extras->animation);
#endif
}

void layout_group_select_player(LayoutGroup *layout_group, Player *player) {
    Layout *layout = layout_group_find_layout(layout_group, player);
    GRect frame = layer_get_frame(layout->group_layer);
    Extras *extras = (Extras *) layout_group->extras;

    extras->animation = property_animation_create_layer_frame(effect_layer_get_layer(extras->effect_layer), NULL, &frame);
    Animation *animation = property_animation_get_animation(extras->animation);
    animation_set_duration(animation, 150);
    animation_set_handlers(animation, (AnimationHandlers) {
        .stopped = (AnimationStoppedHandler) animation_stopped
    }, extras);
    animation_schedule(animation);
}

void layout_group_update_player(LayoutGroup *layout_group, Player *player) {
    Layout *layout = layout_group_find_layout(layout_group, player);
    layout_update_life_layer(layout);
    layout_update_name_layer(layout);
}

uint32_t layout_group_round_time_tick(LayoutGroup *layout_group) {
    RoundTimeLayer *round_time_layer = ((Extras *) layout_group->extras)->round_time_layer;
    return round_time_layer_tick(round_time_layer);
}

#endif