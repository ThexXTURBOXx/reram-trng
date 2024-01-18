#pragma once

#include <circle/types.h>

#define RERAM_ADESTO_RM25C512C_LTAI_T 1
#define RERAM_FUJITSU_MB85AS4MTPF_G_BCERE1 2

#if MEM_TYPE == RERAM_ADESTO_RM25C512C_LTAI_T

// http://web.archive.org/web/20200221101215/https://www.adestotech.com/wp-content/uploads/DS-RM25C512C_079.pdf
#define MEM_NAME                    "RERAM_ADESTO_RM25C512C_LTAI_T"
#define MEM_NAME_SIMPLE             "Adesto"
#define MEM_ADR_SEND                2
#define MEM_SIZE_ADR                ((u32)65536)
#define MEM_ACCESS_WIDTH_BIT        8
#define MEM_ACCESS_TIME_NS          ((u32)150)
#define MEM_CAN_BURN_OUT            1

#elif MEM_TYPE == RERAM_FUJITSU_MB85AS4MTPF_G_BCERE1

// https://www.fujitsu.com/tw/Images/MB85AS4MT-DS501-00045-1v0-E.pdf
#define MEM_NAME                    "RERAM_FUJITSU_MB85AS4MTPF_G_BCERE1"
#define MEM_NAME_SIMPLE             "Fujitsu"
#define MEM_ADR_SEND                3
#define MEM_SIZE_ADR                ((u32)524288)
#define MEM_ACCESS_WIDTH_BIT        8
#define MEM_ACCESS_TIME_NS          ((u32)150)
#define MEM_CAN_BURN_OUT            0 // TODO: Was not yet able to burn it out

#else

#error MEM_TYPE NOT DEFINED

#endif
