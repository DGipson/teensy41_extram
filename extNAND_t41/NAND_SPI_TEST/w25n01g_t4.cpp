#include "w25n01g_t4.h"


w25n01g_t4::w25n01g_t4() {}

void w25n01g_t4::begin() {
  configure_flash();
  init();
}


void w25n01g_t4::configure_flash()
{
  memset(flashID, 0, sizeof(flashID));

  delay(500);

  FLEXSPI2_FLSHA2CR0 = 134218;          //Flash Size in KByte, 1F400
  FLEXSPI2_FLSHA2CR1 = FLEXSPI_FLSHCR1_CSINTERVAL(2)  //minimum interval between flash device Chip selection deassertion and flash device Chip selection assertion.
                       | FLEXSPI_FLSHCR1_CAS(12)
                       | FLEXSPI_FLSHCR1_TCSH(3)                           //Serial Flash CS Hold time.
                       | FLEXSPI_FLSHCR1_TCSS(3);                          //Serial Flash CS setup time

  FLEXSPI2_FLSHA2CR2 = FLEXSPI_FLSHCR2_AWRSEQID(6)    //Sequence Index for AHB Write triggered Command
                       | FLEXSPI_FLSHCR2_AWRSEQNUM(0)                      //Sequence Number for AHB Read triggered Command in LUT.
                       | FLEXSPI_FLSHCR2_ARDSEQID(5)                       //Sequence Index for AHB Read triggered Command in LUT
                       | FLEXSPI_FLSHCR2_ARDSEQNUM(0);                     //Sequence Number for AHB Read triggered Command in LUT.

  // cmd index 7 = read ID bytes, 4*index number
  FLEXSPI2_LUT28 = LUT0(CMD_SDR, PINS1, RDID) | LUT1(DUMMY_SDR, PINS1, 8);
  FLEXSPI2_LUT29 = LUT0(READ_SDR, PINS1, 1);     //9fh Read JDEC

  // cmd index 8 = read Status register
  // set in function readStatusRegister(uint8_t reg, bool dump)

  //cmd index 9 - WG reset, see function deviceReset()
  FLEXSPI2_LUT36 = LUT0(CMD_SDR, PINS1, DEVICE_RESET);

  // cmd index 10 = write Status register
  // see function writeStatusRegister(uint8_t reg, uint8_t data)

  // cmd 11 index write enable cmd
  // see function writeEnable(bool wpE)

  //cmd 12 Command based on PageAddress
  // see functions:
  // eraseSector(uint32_t addr) and
  // readBytes(uint32_t address, uint8_t *data, int length)

  //cmd 13 program load Data
  // see functions:
  // programDataLoad(uint16_t columnAddress, const uint8_t *data, int length)
  // and
  // randomProgramDataLoad(uint16_t columnAddress, const uint8_t *data, int length)

  //cmd 14 program read Data -- reserved
  //FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS1, FAST_READ_QUAD_IO) | LUT1(CADDR_SDR, PINS4, 0x10);
  FLEXSPI2_LUT56 = LUT0(CMD_SDR, PINS1, FAST_READ_QUAD_OUTPUT) | LUT1(CADDR_SDR, PINS1, 0x10);
  FLEXSPI2_LUT57 = LUT0(DUMMY_SDR, PINS4, 8) | LUT1(READ_SDR, PINS4, 1);

  //cmd 15 - program execute
  FLEXSPI2_LUT60 = LUT0(CMD_SDR, PINS1, PROGRAM_EXECUTE) | LUT1(DUMMY_SDR, PINS1, 8);
  FLEXSPI2_LUT61 = LUT0(ADDR_SDR, PINS1, 0x20);

}

void w25n01g_t4::init() {
  if (flexspi2_flash_id(flashBaseAddr) == 0x21AA) {
    Serial.println("Found W25N01G Flash Chip");
  } else {
    Serial.println("no chip found!");
    exit(1);
  }

  // reset the chip
  deviceReset();
}



void w25n01g_t4::flexspi_ip_read(uint32_t index, uint32_t addr, void *data, uint32_t length)
{
  uint32_t n;
  uint8_t *p = (uint8_t *)data;
  const uint8_t *src;

  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index) | FLEXSPI_IPCR1_IDATSZ(length);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)) {
    if (n & FLEXSPI_INTR_IPRXWA) {
      //Serial.print("*");
      if (length >= 8) {
        length -= 8;
        *(uint32_t *)(p + 0) = FLEXSPI2_RFDR0;
        *(uint32_t *)(p + 4) = FLEXSPI2_RFDR1;
        p += 8;
      } else {
        src = (const uint8_t *)&FLEXSPI2_RFDR0;
        while (length > 0) {
          length--;
          *p++ = *src++;
        }
      }
      FLEXSPI2_INTR = FLEXSPI_INTR_IPRXWA;
    }
  }

  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.printf("Error (READ): FLEXSPI2_IPRXFSTS=%08lX\r\n", FLEXSPI2_IPRXFSTS);
    //Serial.print("ERROR: FLEXSPI2_INTR = "), Serial.println(FLEXSPI2_INTR, BIN);
  }

  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
  //Serial.printf(" FLEXSPI2_RFDR0=%08lX\r\n", FLEXSPI2_RFDR0);
  //if (length > 4) Serial.printf(" FLEXSPI2_RFDR1=%08lX\n", FLEXSPI2_RFDR1);
  //if (length > 8) Serial.printf(" FLEXSPI2_RFDR1=%08lX\n", FLEXSPI2_RFDR2);
  //if (length > 16) Serial.printf(" FLEXSPI2_RFDR1=%08lX\n", FLEXSPI2_RFDR3);
  src = (const uint8_t *)&FLEXSPI2_RFDR0;
  while (length > 0) {
    *p++ = *src++;
    length--;
  }
  if (FLEXSPI2_INTR & FLEXSPI_INTR_IPRXWA) FLEXSPI2_INTR = FLEXSPI_INTR_IPRXWA;
}


void w25n01g_t4::flexspi_ip_write(uint32_t index, uint32_t addr, const void *data, uint32_t length)
{
  const uint8_t *src;
  uint32_t n, wrlen;

  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index) | FLEXSPI_IPCR1_IDATSZ(length);
  src = (const uint8_t *)data;
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;

  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)) {

    if (n & FLEXSPI_INTR_IPTXWE) {
      wrlen = length;
      if (wrlen > 8) wrlen = 8;
      if (wrlen > 0) {

        //memcpy((void *)&FLEXSPI2_TFDR0, src, wrlen); !crashes sometimes!
        uint8_t *p = (uint8_t *) &FLEXSPI2_TFDR0;
        for (unsigned i = 0; i < wrlen; i++) *p++ = *src++;

        //src += wrlen;
        length -= wrlen;
        FLEXSPI2_INTR = FLEXSPI_INTR_IPTXWE;
      }
    }

  }

  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.printf("Error: FLEXSPI2_IPRXFSTS=%08lX\r\n", FLEXSPI2_IPRXFSTS);
  }

  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
}


void w25n01g_t4::flexspi_ip_command(uint32_t index, uint32_t addr)
{
  uint32_t n;
  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(index);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!((n = FLEXSPI2_INTR) & FLEXSPI_INTR_IPCMDDONE)); // wait
  if (n & FLEXSPI_INTR_IPCMDERR) {
    Serial.print("ERROR: FLEXSPI2_INTR = "), Serial.println(FLEXSPI2_INTR, BIN);
    Serial.printf("Error: FLEXSPI2_IPRXFSTS=%08lX\n", FLEXSPI2_IPRXFSTS);
  }
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE;
}

FLASHMEM uint32_t w25n01g_t4::flexspi2_flash_id(uint32_t addr)
{
  FLEXSPI2_IPCR0 = addr;
  FLEXSPI2_IPCR1 = FLEXSPI_IPCR1_ISEQID(7) | FLEXSPI_IPCR1_IDATSZ(5);
  FLEXSPI2_IPCMD = FLEXSPI_IPCMD_TRG;
  while (!(FLEXSPI2_INTR & FLEXSPI_INTR_IPCMDDONE)); // wait
  uint32_t id = FLEXSPI2_RFDR0;
  FLEXSPI2_INTR = FLEXSPI_INTR_IPCMDDONE | FLEXSPI_INTR_IPRXWA;
  //Serial.println(id >> 8 , HEX);
  return id >> 8;
}

uint8_t w25n01g_t4::readStatusRegister(uint8_t reg, bool dump)
{
  uint8_t val;

  // cmd index 8 = read Status register #1 SPI
  FLEXSPI2_LUT32 = LUT0(CMD_SDR, PINS1, READ_STATUS_REG) | LUT1(CMD_SDR, PINS1, reg);
  FLEXSPI2_LUT33 = LUT0(READ_SDR, PINS1, 1);

  flexspi_ip_read(8, flashBaseAddr, &val, 1 );

  if (dump) {
    Serial.printf("Status of reg 0x%x: \n", reg);
    Serial.printf("(HEX: ) 0x%02X, (Binary: )", val);
    Serial.println(val, BIN);
    Serial.println();
  }

  return val;

}

void w25n01g_t4::writeStatusRegister(uint8_t reg, uint8_t data)
{
  uint8_t buf[1];
  buf[0] = data;
  // cmd index 10 = write Status register
  FLEXSPI2_LUT40 = LUT0(CMD_SDR, PINS1, WRITE_STATUS_REG) | LUT1(CMD_SDR, PINS1, reg);
  FLEXSPI2_LUT41 = LUT0(WRITE_SDR, PINS1, 1);

  flexspi_ip_write(10, flashBaseAddr, buf, 1);

}


void w25n01g_t4::setTimeout(uint32_t timeoutMillis)
{
  uint32_t now = millis();
  timeoutAt = now + timeoutMillis;
}

void w25n01g_t4::deviceReset()
{
  flexspi_ip_command(9, flashBaseAddr); //reset

  setTimeout(TIMEOUT_RESET_MS);
  waitForReady();

  // No protection, WP-E off, WP-E prevents use of IO2
  writeStatusRegister(PROT_REG, PROT_CLEAR);
  readStatusRegister(PROT_REG, false);

  // Buffered read mode (BUF = 1), ECC enabled (ECC = 1)
  writeStatusRegister(CONF_REG, CONFIG_ECC_ENABLE | CONFIG_BUFFER_READ_MODE);
  //writeStatusRegister(CONF_REG, CONFIG_ECC_ENABLE);
  //writeStatusRegister(CONF_REG,  CONFIG_BUFFER_READ_MODE);
  readStatusRegister(CONF_REG, false);

}

bool w25n01g_t4::isReady()
{
  uint8_t status = readStatusRegister(STAT_REG, false);
  return ((status & STATUS_FLAG_BUSY) == 0);
}

bool w25n01g_t4::waitForReady()
{
  while (!w25n01g_t4::isReady()) {
    uint32_t now = millis();
    //if (now >= timeoutAt) {
    if (millis() - now >= timeoutAt) {
      return false;
    }
  }
  timeoutAt = 0;

  return true;
}

/**
 * The flash requires this write enable command to be sent before commands that would cause
 * a write like program and erase.
 */
void w25n01g_t4::writeEnable(bool wpE)
{
  if (wpE == true) {
    FLEXSPI2_LUT44 = LUT0(CMD_SDR, PINS1, WRITE_ENABLE);
  } else {
    FLEXSPI2_LUT44 = LUT0(CMD_SDR, PINS1, WRITE_DISABLE);
  }
  flexspi_ip_command(11, flashBaseAddr); //Write Enable
  // Assume that we're about to do some writing, so the device is just about to become busy
}


/**
 * Erase a sector full of bytes to all 1's at the given byte offset in the flash chip.
 */
void w25n01g_t4::eraseSector(uint32_t address)
{
  writeEnable(true);
  waitForReady();

  // cmd index 12
  FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS1, BLOCK_ERASE) | LUT1(DUMMY_SDR, PINS1, 8);
  FLEXSPI2_LUT49 = LUT1(ADDR_SDR, PINS1, 0x20);
  flexspi_ip_command(12, flashBaseAddr + LINEAR_TO_PAGE(address));

  uint8_t status = readStatusRegister(STAT_REG, false );
  if ((status &  STATUS_ERASE_FAIL) == 1)
    Serial.println( "erase Status: FAILED ");

  setTimeout( TIMEOUT_BLOCK_ERASE_MS);
}

//
// W25N01G does not support full chip erase.
// Call eraseSector repeatedly.
void deviceErase()
{
  for (uint32_t block = 0; block < sectors; block++) {
    w25n01g_t4::eraseSector(BLOCK_TO_LINEAR(block));
    Serial.print(".");
    if ((block % 128) == 0) Serial.println();
  }
  Serial.println();
}

void w25n01g_t4::programDataLoad(uint16_t Address, const uint8_t *data, int length)
{
  //DTprint(programDataLoad);
  writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)
  uint16_t columnAddress = LINEAR_TO_COLUMN(Address);

  waitForReady();


  FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS1, QUAD_PROGRAM_DATA_DATA_LOAD) | LUT1(CADDR_SDR, PINS1, 0x10);
  FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS4, 1);
  flexspi_ip_write(13, flashBaseAddr + columnAddress, data, length);

  setTimeout(TIMEOUT_PAGE_PROGRAM_MS);
  waitForReady();
  uint8_t status = readStatusRegister(STAT_REG, false );
  if ((status &  STATUS_PROGRAM_FAIL) == 1)
    Serial.println( "Programed Status: FAILED" );

}

void w25n01g_t4::randomProgramDataLoad(uint16_t Address, const uint8_t *data, int length)
{
  writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)
  uint16_t columnAddress = LINEAR_TO_COLUMN(Address);

  waitForReady();

  FLEXSPI2_LUT52 = LUT0(CMD_SDR, PINS1, RANDOM_PROGRAM_DATA_LOAD) | LUT1(CADDR_SDR, PINS1, 0x10);
  FLEXSPI2_LUT53 = LUT0(WRITE_SDR, PINS1, 1);
  flexspi_ip_write(13, flashBaseAddr + columnAddress, data, length);

  waitForReady();
  uint8_t status = readStatusRegister(STAT_REG, false );
  if ((status &  STATUS_PROGRAM_FAIL) == 1)
    Serial.println( "Programed Status: FAILED" );
  setTimeout(TIMEOUT_PAGE_PROGRAM_MS);

}

void w25n01g_t4::programExecute(uint32_t Address)
{
  writeEnable(true);   //sets the WEL in Status Reg to 1 (bit 2)

  uint32_t pageAddress = LINEAR_TO_PAGE(Address);
  waitForReady();

  flexspi_ip_command(15, flashBaseAddr + pageAddress);

  waitForReady();
  setTimeout(TIMEOUT_PAGE_PROGRAM_MS);
}

void w25n01g_t4::pageProgramDataLoad(uint16_t Address, const uint8_t *data, int length )
{
  uint8_t dataTemp[2048];
  uint16_t startAddress = Address;
  uint16_t columnStart = LINEAR_TO_COLUMN(Address);
  uint16_t startPage = LINEAR_TO_PAGE(Address);
  uint16_t transferLength = 0, numberFullPages = 0, remainingBytes = length, ii;
  uint16_t newStartAddr;

  if ( Address % PAGE_SIZE == 0) { //Determine Number of Pages
    numberFullPages = LINEAR_TO_PAGE(length);
  }

  waitForReady();

  //Serial.printf("numberFullPages: %d\n", numberFullPages); Serial.flush();

  remainingBytes = length;
  uint16_t bufTest = PAGE_SIZE * (startPage) - startAddress;

  //Serial.printf("startPage: %d, startAddress: %d\n", startPage,  startAddress); Serial.flush();
  //Serial.printf("BufTest: %d\n", bufTest); Serial.flush();


  //Check if first page a full if not transfer it
  if (bufTest > 0) {
    if (length < bufTest) {
      transferLength = length;
    } else {
      //Serial.println("Partial Transfer"); Serial.flush();
      transferLength = PAGE_SIZE * (startPage + 1) - startAddress;
    }
    //Serial.printf("Transfer 1st Partial: %d\n", transferLength); Serial.flush();
    randomProgramDataLoad(Address, data, transferLength);
    programExecute(Address);
    remainingBytes = remainingBytes - transferLength;
  }
  newStartAddr = Address + transferLength;
  //Serial.printf("newAddr after 1st Partial: %d\n", newStartAddr); Serial.flush();
  //Serial.printf("RemainingBytes after Partial: %d\n", remainingBytes); Serial.flush();

  for (ii = 0; ii < numberFullPages; ii++) {
    //Serial.println("Full Page Transfer"); Serial.flush();
    newStartAddr = newStartAddr + PAGE_SIZE * ii;
    //Serial.printf("newAddr for 1st FUll: %d\n", newStartAddr); Serial.flush();
    //Serial.println(transferLength); Serial.flush();
    //Serial.printf("Copy Src Start: %d, End: %d\n", transferLength+PAGE_SIZE * (ii),transferLength+PAGE_SIZE * (ii+1)-1);
    //Serial.flush();
    // https://stackoverflow.com/questions/11102029/how-can-i-copy-a-part-of-an-array-to-another-array-in-c
    std::copy(data + transferLength + PAGE_SIZE * (ii), data + transferLength + PAGE_SIZE * (ii + 1), dataTemp);
    programDataLoad(newStartAddr, dataTemp, PAGE_SIZE);
    programExecute(newStartAddr);
    remainingBytes = remainingBytes - PAGE_SIZE;
  }
  //Serial.printf("RemainingBytes after Full: %d\n", remainingBytes); //Serial.flush();

  //check last page for any remainder
  if (remainingBytes > 0) {
    //transfer from begining
    //Serial.println("Last transfer"); //Serial.flush();
    //Serial.println(transferLength+PAGE_SIZE*numberFullPages); //Serial.flush();
    std::copy(data + transferLength + PAGE_SIZE * numberFullPages , data + remainingBytes, dataTemp);
    randomProgramDataLoad(newStartAddr, dataTemp, remainingBytes);
    programExecute(newStartAddr);
    //Serial.printf("RemainingBytes after Last: %d\n", remainingBytes); Serial.flush();
  }
  //Serial.println("FINISHE WRITE BYTES!");  Serial.flush();

}

/**
 * Read `length` bytes into the provided `buffer` from the flash starting from the given `address` (which need not lie
 * on a page boundary).
 *
 * Waits up to TIMEOUT_PAGE_READ_MS milliseconds for the flash to become ready before reading.
 *
 * The number of bytes actually read is returned, which can be zero if an error or timeout occurred.
 */

// Continuous read mode (BUF = 0):
// (1) "Page Data Read" command is executed for the page pointed by address
// (2) "Read Data" command is executed for bytes not requested and data are discarded
// (3) "Read Data" command is executed and data are stored directly into caller's buffer
//
// Buffered read mode (BUF = 1), non-read ahead
// (1) If currentBufferPage != requested page, then issue PAGE_DATA_READ on requested page.
// (2) Compute transferLength as smaller of remaining length and requested length.
// (3) Issue READ_DATA on column address.
// (4) Return transferLength.

int w25n01g_t4::readBytes_old(uint32_t address, uint8_t *data, int length)
{
  writeEnable(false);
  uint32_t targetPage = LINEAR_TO_PAGE(address);

  if (currentPage != targetPage) {
    if (!waitForReady()) {
      Serial.println("Read Returning");
      return 0;
    }

    currentPage = UINT32_MAX;

    FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS1, PAGE_DATA_READ) | LUT1(DUMMY_SDR, PINS1, 8);
    FLEXSPI2_LUT49 = LUT0(ADDR_SDR, PINS1, 0x20);
    flexspi_ip_command(12, flashBaseAddr + targetPage);
    //Serial.println("Write Page Addr Complete");

    setTimeout(TIMEOUT_PAGE_READ_MS);
    if (!waitForReady()) {
      return 0;
    }

    currentPage = targetPage;
  }

  uint16_t column = LINEAR_TO_COLUMN(address);
  uint16_t transferLength;

  if (length > PAGE_SIZE - column) {
    transferLength = PAGE_SIZE - column;
  } else {
    transferLength = length;
  }

  flexspi_ip_read(14, flashBaseAddr + column, data, transferLength);

  // XXX Don't need this?
  setTimeout(TIMEOUT_PAGE_READ_MS);
  if (!waitForReady()) {
    return 0;
  }

  // Check ECC
  uint8_t statReg = readStatusRegister(STAT_REG, false);
  uint8_t eccCode = STATUS_FLAG_ECC(statReg);

  switch (eccCode) {
  case 0: // Successful read, no ECC correction
    break;
  case 1: // Successful read with ECC correction
  case 2: // Uncorrectable ECC in a single page
  case 3: // Uncorrectable ECC in multiple pages
    //addError(address, eccCode);
    Serial.printf("ECC Error (addr, code): %x, %x\n", address, eccCode);
    deviceReset();
    break;
  }

  return transferLength;
}

int w25n01g_t4::readBytes(uint32_t Address, uint8_t *data, int length)
{
  uint8_t dataTemp[2048];
  uint16_t startAddress = Address;
  uint16_t columnStart = LINEAR_TO_COLUMN(Address);
  uint16_t startPage = LINEAR_TO_PAGE(Address);
  uint16_t transferLength = 0, numberFullPages = 0, remainingBytes = length, ii;
  uint16_t newStartAddr;

  if (Address % PAGE_SIZE == 0) { //Determine Number of Pages
    numberFullPages = LINEAR_TO_PAGE(length);
  }

  remainingBytes = length;
  uint16_t bufTest = PAGE_SIZE * (startPage) - startAddress;

  setTimeout(TIMEOUT_PAGE_READ_MS);
  if (!waitForReady()) {
    Serial.println("Returned: Waiting for ready expired!");
  }

  //Check if first page is a full if not transfer it
  if (length < bufTest) {
    if (length < bufTest) {
      transferLength = length;
    } else {
      //Serial.println("Partial Transfer");
      transferLength = PAGE_SIZE * (startPage + 1) - startAddress;
    }
    read(startAddress, data, transferLength);
    remainingBytes = remainingBytes - transferLength;
  }
  newStartAddr = Address + transferLength;
  //Serial.printf("newAddr after 1st Partial: %d\n", newStartAddr);
  //Serial.printf("RemainingBytes after Partial: %d\n", remainingBytes);


  for (ii = 0; ii < numberFullPages; ii++) {
    newStartAddr = newStartAddr + PAGE_SIZE * ii;
    read(newStartAddr, dataTemp, PAGE_SIZE);
    for (uint16_t ic = 0; ic < PAGE_SIZE; ic++) {
      data[ic + transferLength + PAGE_SIZE * ii] = dataTemp[ic];
      //Serial.printf("%d, %d, %d\n", ii, ic+transferLength+PAGE_SIZE * ii, data[ic+transferLength+PAGE_SIZE * ii]);
    }
    remainingBytes = remainingBytes - PAGE_SIZE;
  }
  newStartAddr = Address + transferLength;

  //check last page for any remainder
  if (remainingBytes > 0) {
    //transfer from begining
    read(newStartAddr, dataTemp, remainingBytes);
    for (uint16_t ic = 0; ic < remainingBytes; ic++) {
      data[ic + transferLength + numberFullPages * PAGE_SIZE] = dataTemp[ic];
    }
  }

}


void w25n01g_t4::read(uint32_t address, uint8_t *data, int length)
{
  uint32_t targetPage = LINEAR_TO_PAGE(address);
  waitForReady();

  FLEXSPI2_LUT48 = LUT0(CMD_SDR, PINS1, PAGE_DATA_READ) | LUT1(DUMMY_SDR, PINS1, 8);
  FLEXSPI2_LUT49 = LUT0(ADDR_SDR, PINS1, 0x20);
  flexspi_ip_command(12, flashBaseAddr + targetPage);
  //Serial.println("Write Page Addr Complete");

  setTimeout(TIMEOUT_PAGE_READ_MS);

  uint16_t column = LINEAR_TO_COLUMN(address);

  // XXX Don't need this?
  setTimeout(TIMEOUT_PAGE_READ_MS);
  if (!waitForReady()) {
    Serial.println("READ Command TIMEOUT !!!");
  }


  flexspi_ip_read(14, flashBaseAddr + column, data, length);

  // XXX Don't need this?
  setTimeout(TIMEOUT_PAGE_READ_MS);
  if (!waitForReady()) {
    Serial.println("READ TIMEOUT !!!");
  }



  // Check ECC
  uint8_t statReg = readStatusRegister(STAT_REG, false);
  uint8_t eccCode = STATUS_FLAG_ECC(statReg);

  switch (eccCode) {
  case 0: // Successful read, no ECC correction
    break;
  case 1: // Successful read with ECC correction
  case 2: // Uncorrectable ECC in a single page
  case 3: // Uncorrectable ECC in multiple pages
    //addError(address, eccCode);
    Serial.printf("ECC Error (addr, code): %x, %x\n", address, eccCode);
    deviceReset();
    break;
  }

}


/* Not used yet */
void w25n01g_t4::setBufMode(uint8_t bufMode) {
  /* bufMode = 1:
    * The Buffer Read Mode (BUF=1) requires a Column Address to start outputting the
    * existing data inside the Data Buffer, and once it reaches the end of the data
    * buffer (Byte 2,111)
    *
   * bufMode = 0:
    * The Continuous Read Mode (BUF=0) doesn’t require the starting Column Address.
    * The device will always start output the data from the first column (Byte 0) of the
    * Data buffer, and once the end of the data buffer (Byte 2,048) is reached, the data
    * output will continue through the next memory page.
    *
    * With Continuous Read Mode, it is possible to read out the entire memory array using
    * a single read command.
    */

  // Set mode after device reset.

  if (bufMode == 1) {
    writeStatusRegister(CONF_REG, CONFIG_ECC_ENABLE | CONFIG_BUFFER_READ_MODE);
  } else if (bufMode == 0) {
    writeStatusRegister(CONF_REG, CONFIG_ECC_ENABLE);
  } else {
    Serial.println("Mode not supported !!!!");
  }

}

