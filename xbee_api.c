#include "xbee_api.h"
#include <stdio.h>
#include "debug_flag.h"

/*
 * Processes contents of frame_buf, returns a pointer to an
 * xbee_frame struct (points to data in frame_buf), resets
 * frame_ptr to zero for the next frame.
 *
 * Because the returned pointer is within frame_buf, all
 * processing must be completely finished before calling
 * xbee_check_frame() again.
*/
xbee_frame* xbee_process_frame();

unsigned char frame_buf[200];
int frame_ptr=0;

/*
 * Checks for a new frame
 */
int xbee_check_frame()
{
	xbee_frame *x_frame;
	char c;
	static int reset=0;
	if(reset)
	{
		frame_ptr = 0;
		reset = 0;
	}
	
	if(xbee_keypressed())
    {
        c = xbee_getchar();
        if(c == 0x7e)
        {
            frame_ptr=0;
        }
		frame_buf[frame_ptr++] = c;
    }
	if(frame_ptr > 3 && frame_ptr >= ((xbee_frame*)frame_buf)->length + 4)
    {
		reset = 1;
		return 1;
	}
	return 0;
}

unsigned short xbee_get_source()
{
	xbee_frame *frame;
	io_data *io;
	frame = (xbee_frame*)&frame_buf;
	if(frame->api_id == 0x83)
	{
		io = (io_data*)frame->cmd_data;
		return io->source_add;
	}
	return 0xFFFF;
}

unsigned char xbee_get_rssi()
{
	xbee_frame *frame;
	io_data *io;
	frame = (xbee_frame*)&frame_buf;
	if(frame->api_id == 0x83)
	{
		io = (io_data*)frame->cmd_data;
		return io->rssi;
	}
	return 0;
}

/*
 * Gets analog samples
 */
int xbee_get_samples(unsigned short *buf, int max)
{
	xbee_frame *frame;
	io_data *io;
	unsigned short c_mask;
	int i=0, buf_i=0, smp_i=0;
	
	frame = (xbee_frame*)&frame_buf;
	if(frame->api_id != 0x83)
	{
		if(is_debug()){printf(R"xbee_get_samples can't process frame type 0x%02x\n", frame->api_id);}
		return 0;
	}
	
	io = (io_data*)frame->cmd_data;
	if((io->chan_ind & 0x01FF))
	{
		//A digital channel is enabled, so we must
		//ignore the first two bytes of sample data
		i = 1;
		if(is_debug()){printf(R"Digital channels enabled: 0x%02x\n", (io->chan_ind & 0x01FF));}
	}else
	{
		i = 0;
	}
	c_mask = 0x4000;
	while(c_mask != 0x0100) //Only poll the analog lines
	{
		if(is_debug()){printf(R"Polling analog line 0x%02x\n", c_mask);}
		if((c_mask & io->chan_ind))
		{
			if(is_debug()){printf(R"Found analog line 0x%02x with i=%d, num_samps=%d\n", (c_mask & io->chan_ind),i, io->num_samps);}
			for(i=0; i<io->num_samps && i < max; i++)
			{
				buf[buf_i++] = io->sample_data[smp_i++];
			}
		}
		c_mask = (c_mask >> 1);
	}
	return buf_i;
}


void xbee_print_frame()
{
	xbee_frame *frame;
	io_data *io;
	int i=0;
	
	frame = (xbee_frame*)&frame_buf;
	if(is_debug()){printf(R"Frame size: %d\nAPI id: 0x%02x\n", frame->length, frame->api_id);}
	if(frame->api_id == 0x83)
	{
		io = (io_data*)frame->cmd_data;
		if(is_debug()){printf(R"Source: 0x%04x\nRx Signal: -%d dBm\nNum samples: %d\nChannels: 0x%04x\nSamples:\n", io->source_add, io->rssi, io->num_samps, io->chan_ind);}
		for(i=0; i<io->num_samps; i++)
		{
			if(is_debug()){printf(R"0x%04x ", io->sample_data[i]);}
		}
	}
	if(is_debug()){printf(R"\n");}//Checksum: 0x%02x\n", checksum);
}

void xbee_print_raw_frame()
{
    int i;
	int length = frame_ptr;
	for(i=0; i<length; i++)
	{
		if(is_debug()){printf(R"0x%02x ", frame_buf[i]);}
		if(i!= 0 && !(i%10))
		{
			if(is_debug()){printf(R"\n");}
		}
	}
    if(is_debug()){printf(R"\n");}
}
