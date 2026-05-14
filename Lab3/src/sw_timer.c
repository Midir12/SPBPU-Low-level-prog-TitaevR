#include "sw_timer.h"
#include "print.h"

typedef unsigned long uintptr_t;

#define MTIME_ADDR      0x0200BFF8   // Адрес регистра времени машины
#define MTIMECMP_ADDR   0x02004000   // Адрес регистра сравнения

typedef struct {
    sw_timer_type_t type;           // Тип таймера
    long ticks_period;              // Период срабатывания в тактах
    long last_tick_time;            // Время последнего срабатывания
    timer_callback_t callback;      // Функция обратного вызова
    bool is_active;                 // Активен ли таймер
    unsigned long remaining_shots;  // Оставшееся количество срабатываний
} sw_timer_t;

typedef enum {
    SW_TIMER_ADD,       // Добавить программный таймер
    SW_TIMER_IS_ACTIVE, // Проверить активность таймера
    HW_TIMER_INIT,      // Инициализировать аппаратный таймер
} ecall_t;

// Прототипы функций обработки системных вызовов
long hw_timer_init(long* epc, long registers[32]);
long hw_timer_update(uint64_t resolution_us);
long sw_timer_add(long* epc, long registers[32]);
long sw_timer_is_active(long* epc, long registers[32]);

static uint64_t resolution_us = 500000;          // Разрешение таймера в микросекундах
static uint64_t timers_count = 0;                // Текущее количество активных таймеров
static sw_timer_t timers[MAX_SOFT_TIMERS];       // Массив всех программных таймеров
static volatile uint64_t* const mtime = (volatile uint64_t*)MTIME_ADDR;     // Указатель на регистр
static volatile uint64_t* const mtimecmp = (volatile uint64_t*)MTIMECMP_ADDR; // Указатель на mtimecmp

void timer_init(void) asm("timer_init");   // Включение таймера
void timer_off(void) asm("timer_off");     // Выключение таймера

static int syscalls(unsigned long arg0, unsigned long arg1,
    unsigned long arg2, unsigned long arg3, unsigned long arg4,
    unsigned long arg5, unsigned long arg6, unsigned long arg7)
{
    register uintptr_t a0 asm("a0") = (uintptr_t)(arg0);
    register uintptr_t a1 asm("a1") = (uintptr_t)(arg1);
    register uintptr_t a2 asm("a2") = (uintptr_t)(arg2);
    register uintptr_t a3 asm("a3") = (uintptr_t)(arg3);
    register uintptr_t a4 asm("a4") = (uintptr_t)(arg4);
    register uintptr_t a5 asm("a5") = (uintptr_t)(arg5);
    register uintptr_t a6 asm("a6") = (uintptr_t)(arg6);
    register uintptr_t a7 asm("a7") = (uintptr_t)(arg7);

    asm volatile("ecall"
        : "+r"(a0), "+r"(a1)
        : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
        : "memory");

    return a0;
}

long hw_timer_init(long* epc, long registers[32])
{
    *epc += 4;                              // Cледующая инструкция после ecall
    timer_init();                           // Включаем таймер
    *mtimecmp = *mtime + registers[10];     
    return 0;
}

// Установка следующей точки срабатывания)
long hw_timer_update(uint64_t resolution_us)
{
    *mtimecmp = *mtime + resolution_us;     // Следующее прерывание через resolution_us тактов
    return 0;
}

long sw_timer_add(long* epc, long registers[32])
{
    *epc += 4;                                      // Следующая инструкция
    sw_timer_t new_timer;                           

    new_timer.type = registers[11];                 
    new_timer.ticks_period = registers[10];        
    new_timer.last_tick_time = *mtime;              
    new_timer.callback = (timer_callback_t)(registers[13]); 
    new_timer.is_active = true;                     
    new_timer.remaining_shots = registers[12];      

    
    if (timers_count < MAX_SOFT_TIMERS)
    {
        timers[timers_count] = new_timer;           
        return timers_count++;                      
    }
    else
    {
        return -1;  
    }
}

long sw_timer_is_active(long* epc, long registers[32])
{
    *epc += 4;                                              
    unsigned int timer_index = registers[10];               
    return (long)(timers[timer_index].is_active);          
}

sw_timer_handle_t sw_timer_add_syscall(unsigned long period_ticks, sw_timer_type_t timer_type,
    unsigned long shots_count, timer_callback_t callback_func)
{
    return (sw_timer_handle_t)syscalls(period_ticks, timer_type, shots_count,
        (unsigned long)callback_func, 0, 0, 0, SW_TIMER_ADD);
}

bool sw_timer_is_active_syscall(sw_timer_handle_t timer_index)
{
    return (bool)syscalls(timer_index, 0, 0, 0, 0, 0, 0, SW_TIMER_IS_ACTIVE);
}

bool init_hardware_timer_syscall(uint64_t resolution_microseconds)
{
    return (bool)syscalls(resolution_microseconds, 0, 0, 0, 0, 0, 0, HW_TIMER_INIT);
}

long sw_timer_irq_handler(long cause, long epc, long registers[32])
{

    if (cause < 0) 
    {
        unsigned long interrupt_cause = cause & ~(1UL << 63); 

        if (interrupt_cause == 7) 
        {
            
            for (int i = 0; i < MAX_SOFT_TIMERS; i++)
            {
                sw_timer_t* current_timer = &timers[i];

                if (current_timer->is_active) 
                {
                    
                    uint64_t next_shot_time = current_timer->last_tick_time + current_timer->ticks_period;
                    if (*mtime > next_shot_time)
                    {
                        
                        switch (current_timer->type)
                        {
                        case ONESHOT:   
                            current_timer->callback();       
                            current_timer->is_active = false; 
                            timers_count--;                   
                            break;

                        case MULTISHOT: 
                            current_timer->callback();       
                            if (current_timer->remaining_shots == 0)
                            {
                                
                                current_timer->is_active = false;
                                timers_count--;
                            }
                            else
                            {
                                
                                current_timer->last_tick_time = *mtime;
                                current_timer->remaining_shots--;
                            }
                            break;

                        case PERIODIC:  
                            current_timer->callback();           
                            current_timer->last_tick_time = *mtime; 
                            break;
                        }
                    }
                }
            }

            
            if (timers_count == 0)
            {
                timer_off();
            }

            
            hw_timer_update(resolution_us);
        }
        else
        {
            print("UNKNOWN INTERRUPT\n");
        }
    }
    else 
    {
        unsigned long exception_cause = cause; 

        if (exception_cause == 8) 
        {
            
            switch (registers[17])
            {
            case SW_TIMER_ADD:
                sw_timer_add(&epc, registers);
                break;
            case SW_TIMER_IS_ACTIVE:
                sw_timer_is_active(&epc, registers);
                break;
            case HW_TIMER_INIT:
                hw_timer_init(&epc, registers);
                break;
            default:
                print("UNKNOWN SYSCALL\n");
                break;
            }
        }
        else
        {
            print("UNKNOWN EXCEPTION\n");
        }
    }

    return epc;
}
