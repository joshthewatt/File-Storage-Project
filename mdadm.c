#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cache.h"
#include "mdadm.h"
#include "jbod.h"
#include <stdbool.h>

bool MOUNTED = false;

uint32_t bit_line(uint32_t op, uint32_t disk_ID, uint32_t block_ID){ // bits are shifted to pad them
  uint32_t op_line;
  uint32_t op_bit = op << 26;
  uint32_t disk_ID_bit = disk_ID << 28;
  disk_ID_bit = disk_ID_bit >> 6;
  uint32_t block_ID_bit = block_ID << 24;
  block_ID_bit = block_ID_bit >> 24;
  op_line = op_bit | disk_ID_bit | block_ID_bit;
  return op_line;

}

int mdadm_mount(void) {
  if (MOUNTED == true){
    return -1;
  }
  uint32_t op= JBOD_MOUNT; //number to be converted to hex op
  uint32_t mount_op = bit_line(op, 0, 0);
  uint8_t *block = NULL;
  if (jbod_operation(mount_op, (uint8_t *)block) == 0){ //0 means success
    MOUNTED = true;
    //create_cache(4);
    return 1;
  }
  return -1; //failure
}

int mdadm_unmount(void) {
  if (MOUNTED == false){
    return -1;
  }
  uint32_t op = JBOD_UNMOUNT; //op int calling JBOD_Mount
  uint32_t unmount_op = bit_line(op, 0, 0);
  uint8_t *block = NULL;
  if (jbod_operation(unmount_op, (uint8_t *)block) == 0){ //0 means success
    MOUNTED = false;
    //cache_destroy();
    return 1;
  }
  return -1; //failure
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
    int blocks_read = (len / 256) + 2; // the amount of blocks to read from (2 - 5)
    uint8_t buf_temp[1792]; // variable to read into and then copy into buf; MUST BE 1792 for room for buffer in loop


    if (!MOUNTED || (buf == NULL && len != 0 )|| (addr + len) > 1048576){ //if pointer is null and the length is not zero (test case 2)
      return -1;
    }
    if (len > 1024){ // if the address or length of read is greater than 1024 bytes (test case 3)
      return -1;
    }

    // Get disk, block, and block offset
    int disk = addr / 65536; //disk number
    int block_location = (addr % 65536) / 256; //block on the disk
    int start_byte = (addr % 65536) % 256; // byte of start

    // Set disk and block of addr
    uint32_t seek_to_disk_op = bit_line(JBOD_SEEK_TO_DISK, disk, 0);
    uint8_t *block = NULL;
    if (jbod_operation(seek_to_disk_op, (uint8_t *)block) == -1){ //Sets up the disk spot to work with
        return -1; //if fails
    }
    uint32_t seek_to_block_op  = bit_line(JBOD_SEEK_TO_BLOCK, 0, block_location);
    if (jbod_operation(seek_to_block_op, (uint8_t *)block) == -1){ //Sets up the disk spot to work with
        return -1; //if fails
    }

    // Read disk blocks into temp buffer
    uint32_t read_op;
    uint8_t *spot = &buf_temp[0]; //pointer to spot in buf_temp
    for (int k = 0; k < blocks_read; k++){
      if (block_location > 255){ //testing if need to move to next disk
        disk = disk + 1;
        seek_to_disk_op = bit_line(JBOD_SEEK_TO_DISK, disk, 0);
        jbod_operation(seek_to_disk_op, (uint8_t *)block);
        block_location = 0;
      }
      // Check if in cache
      if (cache_lookup(disk, block_location, spot) == -1){
        read_op = bit_line(JBOD_READ_BLOCK, 0, 0);
        jbod_operation(read_op, spot); // reads block
        cache_insert(disk, block_location, spot);
      }

      spot += 256; //move the pointer of buf_temp
      block_location = block_location + 1; //next block

    }

    // Copy into buffer desired bytes
    for (int k = 0; k < len; k++){ //loop through each byte that we want
      buf[k] = buf_temp[k + start_byte]; //starts at the byte in the block
    }

  return len;
}

int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  int blocks_write = (len / 256) + 2; // times to loop through
  if (MOUNTED == false || len > 1024 || (buf == NULL && len != 0 ) || (addr + len) > 1048576){ //conditions
    return -1;
  }

  // Get disk, block, and block offset
  int disk = addr / 65536; //disk number start
  int block_location = (addr % 65536) / 256; //block on the disk start
  int start_byte = (addr % 65536) % 256; // byte of start

  // Set disk and block of addr
  uint32_t seek_to_disk_op = bit_line(JBOD_SEEK_TO_DISK, disk, 0);
  uint8_t *block = NULL;
  if (jbod_operation(seek_to_disk_op, (uint8_t *)block) == -1){ //Sets up the disk spot to work with
      return -1; //if fails
  }
  uint32_t seek_to_block_op  = bit_line(JBOD_SEEK_TO_BLOCK, 0, block_location);
  if (jbod_operation(seek_to_block_op, (uint8_t *)block) == -1){ //Sets up the block spot to work with
      return -1; //if fails
  }

  // Read existing blocks into read_buffer
  uint8_t read_buffer[blocks_write * 256];//buffer with full block contents
  int disk_temp1 = disk; //temp disk location
  int block_location_temp1 = block_location; //temp block location
  uint32_t read_op;
  uint8_t *spot = &read_buffer[0]; //pointer to spot in buf_temp

  for (int k = 0; k < blocks_write; k++){
    if (block_location_temp1 > 255){ //testing if need to move to next disk
      disk_temp1 = disk_temp1 + 1;
      seek_to_disk_op = bit_line(JBOD_SEEK_TO_DISK, disk_temp1, 0);
      jbod_operation(seek_to_disk_op, (uint8_t *)block);
      block_location_temp1 = 0;
    }
    if (cache_lookup(disk, block_location, spot) == -1){
      read_op = bit_line(JBOD_READ_BLOCK, 0, 0);
      jbod_operation(read_op, spot); // reads block
      cache_insert(disk, block_location, spot);
    }

    spot += 256; //move the pointer of buf_temp
    block_location_temp1 = block_location_temp1 + 1; //next block
  }


  // Return starting place back to addr
  seek_to_disk_op = bit_line(JBOD_SEEK_TO_DISK, disk, 0);
  if (jbod_operation(seek_to_disk_op, (uint8_t *)block) == -1){ //Sets up the disk spot to work with
      return -1; //if fails
  }
  seek_to_block_op  = bit_line(JBOD_SEEK_TO_BLOCK, 0, block_location);
  if (jbod_operation(seek_to_block_op, (uint8_t *)block) == -1){ //Sets up the block spot to work with
      return -1; //if fails
  }
  // Creating full block buffer WITH new buf written in
  int end_section; //the remaining area of the read
  int write_block_bytes = blocks_write * 256;
  end_section = write_block_bytes - (len + start_byte);
  uint8_t buf_temp[blocks_write * 256]; // with the start_byte inserted

  for (int k = 0; k < start_byte; k++){//fill in start of the buffer with existing
    buf_temp[k] = read_buffer[k];
  }
  for (int k = 0; k < len; k++) {//fill the buffer with what we want to write in
    buf_temp[k + start_byte] = buf[k];
  }
  for (int k = 0; k < end_section; k++){ //fill remaining space of buffer with existing
    buf_temp[start_byte + len + k] = read_buffer[start_byte + len + k];
  }

  // Write the whole buffer into read blocks
  uint32_t write_op; // op variable
  uint8_t *spot_1 = &buf_temp[0];

  for (int k = 0; k < blocks_write; k++){ // loop through blocks to write in
    if (block_location > 255){ //testing if need to move to next disk
      disk = disk + 1;
      seek_to_disk_op = bit_line(JBOD_SEEK_TO_DISK, disk, 0);
      jbod_operation(seek_to_disk_op, (uint8_t *)block);
      block_location = 0;
    }
    write_op = bit_line(JBOD_WRITE_BLOCK, 0, 0); // write operation
    jbod_operation(write_op, spot_1);
    spot_1 += 256; //moves up buffer 1 block
    block_location = block_location + 1; // moves block up a location
  }

  return len;
}
