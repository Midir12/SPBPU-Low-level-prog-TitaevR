#include "print.h"

#define UART_REG_TXFIFO 0x0
#define UART_BASE_ADDR 0x10010000

static volatile int *uart = (int *)(void *)UART_BASE_ADDR;

static int putchar(char ch)
{
    while (uart[UART_REG_TXFIFO] < 0);
    return uart[UART_REG_TXFIFO] = ch & 0xFF;
}

void print(char* str)
{
    while(*str)
    {
        putchar(*str);
        str++;
    }
}