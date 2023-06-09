#pragma once

#include "common.h"

bool str_eq(char *a, char *b);

int strcpy(char *dst, char *src);

int strlen(char *s);

int strcat(char *dst, char *src);

void delay(u64 ticks);

void put32(u64 address, u32 value);

u32 get32(u64 address);

u32 get_el();
