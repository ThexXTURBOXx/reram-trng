#pragma once

#include "common.h"

void spi_init();

void spi_send_recv(u8 chip_select, u8 *sbuffer, u32 ssize, u8 *rbuffer, u32 rsize);

void spi_send(u8 chip_select, u8 *data, u32 size);

void spi_recv(u8 chip_select, u8 *data, u32 size);

