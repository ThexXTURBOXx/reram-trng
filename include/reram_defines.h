#pragma once

//#define RERAM_ADESTO_RM25C512C_LTAI_T 1
#define RERAM_FUJITSU_MB85AS4MTPF_G_BCERE1 1

#define PARALLEL 0
#define SPI 1

#ifdef RERAM_ADESTO_RM25C512C_LTAI_T
#define MEM_NAME                    "RERAM_ADESTO_RM25C512C_LTAI_T"
#define MEM_ACCESS_IF 				SPI
#define MEM_SIZE_ADR 				((uint32_t)512)
#define MEM_ACCESS_WIDTH_BIT		8
#define MEM_ACCESS_TIME_NS			((uint32_t)150)
#define MEM_SUPPLY_VOLTAGE_MIN_MV	((uint16_t)1650)
#define MEM_SUPPLY_VOLTAGE_MAX_MV	((uint16_t)3600)
#define MEM_OP_TEMP_MAX_K			((uint16_t)233)
#define MEM_OP_TEMP_MIN_K			((uint16_t)358)
#endif

#ifdef RERAM_FUJITSU_MB85AS4MTPF_G_BCERE1
#define MEM_NAME                     "RERAM_FUJITSU_MB85AS4MTPF_G_BCERE1"
#define MEM_ACCESS_IF                SPI
#define MEM_SIZE_ADR                 ((uint32_t)524288)
#define MEM_ACCESS_WIDTH_BIT         8
#define MEM_ACCESS_TIME_NS           ((uint32_t)150)
#define MEM_SUPPLY_VOLTAGE_MIN_MV    ((uint16_t)300)
#define MEM_SUPPLY_VOLTAGE_MAX_MV    ((uint16_t)3600)
#define MEM_OP_TEMP_MAX_K            ((uint16_t)233)
#define MEM_OP_TEMP_MIN_K            ((uint16_t)358)
#endif
