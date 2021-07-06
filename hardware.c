/* for ioctl() */
#include <sys/ioctl.h>
/* for i2c defines */
/*#include <linux/i2c.h>*/
/* for i2c structs and functions */
#include <linux/i2c-dev.h>
/* for open() */
#include <fcntl.h>
/* for close() */
#include <unistd.h>
/* for strto* */
#include <stdlib.h>
#include <stdint.h>
/*for errno*/
#include <errno.h>
/*for strerror*/
#include <string.h>  
#include <stdio.h>
#define OK 0
#define ERROR -1
/*#define WITH_DRIVER*/
#define INPHI_100G 0x00
#define INPHI_400G 0x01
#define CREDO_100G 0x10
#define CREDO_400G 0x11
/**
 * Read register over I2C.
 * 
 * Meant for user-land linux applications.
 * 
 * @param i2c_file      [I] - Opened I2C /dev file
 * @param slave_address [I] - Un-shifted slave address of the device
 * @param register_addr [I] - Register to read on the device
 * @param read_data     [O] - Register data read from the device
 * 
 * @return OK on success, ERROR otherwise
 */
int i2c_read(int i2c_file, uint8_t slave_address, uint16_t register_addr, uint16_t *read_data)
{    
    uint8_t  reg_addr_low = 0;
    uint8_t data = 0;
    reg_addr_low =  (register_addr & 0xFF);
  
    *read_data = 0xbada;

    /* Using I2C Read, equivalent of i2c_smbus_read_byte(file) */

    /*if (read(i2c_file, buf, 1) != 1) {*/
    /*__s32 i2c_smbus_read_byte(int file);*/
    data = i2c_smbus_read_byte_data(i2c_file, reg_addr_low);
    if (data<0) {
        /* ERROR HANDLING: i2c transaction failed */
        printf("  i2c_file: 0x%x slave_address: 0x%x register_addr: 0x%x \n", \
            i2c_file, slave_address, reg_addr_low);
        return ERROR;
    } else {
        /*  contains the read byte */
        *read_data = data;
    }    
    return OK;
}

/**
 * Write register over I2C.
 * 
 * Meant for user-land linux applications.
 * 
 * @param i2c_file      [I] - Opened I2C /dev file
 * @param slave_address [I] - Un-shifted slave address of the device
 * @param register_addr [I] - Register to write on the device
 * @param write_data    [O] - Register data to write to the device
 * 
 * @return OK on success, ERROR otherwise
 */
int i2c_write(int i2c_file, uint8_t slave_address, uint16_t register_addr, uint16_t write_data)
{
    
    uint8_t  reg_addr_low = 0;
    uint8_t  write_data_low = 0;
   
    /* slave address will be shifted by one and then the read/write bit set by the i2c-dev functions */
    
    
    reg_addr_low =  (register_addr & 0xFF);
    
    write_data_low =  (write_data & 0xFF);
     
    if (i2c_smbus_write_byte_data(i2c_file,reg_addr_low,write_data_low) < 0) {
        /* ERROR HANDLING: i2c transaction failed */        
        printf("  i2c_file: 0x%x slave_address: 0x%x register_addr: 0x%x i2c data: 0x%x \n", \
                i2c_file, slave_address, register_addr, write_data_low);
        return ERROR;
    }
    
    return OK;
}


/**
 * Opens an I2C interface on /dev
 * 
 * To close, just use close(i2c_file)
 * 
 * @param dev_number [I] - i2c device under /dev/i2c-*
 * @param i2c_file   [O] - Resulting I2C file descriptor
 * 
 * @return OK on success, ERROR otherwise
 */
int i2c_open(int dev_number, int *i2c_file)
{
    int status = OK;
    char filename[20];
    
    if(sprintf(filename, "/dev/i2c-%d", dev_number) < 0) {
        return ERROR;
    }
    
    /*printf("\nOpen i2c dev %s\n",filename);*/
    *i2c_file = open(filename,O_RDWR);
    if(i2c_file < 0) {
        printf("Cannot open %s. Return code: %p  errno: %s\n", filename, i2c_file, strerror(errno));
        *i2c_file = 0;
        return ERROR;
    }
    
    return status;
}

uint16_t card_reset(uint16_t card)
{    

    int status = OK;
#ifdef WITH_DRIVER    
    char cmd[64];   
    char filepath[64];
    memset(cmd,0,64);  
    memset(filepath,0,64); 
    sprintf(filepath,"/sys/bus/i2c/devices/%d-0032/phy_reset", card);
    if( access( filepath, 0 ) == 0 ) {  
        sprintf(cmd,"echo 0 > %s", filepath,card);
        system(cmd);
        usleep(100);
        memset(cmd,0,64); 
        sprintf(cmd,"echo 1 > %s", filepath,card);
        system(cmd);   
    }
    return OK;
#else    
    int i2c_file;
    int i=0;
    uint16_t card_present = 0;
    if(i2c_open(0, &i2c_file) != OK) {
        printf("Could not open i2c device \n");
        return 0;
    }
    /* read i2c reg:
     * r slave_addr reg_addr */
    #if 0     
    uint8_t  slave_address = 0x30;
    uint16_t register_addr = 0xa2;
    uint16_t read_data = 0;
    uint16_t write_data = 0;
    
    if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {        
        printf("Could not open i2c device \n");
        close(i2c_file);
        return 0;            
    }                
    status |= i2c_read(i2c_file, slave_address, register_addr, &read_data);
    if(status != OK) {
        printf("Could not open i2c device %d \n",__LINE__);
        return 0; 
    }  
    write_data = read_data^(0x1<<card-1);
    printf("i2c_write 0x%x 0x%x = 0x%x\n",slave_address, register_addr,write_data);
    i2c_write(i2c_file,slave_address, register_addr,write_data); 
    
    write_data = read_data|(0x1<<card-1);
    printf("i2c_write 0x%x 0x%x = 0x%x\n",slave_address, register_addr,write_data);
    i2c_write(i2c_file,slave_address, register_addr,write_data); 
    #else
    uint8_t  slave_address = 0x73;
    uint16_t register_addr = 0x0;
    uint16_t read_data = 0;
    uint16_t write_data = (0x1<<card-1);    
    if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {        
        printf("Could not open i2c device \n");
        close(i2c_file);
        return 0;            
    }                 
    i2c_write(i2c_file,slave_address, register_addr,write_data); 
    
    slave_address=0x32;
    register_addr=0xa0;/*reset reg*/
    
    if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("Could not open i2c device %d \n",__LINE__);
        close(i2c_file);
        return 0;
    }    
    i2c_write(i2c_file,slave_address, register_addr,0x00); 
    i2c_write(i2c_file,slave_address, register_addr,0xff); 
    #endif    
    close(i2c_file);
#endif /*end of WITH_DRIVER*/
    return OK;
}
uint16_t card_detection(uint16_t *card_type)
{
#ifdef WITH_DRIVER
    FILE * fp;
    FILE * fp2;
    char str[64];
    char filepath[64];
    char *loc;
    int i=0;
    uint16_t card_present = 0;
    uint8_t power_status[8] = {0,0,0,0,0,0,0,0};
    fp2 = fopen ("/sys/class/hwmon/hwmon2/device/ESC600_Module/module_power", "r");     
    
    if(fp2 == NULL) {
        printf("File open error (%s)\n","/sys/class/hwmon/hwmon2/device/ESC600_Module/module_power");
        return 0;
    }       
    
    while(fgets(str, 50, fp2) != NULL) { 
        if (strstr (str,"Module")==NULL)
            continue;                
        loc = strstr (str,"not power good");
        if(loc == NULL) {    /*power good*/
            power_status[i]=1;
            printf("slot %d %s\n",i,"power good");
        }
        else {
            power_status[i]=0;
            printf("slot %d %s\n",i,"not power good");
        }  
        i++;
    }     
    fclose(fp2);
    #if 0
    i=0;
    while(fgets(str, 64, fp) != NULL) { 
        if (strstr (str,"Slot")==NULL)
            continue;        
        printf("slot %d ",i);
        loc = strstr (str,"400G");
        if(loc == NULL) {
            loc = strstr (str,"100G");  
            if (loc != NULL){
                printf("SPEED = %s",loc);
                card_type[i]=CREDO_100G;
            }
            else {
                printf("\n");
                card_type[i]=0xff;
            }                              
        }
        else {
            printf("SPEED = %s",loc);
            card_type[i]=CREDO_400G;
        }  
        card_present |= (0x1<<i);
        i++;
    }     
    fclose(fp);
    #else
    for (i=0;i<8;i++) {
        if (power_status[i]==0) {
            card_type[i]=0xff;
            continue;
        }
        
        memset(filepath,0,64);
        sprintf(filepath,"/sys/bus/i2c/devices/%d-0032/model",i+1);        
        fp = fopen (filepath, "r"); 
        
        if(fp == NULL) {
            card_type[i]=0xff;
            continue;
        }
        
        while(fgets(str, 64, fp) != NULL) { 
            if (strstr (str,"Inphi")==NULL && strstr (str,"Credo")==NULL)
                continue;
            loc = strstr (str,"Inphi 100G");
            if(loc != NULL) {
                card_type[i]=INPHI_100G;
                printf("slot %d %s\n",i,"Inphi 100G");
                card_present |= (0x1<<i); 
                break;
            }            
            loc = strstr (str,"Inphi 400G");
            if(loc != NULL) {
                card_type[i]=INPHI_400G;
                card_present |= (0x1<<i); 
                printf("slot %d %s\n",i,"Inphi 400G");
                break;
            }
            loc = strstr (str,"Credo 100G");
            if(loc != NULL) {
                card_type[i]=CREDO_100G;
                card_present |= (0x1<<i); 
                printf("slot %d %s\n",i,"Credo 100G");
                break;
            }
            loc = strstr (str,"Credo 400G");
            if(loc != NULL) {
                card_type[i]=CREDO_400G;
                card_present |= (0x1<<i); 
                printf("slot %d %s\n",i,"Credo 400G");
                break;
            }                                   
        }     
        fclose(fp);        
    }
    /*cat /sys/bus/i2c/devices/*-0032/model*/
    #endif
#else           
    int status = OK;
    int i2c_file;
    int i=0;
    uint16_t card_present = 0;
    if(i2c_open(0, &i2c_file) != OK) {
        printf("Could not open i2c device \n");
        return 0;
    }
    /* read i2c reg:
     * r slave_addr reg_addr */
    uint8_t  slave_address = 0x30;
    uint16_t register_addr = 0xa3;
    uint16_t read_data = 0;
    uint16_t write_data = 0;
    /*printf("slave: 0x%x  reg: 0x%x\n",slave_address, register_addr);*/
    if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        printf("Could not open i2c device \n");
        close(i2c_file);
        return 0;            
    }                
    status |= i2c_read(i2c_file, slave_address, register_addr, &card_present);
    if(status != OK) {
        printf("Could not open i2c device %d \n",__LINE__);
        return 0; 
    }  
    
    for (i=0;i<8;i++){
        if(card_present & (0x1<<i)) {
            slave_address=0x73;
            register_addr=0x0;
            write_data = (0x1<<i);
         
            if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {
                /* ERROR HANDLING; you can check errno to see what went wrong */
                printf("Could not open i2c device %d \n",__LINE__);
                close(i2c_file);
                return 0;            
            }              
            i2c_write(i2c_file,slave_address, register_addr,write_data);                 
            slave_address=0x32;
            register_addr=0xb0;            
            if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {
                /* ERROR HANDLING; you can check errno to see what went wrong */
                printf("Could not open i2c device %d \n",__LINE__);
                close(i2c_file);
                return 0;            
            }                        
            status |= i2c_read(i2c_file, slave_address, register_addr, &read_data);                      
            if(status != OK) {
                printf("i2c read error! \n");
                return 0; 
            }            
            card_type[i]=read_data;
        }
    }
    close(i2c_file);  
#endif /*end of WITH_DRIVER*/  
    return card_present;
}

/* this define makes it easy to pull the i2c functions into another project */
#ifdef I2C_EXAMPLE

/* https://www.kernel.org/doc/Documentation/i2c/dev-interface
 * 
 * make sure to load the i2c-dev kernel module
 * $ sudo modprobe i2c-dev
 * On the beaglebone black (used for this example) we're using the i2c-1 interface which
 * is available on the headers (although the headers call it i2c-2!)
 */

int main(int argc, char **argv)
{
    int status = OK;
    int i2c_file;
    
    if(i2c_open(0, &i2c_file) != OK) {
        printf("Could not open i2c device\n");
        return 1;
    }
    
    if( (argc >= 4) && (argv[1][0] == 'r') ) {
        /* read i2c reg:
         * r slave_addr reg_addr */
        uint8_t  slave_address = strtoul(argv[2], NULL, 0);
        uint16_t register_addr = strtoul(argv[3], NULL, 0);
        uint16_t read_data = 0;
        /*printf("slave: 0x%x  reg: 0x%x\n",slave_address, register_addr);*/
        if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {
            /* ERROR HANDLING; you can check errno to see what went wrong */
            printf("ioctl failed\n");
            close(i2c_file);
            return 1;            
        }     
           
        status |= i2c_read(i2c_file, slave_address, register_addr, &read_data);
        if(status != OK) {
            printf("Read failed\n");
        }
        else {
            printf("0x%04x = 0x%04x\n", register_addr, read_data);
        }
    }
    else if( (argc >= 5) && (argv[1][0] == 'w') ) {
        /* write i2c reg:
         * w slave_addr reg_addr value */
        uint16_t slave_address = strtoul(argv[2], NULL, 0);
        uint16_t register_addr = strtoul(argv[3], NULL, 0);
        uint16_t write_data    = strtoul(argv[4], NULL, 0);
        if (ioctl(i2c_file, I2C_SLAVE, slave_address) < 0) {
            /* ERROR HANDLING; you can check errno to see what went wrong */
            printf("Could not open i2c device #2\n");
            close(i2c_file);
            return 1;            
        }           
        status |= i2c_write(i2c_file, slave_address, register_addr, write_data);
        printf("0x%04x set to 0x%04x\n", register_addr, write_data);
        if(status != OK) {
            printf("Write failed\n");
        }
    }
    else {
        printf("Wrong arguments\n");
        printf("  read : %s r slave_address register_addr\n", argv[0]);
        printf("  write: %s w slave_address register_addr write_data\n", argv[0]);
    }
    
    close(i2c_file);
    
    if(status != OK) {
        return 1;
    }
    else {
        return 0;
    }
}

#endif
