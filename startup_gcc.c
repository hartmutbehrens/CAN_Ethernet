//*****************************************************************************
//
// startup_gcc.c - Startup code for use with GNU tools.
//
//*****************************************************************************

// Forward declaration of the default fault handlers. 
void ResetISR(void);
static void NmiSR(void);
static void FaultISR(void);
static void default_handler(void);

// External declaration of CAN interupt handler
extern void CAN_interrupt_handler(void);

// The entry point for the application.
extern int main(void);

// Reserve space for the system stack.
static unsigned long pulStack[64];

// The vector table.  Note that the proper constructs must be placed on this to ensure that it ends up at physical address 0x0000.0000.
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))((unsigned long)pulStack + sizeof(pulStack)),
                                          // The initial stack pointer
    ResetISR,                             // The reset handler
    NmiSR,                                // The NMI handler
    FaultISR,                             // The hard fault handler
    default_handler,                      // The MPU fault handler
    default_handler,                      // The bus fault handler
    default_handler,                      // The usage fault handler
    0,                                    // Reserved
    0,                                    // Reserved
    0,                                    // Reserved
    0,                                    // Reserved
    default_handler,                      // SVCall handler
    default_handler,                      // Debug monitor handler
    0,                                    // Reserved
    default_handler,                      // The PendSV handler
    default_handler,                      // The SysTick handler
    default_handler,                      // GPIO Port A
    default_handler,                      // GPIO Port B
    default_handler,                      // GPIO Port C
    default_handler,                      // GPIO Port D
    default_handler,                      // GPIO Port E
    default_handler,                      // UART0 Rx and Tx
    default_handler,                      // UART1 Rx and Tx
    default_handler,                      // SSI0 Rx and Tx
    default_handler,                      // I2C0 Master and Slave
    default_handler,                      // PWM Fault
    default_handler,                      // PWM Generator 0
    default_handler,                      // PWM Generator 1
    default_handler,                      // PWM Generator 2
    default_handler,                      // Quadrature Encoder 0
    default_handler,                      // ADC Sequence 0
    default_handler,                      // ADC Sequence 1
    default_handler,                      // ADC Sequence 2
    default_handler,                      // ADC Sequence 3
    default_handler,                      // Watchdog timer
    default_handler,                      // Timer 0 subtimer A
    default_handler,                      // Timer 0 subtimer B
    default_handler,                      // Timer 1 subtimer A
    default_handler,                      // Timer 1 subtimer B
    default_handler,                      // Timer 2 subtimer A
    default_handler,                      // Timer 2 subtimer B
    default_handler,                      // Analog Comparator 0
    default_handler,                      // Analog Comparator 1
    default_handler,                      // Analog Comparator 2
    default_handler,                      // System Control (PLL, OSC, BO)
    default_handler,                      // FLASH Control
    default_handler,                      // GPIO Port F
    default_handler,                      // GPIO Port G
    default_handler,                      // GPIO Port H
    default_handler,                      // UART2 Rx and Tx
    default_handler,                      // SSI1 Rx and Tx
    default_handler,                      // Timer 3 subtimer A
    default_handler,                      // Timer 3 subtimer B
    default_handler,                      // I2C1 Master and Slave
    default_handler,                      // Quadrature Encoder 1
    CAN_interrupt_handler,                // CAN0
    default_handler,                      // CAN1
    default_handler,                      // CAN2
    default_handler,                      // Ethernet
    default_handler                       // Hibernate
};

//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//
//*****************************************************************************
extern unsigned long _etext;
extern unsigned long _data;
extern unsigned long _edata;
extern unsigned long _bss;
extern unsigned long _ebss;

//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************
void ResetISR(void)
{
    unsigned long *pulSrc, *pulDest;

    //
    // Copy the data segment initializers from flash to SRAM.
    //
    pulSrc = &_etext;
    for(pulDest = &_data; pulDest < &_edata; )
    {
        *pulDest++ = *pulSrc++;
    }

    //
    // Zero fill the bss segment.
    //
    __asm("    ldr     r0, =_bss\n"
          "    ldr     r1, =_ebss\n"
          "    mov     r2, #0\n"
          "    .thumb_func\n"
          "zero_loop:\n"
          "        cmp     r0, r1\n"
          "        it      lt\n"
          "        strlt   r2, [r0], #4\n"
          "        blt     zero_loop");

    //
    // Call the application's entry point.
    //
    main();
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a NMI.  This
// simply enters an infinite loop, preserving the system state for examination
// by a debugger.
//
//*****************************************************************************
static void NmiSR(void)
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a fault
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
static void FaultISR(void)
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives an unexpected
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
static void default_handler(void)
{
    //
    // Go into an infinite loop.
    //
    while(1)
    {
    }
}
