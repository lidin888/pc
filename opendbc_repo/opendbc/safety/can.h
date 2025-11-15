#pragma once

// Note: panda/board/can.h defines a global symbol named `dlc_to_len`.
// To avoid a duplicate symbol when both headers are included in the
// firmware build, use a distinct name for the opendbc safety table.
#ifdef __PANDA_BOARD_CAN_DECLARATIONS_H__
static const unsigned char opendbc_dlc_to_len[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
#else
#define opendbc_dlc_to_len dlc_to_len
#endif

#define CANPACKET_HEAD_SIZE 6U  // non-data portion of CANPacket_t

#ifdef CANFD
  #define CANPACKET_DATA_SIZE_MAX 64U
#else
  #define CANPACKET_DATA_SIZE_MAX 8U
#endif

// bump this when changing the CAN packet
#define CAN_PACKET_VERSION 4

// Only define CANPacket_t if not already defined in panda board headers
#ifndef __PANDA_BOARD_CAN_DECLARATIONS_H__
typedef struct {
  unsigned char fd : 1;
  unsigned char bus : 3;
  unsigned char data_len_code : 4;  // lookup length with opendbc_dlc_to_len
  unsigned char rejected : 1;
  unsigned char returned : 1;
  unsigned char extended : 1;
  unsigned int addr : 29;
  unsigned char checksum;
  unsigned char data[CANPACKET_DATA_SIZE_MAX];
} __attribute__((packed, aligned(4))) CANPacket_t;
#endif

// Only define GET_LEN if not already defined in panda board headers
#ifndef __PANDA_BOARD_CAN_DECLARATIONS_H__
#define GET_LEN(msg) (opendbc_dlc_to_len[(msg)->data_len_code])
#endif