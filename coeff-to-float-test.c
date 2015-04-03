#include <stdio.h>

/*
 * Code is modified from a post by Freescale forum user
 * PJH, see: http://forums.freescale.com/t5/Other-Microcontrollers/MPL115A-fixed-point-question/td-p/47633
 */
float coeff_to_float(unsigned short coeff,short bitCount,short iBits, short fracBits, short padBits)
{
// Utility function to convert the coeff's into floating point parameters.
// See Freescale App note AN3785 for details of how the parameters are stored.

    float result=0, float_sum=0;
    int s=0;
    int bit, b;
    int fb=0;
	unsigned short mask;

//	printf(R"Converting 0x%04x (%hd bits, %hd int bits, %hd frac bits, %hd padding)\n", coeff, bitCount, iBits, fracBits, padBits);
// We start working our way through the bits from high to low.
// The incoming data is a 16bit integer which we have to shorten since the lower LSB's are empty if <16 bits. (see pg 15 of AN3785).
    if (bitCount<16)
    {// Make coeff the appropriate size
        coeff=coeff>>(16-bitCount);
    }

// Sanity check that the parameters passed into the function make sense. We can't have more or less bits than the amount of data.
// Add 1 to account for sign bit.
    if((iBits+fracBits+1)!=bitCount)
    {
        printf(R"Error in parameters, fractional bits and integer bits don't add up!!\n");
        return 0;
    }
//Sanity check that if we are padding the decimal point that we can't also have integer bits.
    if((padBits>0)&&(iBits>0))
    {
        printf(R"Error in parameters. iBits and padBits not consistent!!\n");
        return 0;
    }
	
    for(b=bitCount;b>0;b--)
    {
		mask = (0x0001 << (b-1));
        bit=((mask & coeff)>>(b-1)); // Get each bit in turn as a 1 or a 0.
        if (b==bitCount) // if this is the first bit (sign bit check if it is set)
        {
            s=bit;
            if(s)
            {
                coeff=~coeff+1; //Calculate 2's complement; //(See App note AN3785 middle of Pg15 for explanation.
            }
        }
        else // process the bits in turn.
        {
            if(iBits>0)
            { // we have an integer bit;
                if(bit)
                {
                    result+=(0x0001 << (iBits-1));
                }
                iBits--;
            }
            else // we have a fractional bit
            {
                fb++;
                if(bit)
                {
                    result+= (float)(1.0/(float)(0x01 << (fb+padBits)));
                }
                fracBits--;
            }
        }
    }
    if(s)
    {
        result*=-1;
    }
    return result;
}

int main()
{
	int len = 16, i, j=0;
	unsigned char buf[] = {
		0xc1, 0x5a, 0xc0, 0x7f, 
		0x00, 0x41, 0x4f, 0xb2,
		 0xad, 0xb7, 0xd0, 0x36, 
		0x78, 0x00, 0x00, 0x00};
	
	unsigned short coeff[8];
	short num_bits[8] = {0, 0, 16, 16, 16, 14, 11, 11},
	 int_bits[8]={0, 0, 12, 2, 1, 0, 0, 0},
	 frac_bits[8]={0, 0, 3, 13, 14, 13, 10, 10},
	 pad_bits[8]={0, 0, 0, 0, 0, 9, 11, 15};
	float f_coeff[8];
	char * names[8] = {"PADC", "TADC", "A0", "B1", "B2", "C12", "C11", "C22"};
	
	printf(R"Coefficients are: \n");
	for(i=0; i<8; i++)
	{
		coeff[i] = (buf[j++] << 8);		
		coeff[i] += (buf[j++] & 0x00FF);
		f_coeff[i] = coeff_to_float(coeff[i], num_bits[i], int_bits[i], frac_bits[i], pad_bits[i]);
		printf(R"%s: %f {%hu} [%hd] (0x%04x)\n", names[i], f_coeff[i], coeff[i], coeff[i], coeff[i]);
	}
	return 0;
}
