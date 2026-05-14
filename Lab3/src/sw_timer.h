#ifndef SW_TIMER_H
#define SW_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*timer_callback_t)(void);

typedef int sw_timer_handle_t;

typedef enum {
	ONESHOT,
	MULTISHOT,
	PERIODIC
} sw_timer_type_t;

sw_timer_handle_t sw_timer_add_syscall(unsigned long ticks, sw_timer_type_t type, unsigned long times_to_shot, timer_callback_t callback_fun);

bool sw_timer_is_active_syscall(sw_timer_handle_t index);

bool init_hardware_timer_syscall(uint64_t resolution);

#endif