/**************************************************************************/
/*! 
    @file     spiffs_t4.h
	
    @section  HISTORY

    v1.0 - First release

    Driver for the 
	

*/
/**************************************************************************/
#ifndef _SPIFFS_T4_H_
#define _SPIFFS_T4_H_

#if !defined(ARDUINO_TEENSY41)
#error "Sorry, extFlashSpiffs_t4 does works only with Teens 4.1"
#endif


#if ARDUINO >= 100
 #include <Arduino.h>
#else
 #include <WProgram.h>
#endif

#include <Wire.h>

#include <spiffs.h>

// Enabling debug I2C - comment to disable / normal operations
#ifndef SERIAL_DEBUG
//  #define SERIAL_DEBUG 1
#endif

#define LUT0(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)))
#define LUT1(opcode, pads, operand) (FLEXSPI_LUT_INSTRUCTION((opcode), (pads), (operand)) << 16)
#define CMD_SDR         FLEXSPI_LUT_OPCODE_CMD_SDR
#define ADDR_SDR        FLEXSPI_LUT_OPCODE_RADDR_SDR
#define READ_SDR        FLEXSPI_LUT_OPCODE_READ_SDR
#define WRITE_SDR       FLEXSPI_LUT_OPCODE_WRITE_SDR
#define DUMMY_SDR       FLEXSPI_LUT_OPCODE_DUMMY_SDR
#define PINS1           FLEXSPI_LUT_NUM_PADS_1
#define PINS4           FLEXSPI_LUT_NUM_PADS_4

// IDs
//Manufacturers codes

// Error management
#define ERROR_0 0 // Success    
#define ERROR_1 1 // Data too long to fit the transmission buffer on Arduino
#define ERROR_2 2 // received NACK on transmit of address
#define ERROR_3 3 // received NACK on transmit of data
#define ERROR_4 4 // Serial seems not available
#define ERROR_5 5 // Not referenced device ID
#define ERROR_6 6 // Unused
#define ERROR_7 7 // Fram chip unidentified
#define ERROR_8 8 // Number of bytes asked to read null
#define ERROR_9 9 // Bit position out of range
#define ERROR_10 10 // Not permitted opération
#define ERROR_11 11 // Memory address out of range

#define FLASH_MEMMAP 1 //Use memory-mapped access

struct dir
{
  char filename[16][32];
  uint16_t fnamelen[16];
  uint32_t fsize[16];
  uint8_t fid[16];
};


class spiffs_t4 : public Print
{
 public:
	spiffs_t4();
	//int8_t  begin(uint8_t config, uint8_t spiffs_region = 0);
	int8_t  begin();
	
	static void printStatusRegs();
	
	void fs_mount();
	void fs_unmount();

	static s32_t fs_erase(u32_t addr, u32_t size);	
	static s32_t spiffs_write(u32_t addr, u32_t size, u8_t * src);
	static s32_t spiffs_read(u32_t addr, u32_t size, u8_t * dst);
	void eraseFlashChip();
	void eraseDevice(void);
	void fs_space(uint32_t * total1, uint32_t *used1);
	dir fs_getDir(uint16_t * numrecs) ;

	void fs_listDir();
	
	int f_open(spiffs_file &fd, const char* fname, spiffs_flags flags);
	int f_write(spiffs_file fd, const char *dst, int szLen);
	int f_read(spiffs_file fd, const char *dst, int szLen);
	int f_writeFile(const char* fname, const char *dst, spiffs_flags flags);
	int f_readFile(const char* fname, const char *dst, int szLen, spiffs_flags);

	int f_close_write(spiffs_file fd);
	void f_close(spiffs_file fd);

	int32_t f_position(spiffs_file fd );
	int f_eof( spiffs_file fd );
	int f_seek(spiffs_file fd ,int32_t offset, int start);
	int f_rename(const char* fname_old, const char* fname_new);
	int f_remove(const char* fname);
	void f_info(const char* fname, spiffs_stat *s);
	
	// overwrite print functions:
	void printTo(spiffs_file fd);
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buffer, size_t size);
	
	static void flexspi_ip_command(uint32_t index, uint32_t addr);
	static void flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length);
	static void flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length);
	static bool waitFlash(uint32_t timeout = 0);
	
 private:


};

#endif
