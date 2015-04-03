#ifndef I2C_H
#define I2C_H

#define RETRY                   32      // Retry attempts when failing to receive ACK
#define TIMEOUT                 4024    // Timeout value for waiting for receive/transmit complete


// Defines for I2CCTL

#define	I2CCTL_ENABLE             0x80    
#define I2CCTL_DISABLE            0x00
#define I2CCTL_START              0x40   
#define I2CCTL_STOP               0x20    
#define I2CCTL_BIRQ               0x10    
#define I2CCTL_TXI                0x08   
#define I2CCTL_NACK               0x04   
#define I2CCTL_FLUSH              0x02   
#define I2CCTL_FILTER_ENABLE      0x01   

/* 
 * There appears that there is no way to send an ACK
 * but it looks like setting the STOP condition does this. 
 * BUT I see no documentation to confirm this. 
 */




// Defines for I2CISTAT

#define TRANSMIT_DATA_REG_EMPTY   0x80    
#define	RECEIVE_DATA_REG_FULL     0x40    
#define	RECEIVED_SAM              0x20    
#define RECEIVED_10BIT_ADDR       0x10    
#define I2C_RECEIVING             0x08    
#define I2C_ARBITRATION           0x04   
#define I2C_SPRS                  0x02    
#define	RECEIVED_NACK             0x01    


void i2c_init();
int i2c_wait_TDRE();
int i2c_wait_RDRF();
int i2c_wait_ACKV();
int i2c_wait_BUSY();

void i2c_print_diag();

/*
 * Returns a 1 if the ACK bit is 1, or a 0 if not
*/
int i2c_get_ACK();
void i2c_clear();
void i2c_write_bytes( int n, ... );
void i2c_write_byte( int addr, int data );
void i2c_write_two_bytes( int addr, int word, unsigned char data );
int i2c_send_byte(unsigned char byte);
int i2c_send_byte_wait_ack(unsigned char byte);
unsigned char i2c_read_byte(int addr);
unsigned char i2c_read_byte2(int addr, int word);
int i2c_check_device(unsigned char addr);


#endif

