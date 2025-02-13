#ifndef ACCELEROMETER_ADXL345_H_
#define ACCELEROMETER_ADXL345_H_

#include <stdint.h>

typedef int bool;
#define true 1
#define false 0

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

// I2C0 Controller Registers
#define I2C_ADDR	0xffc04000
#define I2C_RANGE	4096

#define I2C0_CON                ((long) 0x000)
#define I2C0_TAR                ((long) 0x004)
#define I2C0_DATA_CMD           ((long) 0x010)
#define I2C0_SS_SCL_HCNT        ((long) 0x014)
#define I2C0_SS_SCL_LCNT        ((long) 0x028)
#define I2C0_FS_SCL_HCNT        ((long) 0x01c)
#define I2C0_FS_SCL_LCNT        ((long) 0x020)
#define I2C0_INTR_MASK          ((long) 0x030)
#define I2C0_CLR_INTR           ((long) 0x040)
#define I2C0_TX_ABRT            ((long) 0x054)
#define I2C0_ENABLE             ((long) 0x06c)
#define I2C0_STATUS             ((long) 0x070)
#define I2C0_TXFLR              ((long) 0x074)
#define I2C0_RXFLR              ((long) 0x078)
#define I2C0_TX_ABRT_SOURCE     ((long) 0x080)
#define I2C0_ENABLE_STATUS      ((long) 0x09c)

// SYSMGR Pin Muxing Registers
#define SYSMGR_ADDR		0xffd08000
#define SYSMGR_RANGE	4096
/* GENERALIO7 (trace_d6): 
    0 : Pin is connected to GPIO/LoanIO number 55. 
    1 : Pin is connected to Peripheral signal I2C0.SDA. 
    2 : Pin is connected to Peripheral signal SPIS1.SS0. 
    3 : Pin is connected to Peripheral signal TRACE.D6. */
#define SYSMGR_GENERALIO7       ((long) 0x49C)
/* GENERALIO8 (trace_d7): 
    0 : Pin is connected to GPIO/LoanIO number 56. 
    1 : Pin is connected to Peripheral signal I2C0.SCL. 
    2 : Pin is connected to Peripheral signal SPIS1.MISO. 
    3 : Pin is connected to Peripheral signal TRACE.D7. */
#define SYSMGR_GENERALIO8       ((long) 0x4A0)
/* GPLMUX55 and GPLMUX56: 
    0 : LoanIO. 
    1 : GPIO. */
#define SYSMGR_GPLMUX55         ((long) 0x6B0)
#define SYSMGR_GPLMUX56         ((long) 0x6B4)
/* I2C0USEFPGA: 
    0 : I2C0 uses HPS Pins. 
    1 : I2C0 uses the FPGA Inteface. */
#define SYSMGR_I2C0USEFPGA      ((long) 0x704)
     
// Reset Manager Registers
#define RSTMGR_ADDR		0xffd05000
#define RSTMGR_RANGE	4096
#define RSTMGR_BRGMODRST        ((long) 0x01c)
     
// Security settings registers for peripherals
#define L3REGS_ADDR		0xff800000
#define L3REGS_RANGE	4096
#define L3REGS_L4SP             ((long) 0x00c)
#define L3REGS_L4OSC1           ((long) 0x014)
     
// Rounded division macro
#define ROUNDED_DIVISION(n, d) (((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d))

// ADXL345 Functions
void ADXL345_Init();
void ADXL345_Calibrate();
bool ADXL345_IsDataReady();
bool ADXL345_WasActivityUpdated();
void ADXL345_XYZ_Read(int16_t szData16[3]);
void ADXL345_IdRead(uint8_t *pId);
void ADXL345_REG_READ(uint8_t address, uint8_t *value);
void ADXL345_REG_WRITE(uint8_t address, uint8_t value);
void ADXL345_REG_MULTI_READ(uint8_t address, uint8_t values[], uint8_t len);

// I2C0 Functions
void I2C0_Init();
void I2C0_Enable_FPGA_Access();

// Pinmux Functions
void Pinmux_Config();

extern void *i2c_base, *sysmgr_base, *rstmgr_base, *l3regs_base;

#endif /*ACCELEROMETER_ADXL345_SPI_H_*/
