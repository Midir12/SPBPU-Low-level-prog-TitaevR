#include "print.h"
#include "sw_timer.h"

void end(void) asm("end");
void wfi(void) {__asm__ volatile("wfi" ::: "memory");}

void oneshot(void)
{
    print("ONESHOT timer callback\n");
}

void multishot(void)
{
    print("MULTISHOT timer callback\n");
}

void periodic(void)
{
    print("PERIODIC timer callback\n");
}

void main(void)
{
    sw_timer_handle_t th1 = sw_timer_add_syscall(200000, ONESHOT, 1, oneshot);
    sw_timer_handle_t th2 = sw_timer_add_syscall(200000, MULTISHOT, 5, multishot);
    sw_timer_handle_t th3 = sw_timer_add_syscall(200000, PERIODIC, 1, periodic);

    print("timers inited\n");
    init_hardware_timer_syscall(500000);

    while (sw_timer_is_active_syscall(th1))
    {
        wfi();
    }

    end();
}