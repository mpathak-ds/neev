#ifndef OS_GEN_DEF_H
#define OS_GEN_DEF_H

//#define ARCH_IS_UNIPROCESSOR

#define STATUS_SUCCESS 0x00
#define STATUS_FAILED_DEVICE_INIT 0x01
#define STATUS_FAILED_INVALID_PARAMS 0x02

#define PANIC_FAILED_EARLY_INIT 0x00
#define PANIC_FAILED_LATE_INIT 0x01
#define PANIC_INVALID_CALL 0x02
#define PANIC_UR_PAGE_FAULT 0x03 // for clarity, UR means unrecoverable

typedef unsigned int os_status;
typedef unsigned int panic_code;

#endif
