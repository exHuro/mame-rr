/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "includes/flstory.h"

/***************************************************************************

 Fairy Land Story 68705 protection interface

 The following is ENTIRELY GUESSWORK!!!

 It seems, however, to be identical to Buggy Challenge.

***************************************************************************/

READ8_HANDLER( flstory_68705_port_a_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();

	//logerror("%04x: 68705 port A read %02x\n", cpu_get_pc(&space->device()), state->m_port_a_in);
	return (state->m_port_a_out & state->m_ddr_a) | (state->m_port_a_in & ~state->m_ddr_a);
}

WRITE8_HANDLER( flstory_68705_port_a_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();

	//logerror("%04x: 68705 port A write %02x\n", cpu_get_pc(&space->device()), data);
	state->m_port_a_out = data;
}

WRITE8_HANDLER( flstory_68705_ddr_a_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	state->m_ddr_a = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  1   W  when 1->0, enables latch which brings the command from main CPU (read from port A)
 *  2   W  when 0->1, copies port A to the latch for the main CPU
 */

READ8_HANDLER( flstory_68705_port_b_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	return (state->m_port_b_out & state->m_ddr_b) | (state->m_port_b_in & ~state->m_ddr_b);
}

WRITE8_HANDLER( flstory_68705_port_b_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	//logerror("%04x: 68705 port B write %02x\n",cpu_get_pc(&space->device()),data);

	if ((state->m_ddr_b & 0x02) && (~data & 0x02) && (state->m_port_b_out & 0x02))
	{
		state->m_port_a_in = state->m_from_main;
		if (state->m_main_sent)
			device_set_input_line(state->m_mcu, 0, CLEAR_LINE);
		state->m_main_sent = 0;
		logerror("read command %02x from main cpu\n", state->m_port_a_in);
	}
	if ((state->m_ddr_b & 0x04) && (data & 0x04) && (~state->m_port_b_out & 0x04))
	{
		logerror("send command %02x to main cpu\n", state->m_port_a_out);
		state->m_from_mcu = state->m_port_a_out;
		state->m_mcu_sent = 1;
	}

	state->m_port_b_out = data;
}

WRITE8_HANDLER( flstory_68705_ddr_b_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	state->m_ddr_b = data;
}


READ8_HANDLER( flstory_68705_port_c_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();

	state->m_port_c_in = 0;
	if (state->m_main_sent)
		state->m_port_c_in |= 0x01;

	if (!state->m_mcu_sent)
		state->m_port_c_in |= 0x02;

	//logerror("%04x: 68705 port C read %02x\n", cpu_get_pc(&space->device()), port_c_in);
	return (state->m_port_c_out & state->m_ddr_c) | (state->m_port_c_in & ~state->m_ddr_c);
}

WRITE8_HANDLER( flstory_68705_port_c_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	logerror("%04x: 68705 port C write %02x\n", cpu_get_pc(&space->device()), data);
	state->m_port_c_out = data;
}

WRITE8_HANDLER( flstory_68705_ddr_c_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	state->m_ddr_c = data;
}

WRITE8_HANDLER( flstory_mcu_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();

	logerror("%04x: mcu_w %02x\n", cpu_get_pc(&space->device()), data);
	state->m_from_main = data;
	state->m_main_sent = 1;
	device_set_input_line(state->m_mcu, 0, ASSERT_LINE);
}

READ8_HANDLER( flstory_mcu_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();

	logerror("%04x: mcu_r %02x\n",cpu_get_pc(&space->device()), state->m_from_mcu);
	state->m_mcu_sent = 0;
	return state->m_from_mcu;
}

READ8_HANDLER( flstory_mcu_status_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	int res = 0;

	/* bit 0 = when 1, mcu is ready to receive data from main cpu */
	/* bit 1 = when 1, mcu has sent data to the main cpu */
	//logerror("%04x: mcu_status_r\n", cpu_get_pc(&space->device()));
	if (!state->m_main_sent)
		res |= 0x01;
	if (state->m_mcu_sent)
		res |= 0x02;

	return res;
}

WRITE8_HANDLER( onna34ro_mcu_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	UINT16 score_adr = state->m_workram[0x29e] * 0x100 + state->m_workram[0x29d];

	switch (data)
	{
		case 0x0e:
			state->m_from_mcu = 0xff;
			break;
		case 0x01:
			state->m_from_mcu = 0x6a;
			break;
		case 0x40:
			if(score_adr >= 0xe000 && score_adr < 0xe800)
				state->m_from_mcu = state->m_workram[score_adr - 0xe000];			/* score l*/
			break;
		case 0x41:
			if(score_adr >= 0xe000 && score_adr < 0xe800)
				state->m_from_mcu = state->m_workram[(score_adr + 1) - 0xe000];		/* score m*/
			break;
		case 0x42:
			if(score_adr >= 0xe000 && score_adr < 0xe800)
				state->m_from_mcu = state->m_workram[(score_adr + 2) - 0xe000] & 0x0f;	/* score h*/
			break;
		default:
			state->m_from_mcu = 0x80;
	}
}

READ8_HANDLER( onna34ro_mcu_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	return state->m_from_mcu;
}

READ8_HANDLER( onna34ro_mcu_status_r )
{
	int res = 3;

	return res;
}


#define VICTNINE_MCU_SEED	(state->m_workram[0x685])

static const UINT8 victnine_mcu_data[0x100] =
{
	0x3e, 0x08, 0xdd, 0x29, 0xcb, 0x14, 0xfd, 0x29,
	0xcb, 0x15, 0xd9, 0x29, 0xd9, 0x30, 0x0d, 0xd9,
	0x19, 0xd9, 0xdd, 0x09, 0x30, 0x01, 0x24, 0xfd,
	0x19, 0x30, 0x01, 0x2c, 0x3d, 0x20, 0xe3, 0xc9,
	0x11, 0x14, 0x00, 0x19, 0x7e, 0x32, 0xed, 0xe4,
	0x2a, 0x52, 0xe5, 0x22, 0xe9, 0xe4, 0x2a, 0x54,
	0xe5, 0x22, 0xeb, 0xe4, 0x21, 0x2a, 0xe6, 0xfe,
	0x06, 0x38, 0x02, 0xcb, 0xc6, 0xcb, 0xce, 0xc9,
	0x06, 0x00, 0x3a, 0xaa, 0xe4, 0x07, 0x07, 0x07,
	0xb0, 0x47, 0x3a, 0xab, 0xe4, 0x07, 0x07, 0xb0,
	0x47, 0x3a, 0xac, 0xe4, 0xe6, 0x03, 0xb0, 0x21,
	0xe3, 0xe6, 0xc9, 0x38, 0xe1, 0x29, 0x07, 0xc9,
	0x23, 0x7e, 0x47, 0xe6, 0x1f, 0x32, 0x0c, 0xe6,
	0x78, 0xe6, 0xe0, 0x07, 0x07, 0x47, 0xe6, 0x03,
	0x28, 0x06, 0xcb, 0x7a, 0x28, 0x02, 0xc6, 0x02,
	0x32, 0x0a, 0xe6, 0x78, 0xe6, 0x80, 0xc9, 0x3a,
	0x21, 0x29, 0xe5, 0x7e, 0xe6, 0xf8, 0xf6, 0x01,
	0x77, 0x23, 0x3a, 0x0a, 0xe6, 0x77, 0x21, 0x08,
	0xe6, 0xcb, 0xc6, 0xcb, 0x8e, 0x3a, 0x2b, 0xe5,
	0x21, 0xff, 0xe5, 0xfe, 0x02, 0xc9, 0x1f, 0xc6,
	0x47, 0x3a, 0xaa, 0xe4, 0xa7, 0x21, 0x00, 0xe5,
	0x28, 0x03, 0x21, 0x1b, 0xe5, 0x70, 0x3a, 0xaa,
	0xe4, 0xee, 0x01, 0x32, 0xaa, 0xe4, 0x21, 0xb0,
	0xe4, 0x34, 0x23, 0x36, 0x00, 0xc9, 0x2b, 0xb2,
	0xaf, 0x77, 0x12, 0x23, 0x13, 0x3c, 0xfe, 0x09,
	0x20, 0xf7, 0x3e, 0x01, 0x32, 0xad, 0xe4, 0x21,
	0x48, 0xe5, 0xcb, 0xfe, 0xc9, 0x32, 0xe5, 0xaa,
	0x21, 0x00, 0x13, 0xe4, 0x47, 0x1b, 0xa1, 0xc9,
	0x00, 0x08, 0x04, 0x0c, 0x05, 0x0d, 0x06, 0x0e,
	0x22, 0x66, 0xaa, 0x22, 0x33, 0x01, 0x11, 0x88,
	0x06, 0x05, 0x03, 0x04, 0x08, 0x01, 0x03, 0x02,
	0x06, 0x07, 0x02, 0x03, 0x15, 0x17, 0x11, 0x13
};

WRITE8_HANDLER( victnine_mcu_w )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	UINT8 seed = VICTNINE_MCU_SEED;

	if (!seed && (data & 0x37) == 0x37)
	{
		state->m_from_mcu = 0xa6;
		logerror("mcu initialize (%02x)\n", data);
	}
	else
	{
		data += seed;

		if ((data & ~0x1f) == 0xa0)
		{
			state->m_mcu_select = data & 0x1f;
			//logerror("mcu select: 0x%02x\n", state->m_mcu_select);
		}
		else if (data < 0x20)
		{
			int offset = state->m_mcu_select * 8 + data;

			//logerror("mcu fetch: 0x%02x\n", offset);
			state->m_from_mcu = victnine_mcu_data[offset];
		}
		else if (data >= 0x38 && data <= 0x3a)
		{
			state->m_from_mcu = state->m_workram[0x691 - 0x38 + data];
		}
		else
		{
			//logerror("mcu: 0x%02x: unknown command\n", data);
		}
	}
}

READ8_HANDLER( victnine_mcu_r )
{
	flstory_state *state = space->machine().driver_data<flstory_state>();
	//logerror("%04x: mcu read (0x%02x)\n", cpu_get_previouspc(&space->device()), state->m_from_mcu);

	return state->m_from_mcu - VICTNINE_MCU_SEED;
}

READ8_HANDLER( victnine_mcu_status_r )
{
	int res = 3;

	return res;
}
