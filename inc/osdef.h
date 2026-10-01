#ifndef OS_GEN_DEF_H
#define OS_GEN_DEF_H

//#define ARCH_IS_UNIPROCESSOR
#define OS_CONDUCT_TESTS

#define STATUS_SUCCESS 0x00
#define STATUS_FAILED_DEVICE_INIT 0x01
#define STATUS_FAILED_INVALID_PARAMS 0x02
#define STATUS_FAILED_EARLY_TEST 0x03
#define STATUS_FAILED_LATE_TEST 0x04

#define PANIC_FAILED_EARLY_INIT 0x00
#define PANIC_FAILED_LATE_INIT 0x01
#define PANIC_INVALID_CALL 0x02
#define PANIC_UR_PAGE_FAULT 0x03 // for clarity, UR means unrecoverable
#define PANIC_OUT_OF_MEMORY 0x04
#define PANIC_OUT_OF_BALLOC 0x05

#define ALIGN_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

typedef unsigned int os_status;
typedef unsigned int panic_code;

#endif
