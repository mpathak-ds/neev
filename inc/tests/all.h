#ifndef OS_TEST_SUITE_H
#define OS_TEST_SUITE_H

#include <stdint.h>
#include <osdef.h>
#include <adefs.h>

os_status
tests_do_all (
	pfirmware_info_t boot_info
	);

os_status
test_physmem (
	pfirmware_info_t boot_info
	);

#endif
