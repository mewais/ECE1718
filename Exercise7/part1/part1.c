#include "ADXL345.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>

void *i2c_base, *sysmgr_base, *rstmgr_base, *l3regs_base;

int main(void){
	int fd;
    uint8_t devid;
    int16_t mg_per_lsb = 4;
    int16_t XYZ[3];
	if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf ("ERROR could not open /dev/mem\n");
		return -1;
	}
	i2c_base = (char *) mmap (NULL, I2C_RANGE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, I2C_ADDR);
	sysmgr_base = (char *) mmap (NULL, SYSMGR_RANGE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, SYSMGR_ADDR);
	rstmgr_base = (char *) mmap (NULL, RSTMGR_RANGE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, RSTMGR_ADDR);
	l3regs_base = (char *) mmap (NULL, L3REGS_RANGE, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, L3REGS_ADDR);

    // Configure Pin Muxing
    Pinmux_Config();
    
    // Initialize I2C0 Controller
    I2C0_Init();
    
    // 0xE5 is read from DEVID(0x00) if I2C is functioning correctly
    ADXL345_REG_READ(0x00, &devid);
    
    // Correct Device ID
    if (devid == 0xE5){
        // Initialize accelerometer chip
        ADXL345_Init();
        
        while(1){
            if (ADXL345_WasActivityUpdated()){
                ADXL345_XYZ_Read(XYZ);
                printf("X=%d mg, Y=%d mg, Z=%d mg\n", XYZ[0]*mg_per_lsb, XYZ[1]*mg_per_lsb, XYZ[2]*mg_per_lsb);
            }
        }
    } else {
        printf("Incorrect device ID\n");
    }

	munmap(i2c_base, I2C_RANGE);
	munmap(sysmgr_base, SYSMGR_RANGE);
	munmap(rstmgr_base, RSTMGR_RANGE);
	munmap(l3regs_base, L3REGS_RANGE);
    close(fd);
    return 0;
}
