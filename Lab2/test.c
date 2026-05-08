typedef unsigned long uintptr_t;

#define UART_REG_TXFIFO 0x0

#define UART_BASE_ADDR 0x10010000
#define CLINT_BASE_ADDR 0x2000000
#define CLINT_MTIME_OFFSET 0xBFF8
#define CLINT_MTIMECMP_OFFSET 0x4000 // offset for hart #0

void end(void) asm("end");


void timer_init(void) asm("timer_init");
void timer_setup(unsigned long timecmp_inc) asm("timer_setup");
void print(char* str);
void wfi(void) {__asm__ volatile("wfi" ::: "memory");}

static int syscall_timer_init();
static int syscall_timer_setup(unsigned long timecmp_inc);
static int syscall_print(char* str);
static int putchar(char ch);


static volatile int *mtime_addr = (int *)(void *)(CLINT_BASE_ADDR + CLINT_MTIME_OFFSET);
static volatile int *mtimecmp_addr = (int *)(void *)(CLINT_BASE_ADDR + CLINT_MTIMECMP_OFFSET);
static volatile int *uart = (int *)(void *)UART_BASE_ADDR;


static int syscalls(unsigned long arg0, unsigned long arg1, 
    unsigned long arg2, unsigned long arg3, unsigned long arg4,
    unsigned long arg5, unsigned long arg6, unsigned long arg7) 
{
    // despite you don't need all these registers it is better to set them all
    // remember that in trap_vector in start.S you rewrite a0, a1 and a2 so you better to use another registers
    // for sending info to handle_trap

    register uintptr_t a0 asm("a0") = (uintptr_t)(arg0); // connect variable to register
    register uintptr_t a1 asm("a1") = (uintptr_t)(arg1); // connect variable to register
    register uintptr_t a2 asm("a2") = (uintptr_t)(arg2); // connect variable to register
    register uintptr_t a3 asm("a3") = (uintptr_t)(arg3); // connect variable to register
    register uintptr_t a4 asm("a4") = (uintptr_t)(arg4); // connect variable to register
    register uintptr_t a5 asm("a5") = (uintptr_t)(arg5); // connect variable to register
    register uintptr_t a6 asm("a6") = (uintptr_t)(arg6); // connect variable to register
    register uintptr_t a7 asm("a7") = (uintptr_t)(arg7); // connect variable to register

    asm volatile("ecall" 
        : "=r"(a0), "=r"(a1) 
        : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7) 
        : "memory");

    return (int)a0;
}

void print(char* str)
{
    while(*str)
    {
        putchar(*str);
        str++;
    }
}

static int syscall_timer_init()
{
    return syscalls(0, 0, 0, 0, 0, 1, 0, 0);
}

static int syscall_timer_setup(unsigned long timecmp_inc)
{
    return syscalls(0, 0, 0, 0, 0, 2, timecmp_inc, 0);
}

static int syscall_print(char* str)
{
    return syscalls(0, 0, 0, 0, 0, 3, (unsigned long)str, 0);
}

static int putchar(char ch)
{
    while (uart[UART_REG_TXFIFO] < 0);
    return uart[UART_REG_TXFIFO] = ch & 0xFF;
}

void main(void)
{
    char* str = "Titaev\n";
    syscall_print(str);
    syscall_timer_init();
    syscall_timer_setup(1000);
    wfi();
    end();
}

long handle_trap(long cause, long epc, long regs[32])
{
    print("HANDLE BEGIN\n");
    if (cause < 0) // Прерывание
    {
        unsigned long interrupt_cause = cause & ~(1UL << 63); // Убираем знаковый бит (63-й бит), чтобы получить код прерывания

        if (interrupt_cause == 7) // Таймерное прерывание (код 7)
        {
            print("TIMER INTERRUPT\n");
            timer_setup(0); // Выключаем таймер, передавая 0 в качестве параметра
        }
        else
        {
            print("UNKNOWN INTERRUPT\n");
        }
    }
    else // Исключение
    {
        unsigned long exception_cause = cause; // Знаковый бит уже равен 0, поэтому просто используем cause как код исключения

        if (exception_cause == 8) // Исключение ecall
        {
            int exception_type = regs[15];
            switch (exception_type)
            {
                case 1: // Инициализация таймера
                    print("TIMER_INIT SYSCALL\n");
                    timer_init();
                    break;
                case 2: // настройка таймера
                    print("TIMER_SETUP SYSCALL\n");
                    timer_setup(regs[16]);
                    break;
                case 3: // Печать
                    print("PRINT SYSCALL\n");
                    print((char*)regs[16]);
                    break;
            }
        }
        else
        {
            print("UNKNOWN EXCEPTION\n");
        }

        epc += 4; // Инкрементируем epc на 4 байта на след. инструкцию
    }

    return epc;
}

//qemu-system-riscv64 -machine sifive_u -nographic -bios none -kernel test
//cd Desktop / 6 семестр / NIP / SPBPU - Low - level - prog - TitaevR / lab2