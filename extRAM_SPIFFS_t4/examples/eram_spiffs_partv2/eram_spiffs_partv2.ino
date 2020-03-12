/*
   This test uses the optional quad spi flash on Teensy 4.1
   https://github.com/pellepl/spiffs/wiki/Using-spiffs
   https://github.com/pellepl/spiffs/wiki/FAQ

   ATTENTION: Flash needs to be empty before first use of SPIFFS


   Frank B, 2020
*/

// do dtSTART; and dtEND( "what") to show time
elapsedMicros _dt;
#define dtSTART {_dt=0;}
#define dtEND(a) { Serial.printf( "\n%s()_%s : dt %lu us", __func__, a, (uint32_t)_dt);}
// do dtSTART; and dtEND( "what") to show time

#include <extRAM_t4.h>
#include <spiffs.h>

extRAM_t4 eRAM;

uint8_t config = 0; //0 - init eram only, 1-init flash only, 2-init both
uint8_t spiffs_region = 2;  //0 - flash, 1 - eram
//2 - 2 4meg eram pseudo partitions
//#define DO_DEBUG 1

char buf[512] = "Hello World! What a wonderful World :)";
char fname[32] = "my_file1";
int szLen = strlen( buf );
elapsedMicros my_us;


void setup() {
  while (!Serial);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
#if 1
  Serial.println("\n Enter 'y' in 6 seconds to format FlashChip - other to skip");
  uint32_t pauseS = millis();
  char chIn = 9;
  while ( pauseS + 6000 > millis() && 9 == chIn ) {
    if ( Serial.available() ) {
      do {
        if ( chIn != 'y' )
          chIn = Serial.read();
        else
          Serial.read();
      }
      while ( Serial.available() );
    }
  }
  if ( chIn == 'y' ) {
    eRAM.begin(config, spiffs_region);
    if (spiffs_region == 0) {
      eRAM.eraseFlashChip();
    } else {
      eRAM.eraseDevice();
    }
  }
#endif

  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.begin(config, spiffs_region);
  eRAM.fs_mount();

#if 1
  Serial.println("Write file:");
  Serial.println(buf);
  eRAM.f_writeFile(fname, buf, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
#endif

  Serial.println();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  memset(buf, 0, sizeof(buf)); //emtpy buffer
  Serial.println("Read file:");
  eRAM.f_readFile(fname, buf, sizeof(buf) / sizeof(char), SPIFFS_RDWR);
  Serial.println(buf);

  Serial.println("==================================================");
  Serial.println();
  Serial.println("Mount SPIFFS:");
  eRAM.fs_mount();
  loopTest3R();
  Serial.println("Directory contents:");
  eRAM.fs_listDir();

  //test of print
  Serial.println();
  Serial.println("Using println and printf to printoutput file");
  eRAM.f_open("PRINTOUTPUT", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  eRAM.println("THIS IS A TEST");
  eRAM.printf("Float: %f, Int: %c\n", 26.4, 98);
  eRAM.f_close();

  Serial.println("Directory contents:");
  eRAM.fs_listDir();
  Serial.println();
  Serial.println("Test print output:");
  eRAM.f_open("PRINTOUTPUT", SPIFFS_RDONLY);
  eRAM.f_read(buf, 512);
  eRAM.f_close();
  Serial.println(buf);
  Serial.println("==================================================");

}

void loop() {
  char chIn = 255;
  if ( Serial.available() ) {
    do {
      if ( chIn < '0' || chIn > '5' )
        chIn = Serial.read();
      else
        Serial.read();
    }
    while ( Serial.available() );
  }
  if ( chIn == '0' ) {
    dtSTART
    eRAM.fs_listDir();
    dtEND( "0 :: Directory contents:");
    Serial.println();
  }
  else if ( chIn == '1' ) {
    Serial.println("\n1 :: Loop Test1 - New Files");
    loopTest1();
    Serial.println();
  }
  else if ( chIn == '2' ) {
    Serial.println("\n2 :: Loop Test2 - Overwrite");
    loopTest2();
    Serial.println();
  }
  else if ( chIn == '3' ) {
    Serial.println("\n3 :: Loop Test3 - Partition Test");
    loopTest3();
    Serial.println();
  }
  else if ( chIn == '4' ) {
    Serial.println("\n4 :: Loop Test4 - check24()");
    check24();
    Serial.println();
  }
  else if ( chIn == '5' ) {
    Serial.println("\n5 :: Loop Test5 - READ ONLY Verify Test");
    loopTest3R();
    Serial.println();
  }
}

void loopTest2() {
  char xData[12048], xData1[12048];

  char fname1[52] = "loopTest2";

  /**
     Opens/creates a file.
     @param fs      the file system struct
     @param path    the path of the new file
     @param flags   the flags for the open command, can be combinations of
                    SPIFFS_APPEND, SPIFFS_TRUNC, SPIFFS_CREAT, SPIFFS_RDONLY,
                    SPIFFS_WRONLY, SPIFFS_RDWR, SPIFFS_DIRECT, SPIFFS_EXCL
  **/
  dtSTART;
  eRAM.f_open(fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(xData, 26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.f_close();
  dtEND( "write:");

  Serial.println("\nDirectory contents:");
  eRAM.fs_listDir();

  dtSTART;
  eRAM.f_open(fname1, SPIFFS_RDWR);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.f_read(xData1, 2600);
  //SPIFFS_close(&fs, fd);
  eRAM.f_close();
  dtEND( "f_read:");

#ifdef DO_DEBUG
  Serial.println();
  for ( int ii = 0; ii < 100; ii++) {
    Serial.println("  <<< eRAM");
    for ( int jj = 0; jj < 26; jj++) {
      Serial.print( xData1[jj + (ii * 26)]);
    }
    Serial.println("  <<< FLASH");
  }
#endif
}

void loopTest1() {
  char xData[12048], xData1[12048];

  char fname1[52];
  static char fIdx = 'A';
  snprintf(fname1, 52, "lt1_%s_%c", __TIME__, fIdx++);
  if ( fIdx > 'Z' ) fIdx = 'A';

  //spiffs_file fd1 = SPIFFS_open(&fs, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  dtSTART;
  eRAM.f_open(fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(xData, 26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.f_close();


  Serial.println("\nDirectory contents:");
  eRAM.fs_listDir();

  eRAM.f_open(fname1, SPIFFS_RDWR);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.f_read(xData1, 2600);
  //SPIFFS_close(&fs, fd);
  eRAM.f_close();
  dtEND( "write");


#ifdef DO_DEBUG
  Serial.println();
  for ( int ii = 0; ii < 100; ii++) {
    Serial.println("  <<< eRAM");
    for ( int jj = 0; jj < 26; jj++) {
      Serial.print( xData1[jj + (ii * 26)]);
    }
    Serial.println("  <<< FLASH");
  }
#endif
}

void loopTest3() {
  char xData[12048], xData1[12048], xData2[26];

  uint32_t part_offset = 4194304 + 1;

  Serial.println("Loop Test 3");

  char fname1[32] = "loopTest3";
  //spiffs_file fd1 = SPIFFS_open(&fs, fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  dtSTART;
  eRAM.f_open(fname1, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    //if (SPIFFS_write(&fs, fd1, (u8_t *)xData, 26) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
    eRAM.f_write(xData, 26);
  }
  //SPIFFS_close(&fs, fd1);
  //SPIFFS_fflush(&fs, fd1);
  eRAM.f_close();
  dtEND( "write");

  //  Serial.println("Directory contents:");
  //  eRAM.fs_listDir();

  eRAM.f_open(fname1, SPIFFS_RDONLY);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.f_read(xData1, 2600);
  //SPIFFS_close(&fs, fd);
  eRAM.f_close();

  dtSTART;
  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj] = 'A' + jj;
      else
        xData[jj] = 'a' + jj;
    }
    eRAM.writeArray(ii * 26 + part_offset, 26, (uint8_t*)xData);
  }
  dtEND( "writeArray");

#if 1
  Serial.println();
  int jj = 0;
  for ( int ii = 0; ii < 100; ii++) {
    eRAM.readArray(ii * 26 + part_offset, 26, (uint8_t*)xData2);
    // DEBUG if ( ii == 13 ) xData2[13]='4'; // Prove test find mismatch

    if ( memcmp( xData2, &xData1[jj], 26) || ii < 3) {
      Serial.print('\n');
      for ( int kk = 0; kk < 26; kk++) {
        Serial.print( xData2[kk]);
      }
      Serial.print("  <<< eRAM @row #=");
      Serial.println(ii);
      for ( int ll = 0; ll < 26; ll++) {
        Serial.print( xData1[ll + (ii * 26)]);
      }
      Serial.println("  <<< FLASH");
    }
    else
      Serial.print("+");
    if ( ii == 49 )
      Serial.print('\n');
    // DEBUG
    // if ( ii == 98 ) jj++; // Prove test find mismatch { trigger but won't print error from ll}
    jj += 26;
  }
#endif
}

void loopTest3R() {
  char xData[12048], xData1[12048];
  Serial.println("Loop Test 3R -  READ ONLY Verify");

  char fname1[32] = "loopTest3";
  dtSTART

  eRAM.f_open(fname1, SPIFFS_RDONLY);
  //if (SPIFFS_read(&fs, fd, (u8_t *)xData1, 2600) < 0) Serial.printf("errno %i\n", SPIFFS_errno(&fs));
  eRAM.f_read(xData1, 2600);
  //SPIFFS_close(&fs, fd);
  eRAM.f_close();
  dtEND( "5 :: Loop Test5 - READ ONLY Verify Test");

  for ( int ii = 0; ii < 100; ii++) {
    for ( int jj = 0; jj < 26; jj++) {
      if ( ii % 2 )
        xData[jj + (ii * 26)] = 'A' + jj;
      else
        xData[jj + (ii * 26)] = 'a' + jj;
    }
  }
  if ( memcmp( &xData, &xData1, 2600) ) {
    Serial.print("\t:( - Content not in ERAM");
  }
  else {
    Serial.print("\t:) - Expected Data in ERAM");
  }
  Serial.print('\n');
}

#if 0
uint32_t *ptrERAM_32 = (uint32_t *)0x70400012;  // Set to ERAM
const uint32_t  sizeofERAM_32 = 0x3FFF71 / sizeof( ptrERAM_32 ); // sizeof free RAM in uint32_t units.
#else
uint8_t *ptrERAM_32 = (uint8_t *)0x70400012;  // Set to ERAM
const uint32_t  sizeofERAM_32 = 0x3FFF71 / sizeof( ptrERAM_32[0] ); // sizeof free RAM in uint32_t units.
#endif
void check24() {
  uint32_t ii;
  uint32_t jj = 0, kk = 0;
  Serial.print("\n    ERAM ========== memory map ===== array* ======== check24() : WRITE !!!!\n");
  Serial.printf("\t\tERAM length 0x%X element size of %d\n", sizeofERAM_32, sizeof( ptrERAM_32[0] ));
  my_us = 0;
  for ( ii = 0; ii < sizeofERAM_32; ii++ ) {
    ptrERAM_32[ii] = 24;
  }
  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  Serial.print("    ERAM ============================ check24() : COMPARE !!!!\n");
  my_us = 0;
  for ( ii = 0; ii < sizeofERAM_32; ii++ ) {
    if ( 24 != ptrERAM_32[ii] ) {
      if ( kk != 0 ) {
        Serial.printf( "\t+++ Good Run of %u {bad @ %u}\n", kk, ii );
        kk = 0;
      }
      if ( jj < 100 )
        Serial.printf( "%3u=%8u\n", ii, ptrERAM_32[ii] );
      jj++;
    }
    else {
      kk++;
    }
  }
  Serial.printf( "\t took %lu elapsed us\n", (uint32_t)my_us );
  if ( 0 == jj )
    Serial.printf( "Good, " );
  else
    Serial.printf( "Failed to find 24 in ERAM %d Times", jj );
  Serial.printf( "\tFound 24 in ERAM %X Times\n", sizeofERAM_32 - jj );
}
