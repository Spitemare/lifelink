#pragma once
#include <pebble.h>

typedef Layer NameLayer;

NameLayer *name_layer_create(GRect frame);
void name_layer_destroy(NameLayer *this);
char *name_layer_get_name(NameLayer *this);
void name_layer_set_name(NameLayer *this, const char *name);
