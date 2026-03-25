#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "../../../v9-byd/system/sensord_wt/sensors/wit_c_sdk.h"
#include "../../../v9-byd/system/sensord_wt/sensors/serial.h"
#include "../../../v9-byd/system/sensord_wt/sensors/REG.h"

static int s_fd = -1;

static void serial_write_cb(uint8_t *data, uint32_t len) {
  serial_write_data(s_fd, data, (int)len);
}

static void delay_ms_cb(uint16_t ms) {
  usleep((useconds_t)ms * 1000);
}

static void reg_update_cb(uint32_t reg, uint32_t num) {
  (void)reg;
  (void)num;
}

static int check(const char *name, int32_t ret) {
  if (ret == WIT_HAL_OK) {
    printf("[OK] %s\n", name);
    return 0;
  }
  fprintf(stderr, "[ERR] %s failed: %d\n", name, ret);
  return -1;
}

int main(int argc, char *argv[]) {
  const char *dev = "/dev/ttyUSB0";
  if (argc >= 2) {
    dev = argv[1];
  }

  printf("Opening %s ...\n", dev);
  s_fd = serial_open(dev, 115200);
  if (s_fd < 0) {
    fprintf(stderr, "Failed to open %s\n", dev);
    return 1;
  }

  if (check("WitInit", WitInit(WIT_PROTOCOL_NORMAL, 0x50)) != 0) return 2;
  if (check("WitSerialWriteRegister", WitSerialWriteRegister(serial_write_cb)) != 0) return 2;
  if (check("WitDelayMsRegister", WitDelayMsRegister(delay_ms_cb)) != 0) return 2;
  if (check("WitRegisterCallBack", WitRegisterCallBack(reg_update_cb)) != 0) return 2;

  printf("\nApplying factory-like default profile ...\n");

  // Conservative settings close to common out-of-box behavior.
  if (check("WitSetBandwidth(44Hz)", WitSetBandwidth(BANDWIDTH_44HZ)) != 0) return 3;
  usleep(50000);
  if (check("WitWriteReg(ACCFILT, 8)", WitWriteReg(ACCFILT, 8)) != 0) return 3;
  usleep(50000);
  if (check("WitWriteReg(GYROFILT, 8)", WitWriteReg(GYROFILT, 8)) != 0) return 3;
  usleep(50000);
  if (check("WitSetOutputRate(100Hz)", WitSetOutputRate(RRATE_100HZ)) != 0) return 3;
  usleep(50000);
  if (check("WitSetContent(ACC|GYRO|ANGLE)", WitSetContent(RSW_ACC | RSW_GYRO | RSW_ANGLE)) != 0) return 3;
  usleep(50000);

  if (check("WitSaveParameter", WitSaveParameter()) != 0) return 4;
  usleep(200000);

  printf("\nDone. Persisted default-like profile to module flash.\n");
  printf("  BANDWIDTH = 44Hz\n");
  printf("  ACCFILT = 8\n");
  printf("  GYROFILT = 8\n");
  printf("  RRATE = 100Hz\n");
  printf("  RSW = ACC|GYRO|ANGLE\n");

  serial_close(s_fd);
  return 0;
}
