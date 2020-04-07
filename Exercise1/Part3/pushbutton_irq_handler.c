#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "../address_map_arm.h"
#include "../interrupt_ID.h"

void * LW_virtual; // Lightweight bridge base address
volatile int *LEDR_ptr, *HEX_ptr, *KEY_ptr; // virtual addresses

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	int cur_val = *LEDR_ptr + 1;
	if (cur_val > 0x209)
	{
		cur_val = 0x200;
	}
	*LEDR_ptr = cur_val;
	int hex_val;
	switch(cur_val - 0x200)
	{
		case 0:
			hex_val = 0x3F;
			break;
		case 1:
			hex_val = 0x06;
			break;
		case 2:
			hex_val = 0x5B;
			break;
		case 3:
			hex_val = 0x4F;
			break;
		case 4:
			hex_val = 0x66;
			break;
		case 5:
			hex_val = 0x6D;
			break;
		case 6:
			hex_val = 0x7D;
			break;
		case 7:
			hex_val = 0x07;
			break;
		case 8:
			hex_val = 0x7F;
			break;
		case 9:
			hex_val = 0x67;
			break;
		default:
			hex_val = 0x00;
	}
	*HEX_ptr = hex_val;
	// Clear the Edgecapture register (clears current interrupt)
	*(KEY_ptr + 3) = 0xF;
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init initialize_pushbutton_handler(void)
{
	int value;
	// generate a virtual address for the FPGA lightweight bridge
	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

	LEDR_ptr = LW_virtual + LEDR_BASE; // virtual address for LEDR port
	*LEDR_ptr = 0x200; // turn on the leftmost light

	HEX_ptr = LW_virtual + HEX3_HEX0_BASE; // virtual address for HEX3-0 port
	*HEX_ptr = 0x3F;

	KEY_ptr = LW_virtual + KEY_BASE; // virtual address for KEY port
	*(KEY_ptr + 3) = 0xF; // Clear the Edgecapture register
	*(KEY_ptr + 2) = 0xF; // Enable IRQ generation for the 4 buttons

	// Register the interrupt handler.
	value = request_irq (KEYS_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED, "pushbutton_irq_handler", (void *) (irq_handler));
	return value;
}

static void __exit cleanup_pushbutton_handler(void)
{
	*LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
	iounmap (LW_virtual);
	free_irq (KEYS_IRQ, (void*) irq_handler);
}

module_init(initialize_pushbutton_handler);
module_exit(cleanup_pushbutton_handler);
