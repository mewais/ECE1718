#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>             // for __init, see code
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <asm/io.h>
#include <asm/uaccess.h>
#include "../address_map_arm.h"

#define DEV_NAME "accel"
#define SUCCESS 0

/* Bit values in BW_RATE                                                */
/* Expresed as output data rate */
#define XL345_RATE_3200       0x0f
#define XL345_RATE_1600       0x0e
#define XL345_RATE_800        0x0d
#define XL345_RATE_400        0x0c
#define XL345_RATE_200        0x0b
#define XL345_RATE_100        0x0a
#define XL345_RATE_50         0x09
#define XL345_RATE_25         0x08
#define XL345_RATE_12_5       0x07
#define XL345_RATE_6_25       0x06
#define XL345_RATE_3_125      0x05
#define XL345_RATE_1_563      0x04
#define XL345_RATE__782       0x03
#define XL345_RATE__39        0x02
#define XL345_RATE__195       0x01
#define XL345_RATE__098       0x00

/* Bit values in DATA_FORMAT                                            */

/* Register values read in DATAX0 through DATAZ1 are dependant on the 
   value specified in data format.  Customer code will need to interpret
   the data as desired.                                                 */
#define XL345_RANGE_2G             0x00
#define XL345_RANGE_4G             0x01
#define XL345_RANGE_8G             0x02
#define XL345_RANGE_16G            0x03
#define XL345_DATA_JUST_RIGHT      0x00
#define XL345_DATA_JUST_LEFT       0x04
#define XL345_10BIT                0x00
#define XL345_FULL_RESOLUTION      0x08
#define XL345_INT_LOW              0x20
#define XL345_INT_HIGH             0x00
#define XL345_SPI3WIRE             0x40
#define XL345_SPI4WIRE             0x00
#define XL345_SELFTEST             0x80

/* Bit values in INT_ENABLE, INT_MAP, and INT_SOURCE are identical
   use these bit values to read or write any of these registers.        */
#define XL345_OVERRUN              0x01
#define XL345_WATERMARK            0x02
#define XL345_FREEFALL             0x04
#define XL345_INACTIVITY           0x08
#define XL345_ACTIVITY             0x10
#define XL345_DOUBLETAP            0x20
#define XL345_SINGLETAP            0x40
#define XL345_DATAREADY            0x80

/* Bit values in POWER_CTL                                              */
#define XL345_WAKEUP_8HZ           0x00
#define XL345_WAKEUP_4HZ           0x01
#define XL345_WAKEUP_2HZ           0x02
#define XL345_WAKEUP_1HZ           0x03
#define XL345_SLEEP                0x04
#define XL345_MEASURE              0x08
#define XL345_STANDBY              0x00
#define XL345_AUTO_SLEEP           0x10
#define XL345_ACT_INACT_SERIAL     0x20
#define XL345_ACT_INACT_CONCURRENT 0x00

// ADXL345 Register List
#define ADXL345_REG_DEVID       	0x00
#define ADXL345_TAP_THRESH			0x1D
#define ADXL345_TAP_DURATION		0x21
#define ADXL345_TAP_LATENCY			0x22
#define ADXL345_TAP_WINDOW			0x23
#define ADXL345_TAP_AXES			0x2A
#define ADXL345_REG_POWER_CTL   	0x2D
#define ADXL345_REG_DATA_FORMAT 	0x31
#define ADXL345_REG_FIFO_CTL    	0x38
#define ADXL345_REG_BW_RATE     	0x2C
#define ADXL345_REG_INT_ENABLE  	0x2E  // default value: 0x00
#define ADXL345_REG_INT_MAP     	0x2F  // default value: 0x00
#define ADXL345_REG_INT_SOURCE  	0x30  // default value: 0x02
#define ADXL345_REG_DATAX0      	0x32  // read only
#define ADXL345_REG_DATAX1      	0x33  // read only
#define ADXL345_REG_DATAY0      	0x34  // read only
#define ADXL345_REG_DATAY1      	0x35  // read only
#define ADXL345_REG_DATAZ0      	0x36  // read only
#define ADXL345_REG_DATAZ1      	0x37  // read only
#define ADXL345_REG_OFSX        	0x1E
#define ADXL345_REG_OFSY        	0x1F
#define ADXL345_REG_OFSZ        	0x20
#define ADXL345_REG_THRESH_ACT		0x24
#define ADXL345_REG_THRESH_INACT	0x25
#define ADXL345_REG_TIME_INACT		0x26
#define ADXL345_REG_ACT_INACT_CTL	0x27
     
// Reset Manager Registers
#define RSTMGR_BRGMODRST        ((volatile unsigned int *) 0xffd0501c)
     
// Security settings registers for peripherals
#define L3REGS_L4SP             ((volatile unsigned int *) 0xff80000c)
#define L3REGS_L4OSC1           ((volatile unsigned int *) 0xff800014)

// Rounded division macro
#define ROUNDED_DIVISION(n, d) (((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d))
     
// ADXL345 Functions
void ADXL345_Init(void);
void ADXL345_SetFormat( int, int);
void ADXL345_SetRate( int);
void ADXL345_Calibrate(void);
bool ADXL345_IsDataReady(void);
bool ADXL345_WasActivityUpdated(void);
void ADXL345_XYZ_Read( int16_t szData16[3]);
void ADXL345_IdRead( uint8_t *pId);
void ADXL345_REG_READ( uint8_t address, uint8_t *value);
void ADXL345_REG_WRITE( uint8_t address, uint8_t value);
void ADXL345_REG_MULTI_READ( uint8_t address, uint8_t values[], uint8_t len);
int initialize_elements(void);
// I2C0 Functions
void I2C0_Init(void);
void I2C0_Enable_FPGA_Access(void);

// Pinmux Functions
void Pinmux_Config(void);
int mg_per_lsb=0;
// Declare functions
static int device_open (struct inode *, struct file *);
static int device_release (struct inode *, struct file *);
static ssize_t device_read (struct file *, char *, size_t, loff_t *);
static ssize_t device_write (struct file *, const char *, size_t, loff_t *);


#define MAX_SIZE 1024
char msg [MAX_SIZE + 1];
// Declare global variables needed to use the accelerometer driver
volatile int* I2C0_ptr; 	// virtual address for I2C communication
volatile int* SYSMGR_ptr; 	// virtual address for System Manager communication

// Define file operations
static struct file_operations accel_fops = {
	.owner = THIS_MODULE,
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct miscdevice accel = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME,
    .fops = &accel_fops,
    .mode = 0666
};

int device_registered = 0;

void Pinmux_Config(){
    // Set up pin muxing (in sysmgr) to connect ADXL345 wires to I2C0
	*(SYSMGR_ptr + SYSMGR_I2C0USEFPGA) = 0;
	*(SYSMGR_ptr + SYSMGR_GENERALIO7) = 1;
	*(SYSMGR_ptr + SYSMGR_GENERALIO8) = 1;
}

// Initialize the I2C0 controller for use with the ADXL345 chip
void I2C0_Init(){

	// Abort any ongoing transmits and disable I2C0.
	*(I2C0_ptr + I2C0_ENABLE) = 2;

	// Wait until I2C0 is disabled
	while(((*(I2C0_ptr + I2C0_ENABLE_STATUS))&0x1) == 1){}

	// Configure the config reg with the desired setting (act as 
	// a master, use 7bit addressing, fast mode (400kb/s)).
	*(I2C0_ptr + I2C0_CON) = 0x65;

	// Set target address (disable special commands, use 7bit addressing)
	*(I2C0_ptr + I2C0_TAR) = 0x53;

	// Set SCL high/low counts (Assuming default 100MHZ clock input to I2C0 Controller).
	// The minimum SCL high period is 0.6us, and the minimum SCL low period is 1.3us,
	// However, the combined period must be 2.5us or greater, so add 0.3us to each.
	*(I2C0_ptr + I2C0_FS_SCL_HCNT) = 60 + 30; // 0.6us + 0.3us
	*(I2C0_ptr + I2C0_FS_SCL_LCNT) = 130 + 30; // 1.3us + 0.3us

	// Enable the controller
	*(I2C0_ptr + I2C0_ENABLE) = 1;

	// Wait until controller is enabled
	while(((*(I2C0_ptr + I2C0_ENABLE_STATUS))&0x1) == 0){}
    
}

// Function to allow components on the FPGA side (ex. Nios II processors) to 
// access the I2C0 controller through the F2H bridge. This function
// needs to be called from an ARM program, and to allow a Nios II program
// to access the I2C0 controller.
void I2C0_Enable_FPGA_Access(){

    // Deassert fpga bridge resets
    *RSTMGR_BRGMODRST = 0;
    
    // Enable non-secure masters to access I2C0
    *L3REGS_L4SP = *L3REGS_L4SP | 0x4;
    
    // Enable non-secure masters to access pinmuxing registers (in sysmgr)
    *L3REGS_L4OSC1 = *L3REGS_L4OSC1 | 0x10;
}

// Write value to internal register at address
void ADXL345_REG_WRITE( uint8_t address, uint8_t value){
    
    // Send reg address (+0x400 to send START signal)
    *(I2C0_ptr + I2C0_DATA_CMD) = address + 0x400;
    
    // Send value
    *(I2C0_ptr + I2C0_DATA_CMD) = value;
}

// Read value from internal register at address
void ADXL345_REG_READ( uint8_t address, uint8_t *value){

    // Send reg address (+0x400 to send START signal)
    *(I2C0_ptr + I2C0_DATA_CMD) = address + 0x400;
    
    // Send read signal
    *(I2C0_ptr + I2C0_DATA_CMD) = 0x100;
    
    // Read the response (first wait until RX buffer contains data)  
    while (*(I2C0_ptr + I2C0_RXFLR) == 0){}
    *value = *(I2C0_ptr + I2C0_DATA_CMD);
}

// Read multiple consecutive internal registers
void ADXL345_REG_MULTI_READ( uint8_t address, uint8_t values[], uint8_t len){

    int i;
    int nth_byte=0;
    // Send reg address (+0x400 to send START signal)
    *(I2C0_ptr + I2C0_DATA_CMD) = address + 0x400;
    
    // Send read signal len times
    for (i=0;i<len;i++)
        *(I2C0_ptr + I2C0_DATA_CMD) = 0x100;

    // Read the bytes
    while (len){
        if ((*(I2C0_ptr + I2C0_RXFLR)) > 0){
            values[nth_byte] = *(I2C0_ptr + I2C0_DATA_CMD);
            nth_byte++;
            len--;
        }
    }
}

// Initialize the ADXL345 chip
void ADXL345_Init(){

    // +- 16g range, full resolution
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);
    mg_per_lsb = 4;
    // Output Data Rate: 200Hz
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_200);

    // NOTE: The DATA_READY bit is not reliable. It is updated at a much higher rate than the Data Rate
    // Use the Activity and Inactivity interrupts.
    //----- Enabling interrupts -----//
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_ACT, 0x04);	//activity threshold
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_INACT, 0x00);	//inactivity threshold
    ADXL345_REG_WRITE(ADXL345_REG_TIME_INACT, 0xFF);	//time for inactivity
    //ADXL345_REG_WRITE(ADXL345_REG_THRESH_INACT, 0x02);	//inactivity threshold
    //ADXL345_REG_WRITE(ADXL345_REG_TIME_INACT, 0x02);	//time for inactivity
    ADXL345_REG_WRITE(ADXL345_REG_ACT_INACT_CTL, 0xFF);	//Enables AC coupling for thresholds
    ADXL345_REG_WRITE(ADXL345_REG_INT_ENABLE, XL345_ACTIVITY | XL345_INACTIVITY | XL345_SINGLETAP | XL345_DOUBLETAP);	//enable interrupts
    //-------------------------------//
    
    // stop measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // start measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}

// Calibrate the ADXL345. The DE1-SoC should be placed on a flat
// surface, and must remain stationary for the duration of the calibration.
void ADXL345_Calibrate(){
    
    int average_x = 0;
    int average_y = 0;
    int average_z = 0;
    int16_t XYZ[3];
    int8_t offset_x;
    int8_t offset_y;
    int8_t offset_z;
    uint8_t saved_bw;
    uint8_t saved_dataformat;
    int i = 0;
    
    // stop measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // Get current offsets
    ADXL345_REG_READ( ADXL345_REG_OFSX, (uint8_t *)&offset_x);
    ADXL345_REG_READ( ADXL345_REG_OFSY, (uint8_t *)&offset_y);
    ADXL345_REG_READ( ADXL345_REG_OFSZ, (uint8_t *)&offset_z);
    
    // Use 100 hz rate for calibration. Save the current rate.
    ADXL345_REG_READ( ADXL345_REG_BW_RATE, &saved_bw);
    ADXL345_REG_WRITE( ADXL345_REG_BW_RATE, XL345_RATE_100);
    
    // Use 16g range, full resolution. Save the current format.
    ADXL345_REG_READ( ADXL345_REG_DATA_FORMAT, &saved_dataformat);
    ADXL345_REG_WRITE( ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);
    
    // start measure
    ADXL345_REG_WRITE( ADXL345_REG_POWER_CTL, XL345_MEASURE);
    
    // Get the average x,y,z accelerations over 32 samples (LSB 3.9 mg)
    while (i < 32){
	// Note: use DATA_READY here, can't use ACTIVITY because board is stationary.
        if (ADXL345_IsDataReady()){
            ADXL345_XYZ_Read(XYZ);
            average_x += XYZ[0];
            average_y += XYZ[1];
            average_z += XYZ[2];
            i++;
        }
    }
    average_x = ROUNDED_DIVISION(average_x, 32);
    average_y = ROUNDED_DIVISION(average_y, 32);
    average_z = ROUNDED_DIVISION(average_z, 32);
    
    // stop measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);
    
    // printf("Average X=%d, Y=%d, Z=%d\n", average_x, average_y, average_z);
    
    // Calculate the offsets (LSB 15.6 mg)
    offset_x += ROUNDED_DIVISION(0-average_x, 4);
    offset_y += ROUNDED_DIVISION(0-average_y, 4);
    offset_z += ROUNDED_DIVISION(256-average_z, 4);
    
    // printf("Calibration: offset_x: %d, offset_y: %d, offset_z: %d (LSB: 15.6 mg)\n",offset_x,offset_y,offset_z);
    
    // Set the offset registers
    ADXL345_REG_WRITE(ADXL345_REG_OFSX, offset_x);
    ADXL345_REG_WRITE( ADXL345_REG_OFSY, offset_y);
    ADXL345_REG_WRITE( ADXL345_REG_OFSZ, offset_z);
    
    // Restore original bw rate
    ADXL345_REG_WRITE( ADXL345_REG_BW_RATE, saved_bw);
    
    // Restore original data format
    ADXL345_REG_WRITE( ADXL345_REG_DATA_FORMAT, saved_dataformat);
    
    // start measure
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}

// Return true if there was activity since the last read (checks ACTIVITY bit).
bool ADXL345_WasActivityUpdated(){
	bool bReady = false;
    uint8_t data8;
    
    ADXL345_REG_READ( ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_ACTIVITY)
        bReady = true;
    
    return bReady;
}

// Return true if there is new data (checks DATA_READY bit).
bool ADXL345_IsDataReady(){
    bool bReady = false;
    uint8_t data8;
    
    ADXL345_REG_READ(ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_DATAREADY)
        bReady = true;
    
    return bReady;
}

// Read acceleration data of all three axes
void ADXL345_XYZ_Read(int16_t szData16[3]){
    uint8_t szData8[6];
    ADXL345_REG_MULTI_READ( 0x32, (uint8_t *)&szData8, sizeof(szData8));

    szData16[0] = (szData8[1] << 8) | szData8[0]; 
    szData16[1] = (szData8[3] << 8) | szData8[2];
    szData16[2] = (szData8[5] << 8) | szData8[4];
}

// Read the ID register
void ADXL345_IdRead(uint8_t *pId){
	ADXL345_REG_READ( ADXL345_REG_DEVID, pId);
}

// Set data format
void ADXL345_SetFormat(int XL345_RANGE, int XL345_RESOLUTION){
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE | XL345_RESOLUTION);
}

// Set the data rate
void ADXL345_SetRate(int XL345_RATE){
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE);
}

static int __init start_accel(void)
{
	// initialize the dev_t, cdev, and class data structures
	int err = misc_register(&accel);
	if (err < 0)
	{
		printk(KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME);
		return err;
	}
    printk(KERN_INFO "/dev/%s driver registered\n", DEV_NAME);

	I2C0_ptr = ioremap_nocache (I2C0_BASE, I2C0_SPAN);
	SYSMGR_ptr = ioremap_nocache (SYSMGR_BASE, SYSMGR_SPAN);
	if ((I2C0_ptr == 0) || (SYSMGR_ptr == NULL)) {
		printk (KERN_ERR "Error: ioremap_nocache returned NULL\n");
		return -1;
	}
	return initialize_elements();
}

int initialize_elements(void)
{
	// Configure Pin Muxing
    uint8_t devid;
    Pinmux_Config();
    
    // Initialize I2C0 Controller
    I2C0_Init();
    
    // 0xE5 is read from DEVID(0x00) if I2C is functioning correctly
    ADXL345_REG_READ(0x00, &devid);
    
    // Correct Device ID
    if (devid == 0xE5){
        // Initialize accelerometer chip
        ADXL345_Init();
        ADXL345_Calibrate();

		device_registered = 1;
		printk(KERN_INFO "Accel registered");
    } else {
        printk(KERN_ERR "Incorrect device ID\n");
		return -1;
    }
	ADXL345_REG_WRITE(ADXL345_TAP_AXES, 1);
	ADXL345_REG_WRITE(ADXL345_TAP_THRESH, 40);
	ADXL345_REG_WRITE(ADXL345_TAP_DURATION, 32);
	ADXL345_REG_WRITE(ADXL345_TAP_LATENCY, 80);
	ADXL345_REG_WRITE(ADXL345_TAP_WINDOW, 240);
	return 0;
}

static void __exit stop_accel(void)
{
	/* unmap the physical-to-virtual mappings */
	if (device_registered)
	{
		iounmap (SYSMGR_ptr);
		iounmap (I2C0_ptr);

		misc_deregister(&accel);
	}
}

static int device_open(struct inode *inode, struct file *file)
{
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
	size_t bytes;
	char info[30];
    int16_t XYZ[3];
	uint8_t interrupt_reg;
	int new = 0;
	ADXL345_REG_READ(ADXL345_REG_INT_SOURCE, &interrupt_reg);
	// Verify whether there are any new data
	// (function returns bool)
	new = (int) ADXL345_WasActivityUpdated();
	// Read new data
	ADXL345_XYZ_Read(XYZ);
	sprintf(info, "%02x %4d %4d %4d %2d\n", interrupt_reg, XYZ[0], XYZ[1], XYZ[2], mg_per_lsb);
	
	bytes = strlen (info) - (*offset);           // how many bytes not yet sent?
	bytes = bytes > length ? length : bytes;        // too much to send all at once?
	
	if (bytes)
		if (copy_to_user (buffer, &info, bytes) != 0)
			printk (KERN_ERR "Error: copy_to_user unsuccessful");
	*offset = bytes;        // keep track of number of bytes sent to the user
	
	return bytes;
}


static ssize_t device_write(struct file *filp, const char* buffer, size_t length, loff_t *offset)
{
	size_t bytes;
	char dev_str[] = "device";
	char init_str[] = "init";
	char cal_str[] = "calibrate";
	char form_str[] = "format";
	char rate_str[] = "rate";
    bytes = length;
	if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    msg[bytes-1] = '\0';    // NULL terminate
	// Find out the command we got
	if (strcmp(msg, cal_str) == 0)
	{
		ADXL345_Calibrate();
		return bytes;
	}
	else if (strcmp(msg, dev_str) == 0)
	{
		uint8_t devid;
		ADXL345_REG_READ(0x00, &devid);
		printk(KERN_INFO "The Device ID is %x\n",devid);
		return bytes;
	}
	else if (strcmp(msg, init_str) == 0)
	{
		initialize_elements();
		return bytes;
	}
	else if (strncmp(msg, form_str, 6) == 0)
	{
		if (msg[6] != ' ')
		{
			printk(KERN_ERR "Format string invalid");
			return bytes;
		}
		if (msg[7] == '1')
		{
			ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);
			mg_per_lsb = 4;
		}
		else if (msg[7] == '0' && msg[8] == ' ')
		{
			if (msg[9]=='2')
			{
				ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_2G);
				mg_per_lsb = 4;
			}
			else if (msg[9]=='4')
			{
				ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_4G);
				mg_per_lsb = 8;
			}
			else if (msg[9]=='8')
			{
				ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_8G);
				mg_per_lsb = 16;
			}
			else if (msg[9]=='1' && msg[10]=='6')
			{
				ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G);
				mg_per_lsb = 31;
			}
			else
			{
				printk(KERN_ERR "Format string invalid");
				return bytes;
			}
		}
		else
		{
			printk(KERN_ERR "Format string invalid");
			return bytes;
		}
		return bytes;
	}
	else if (strncmp(msg, rate_str, 4))
	{
		
		if (msg[4] != ' ')
		{
			printk(KERN_ERR "Format string invalid");
			return bytes;
		}
		
		if (msg[5] == '5' && msg[6] == '0')
		{
			ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_50);
		}
		else if (msg[5] == '2' && msg[6] == '5')
		{
			ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_25);
		}
		else if (msg[5] == '1' && msg[6] == '0' && msg[7] == '0')
		{
			ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_100);
		}
		else if (msg[5] == '2' && msg[6] == '0' && msg[7] == '0')
		{
			ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_200);
		}
		else if (msg[5] == '1' && msg[6] == '2' && msg[7] == '.' && msg[8]=='5')
		{
			ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_12_5);
		}
		else if (msg[5] == '6' && msg[6] == '.' && msg[7] == '2' && msg[8]=='5')
		{
			ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_6_25);
		}
	}
	return bytes;
}
MODULE_LICENSE("GPL");
module_init (start_accel);
module_exit (stop_accel);
