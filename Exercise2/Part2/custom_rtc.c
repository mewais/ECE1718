#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

void * LW_virtual; // Lightweight bridge base address
volatile int *HEX_3_0_ptr, *HEX_5_4_ptr, *TIMER_PTR; // virtual addresses
int cur_count;

int get_hex(int value)
{
	switch (value)
	{
	case 0:
	{
		return 63;
	}
	case 1:
	{
		return 6;
	}
	case 2:
	{
		return 91;
	}
	case 3:
	{
		return 79;
	}
	case 4:
	{
		return 102;
	}
	case 5:
	{
		return 109;
	}
	case 6:
	{
		return 124;
	}
	case 7:
	{
		return 7;
	}
	case 8:
	{
		return 127;
	}
	case 9:
	{
		return 111;
	}
	}
	return 0;
}

void set_hex_display( void)
{
	int hs, s, m, digit0, digit1, digit2, digit3, digit4, digit5;
	if (cur_count > 599999)
	{
		cur_count = 0;
	}
	hs = cur_count%100;
	s = (cur_count / 100)%60;
	m = (cur_count / 6000);
	digit0=get_hex(hs%10);
	digit1=get_hex(hs/10);
	digit2=get_hex(s%10);
	digit3=get_hex(s/10);
	digit4=get_hex(m%10);
	digit5=get_hex(m/10);
	*HEX_5_4_ptr = digit4 + (digit5<<8);
	*HEX_3_0_ptr = digit0 + (digit1<<8) + (digit2<<16) + (digit3<<24);
}

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	TIMER_PTR[0]=0;
	cur_count ++;
	set_hex_display();
	//reset interrupt
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init initialize_timer_handler(void)
{
	int value=0;
	cur_count = 0;
	// generate a virtual address for the FPGA lightweight bridge
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	HEX_3_0_ptr = LW_virtual + HEX3_HEX0_BASE; // virtual address for HEX3-0 port
	HEX_5_4_ptr = LW_virtual + HEX5_HEX4_BASE;
	TIMER_PTR = LW_virtual + TIMER0_BASE; // virtual address for KEY port
	set_hex_display();
	// Register the interrupt handler.
	value = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, "timer_0_irq_handler", (void *) (irq_handler));
	TIMER_PTR[3]=0xF;
	TIMER_PTR[2]=0x4240;
	TIMER_PTR[1]=7;
	return value;
}

static void __exit cleanup_timer_handler(void)
{
	TIMER_PTR[1] = 0;
	iounmap (LW_virtual);
	free_irq (TIMER0_IRQ, (void*) irq_handler);
}

module_init(initialize_timer_handler);
module_exit(cleanup_timer_handler);
