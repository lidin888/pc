/*
 * jy62_flash_config.c
 *
 * 一次性写入 JY62 Flash 配置工具：
 *   - 关闭板内固件 IIR 滤波 (ACCFILT=0, GYROFILT=0)
 *   - 设置硬件 LPF 带宽为 256Hz（接近直通）
 *   - 设置输出速率为 100Hz
 *   - 保存到 Flash（断电不丢）
 *
 * 编译：
 *   gcc -o jy62_flash_config jy62_flash_config.c \
 *       ../../../v9-byd/system/sensord_wt/sensors/wit_c_sdk.c \
 *       ../../../v9-byd/system/sensord_wt/sensors/serial.c
 *       (根据实际路径调整)
 *
 * 使用：
 *   sudo ./jy62_flash_config [/dev/ttyUSB0]
 *
 * 运行一次即可，设置永久保存在 JY62 Flash 中。
 * 之后 sensord_jy62 读到的就是未被板内额外滤波的原始数据。
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* 直接包含 v9-byd 的 WIT SDK */
#include "../../../v9-byd/system/sensord_wt/sensors/wit_c_sdk.h"
#include "../../../v9-byd/system/sensord_wt/sensors/serial.h"
#include "../../../v9-byd/system/sensord_wt/sensors/REG.h"

static int s_fd = -1;
static volatile uint32_t s_last_cb_reg = 0;
static volatile uint32_t s_last_cb_num = 0;
static volatile uint32_t s_acc_updates = 0;
static volatile uint32_t s_gyro_updates = 0;

static void serial_write_cb(uint8_t *data, uint32_t len)
{
    serial_write_data(s_fd, data, (int)len);
}

static void delay_ms_cb(uint16_t ms)
{
    usleep((useconds_t)ms * 1000);
}

static void sensor_callback(uint32_t reg, uint32_t num)
{
    s_last_cb_reg = reg;
    s_last_cb_num = num;

        /* 回调中统计 ACC/GYRO 更新次数，用于输出率估算 */
        if (num > 0U) {
            uint32_t end = reg + num;
            if ((AX < end) && ((AX + 3U) > reg)) {
                s_acc_updates++;
            }
            if ((GX < end) && ((GX + 3U) > reg)) {
                s_gyro_updates++;
            }
        }
}

static void check(const char *name, int32_t ret)
{
    if (ret == WIT_HAL_OK) {
        printf("[OK] %s\n", name);
    } else {
        fprintf(stderr, "[ERR] %s failed: %d\n", name, ret);
    }
}

static uint64_t now_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

static bool reg_updated(uint32_t reg)
{
        uint32_t start = s_last_cb_reg;
        uint32_t num = s_last_cb_num;
        return (num > 0U) && (reg >= start) && (reg < (start + num));
}

static bool query_reg(uint32_t reg, uint16_t *out)
{
        uint8_t ch = 0;
        uint64_t start_ms = 0;

        s_last_cb_reg = 0;
        s_last_cb_num = 0;

        if (WitReadReg(reg, 1) != WIT_HAL_OK) {
            return false;
        }

        start_ms = now_ms();
        while ((now_ms() - start_ms) < 1000ULL) {
            int n = serial_read_data(s_fd, &ch, 1);
            if (n > 0) {
                WitSerialDataIn(ch);
                if (reg_updated(reg)) {
                    *out = (uint16_t)sReg[reg];
                    return true;
                }
            } else {
                usleep(2000);
            }
        }

        return false;
}

static void verify_current_config(void)
{
        uint16_t v_bw = 0, v_acc = 0, v_gyro = 0, v_rate = 0, v_rsw = 0;
        bool ok_bw = query_reg(BANDWIDTH, &v_bw);
        bool ok_acc = query_reg(ACCFILT, &v_acc);
        bool ok_gyro = query_reg(GYROFILT, &v_gyro);
        bool ok_rate = query_reg(RRATE, &v_rate);
        bool ok_rsw = query_reg(RSW, &v_rsw);

        printf("\n--- 读回校验(重启后有效性) ---\n");
        if (ok_bw)   printf("BANDWIDTH(0x1F) = %u %s\n", v_bw,   (v_bw == BANDWIDTH_256HZ) ? "[OK]" : "[MISMATCH]");
        else         printf("BANDWIDTH(0x1F) = <read failed> [ERR]\n");

        if (ok_acc)  printf("ACCFILT(0x2A)   = %u %s\n", v_acc,  (v_acc == 0) ? "[OK]" : "[MISMATCH]");
        else         printf("ACCFILT(0x2A)   = <read failed> [ERR]\n");

        if (ok_gyro) printf("GYROFILT(0x2B)  = %u %s\n", v_gyro, (v_gyro == 0) ? "[OK]" : "[MISMATCH]");
        else         printf("GYROFILT(0x2B)  = <read failed> [ERR]\n");

        if (ok_rate) printf("RRATE(0x03)     = %u %s\n", v_rate, (v_rate == RRATE_100HZ) ? "[OK]" : "[MISMATCH]");
        else         printf("RRATE(0x03)     = <read failed> [ERR]\n");

        if (ok_rsw)  printf("RSW(0x02)       = 0x%X %s\n", v_rsw, ((v_rsw & (RSW_ACC | RSW_GYRO)) == (RSW_ACC | RSW_GYRO)) ? "[OK]" : "[MISMATCH]");
        else         printf("RSW(0x02)       = <read failed> [ERR]\n");

        if (!(ok_bw || ok_acc || ok_gyro || ok_rate || ok_rsw)) {
            uint8_t ch = 0;
            uint64_t t0 = now_ms();
            s_acc_updates = 0;
            s_gyro_updates = 0;

            while ((now_ms() - t0) < 2000ULL) {
                int n = serial_read_data(s_fd, &ch, 1);
                if (n > 0) {
                    WitSerialDataIn(ch);
                } else {
                    usleep(1000);
                }
            }

            printf("\n--- 兜底校验(2秒流式统计) ---\n");
            printf("ACC updates: %u (约 %.1f Hz)\n", s_acc_updates, s_acc_updates / 2.0);
            printf("GYRO updates: %u (约 %.1f Hz)\n", s_gyro_updates, s_gyro_updates / 2.0);
            if ((s_acc_updates >= 160U && s_acc_updates <= 240U) && (s_gyro_updates >= 160U && s_gyro_updates <= 240U)) {
                printf("输出率接近 100Hz，重启后配置大概率已保留 [LIKELY_OK]\n");
            } else {
                printf("输出率明显偏离 100Hz，建议重新写入并断电重启再测 [CHECK_NEEDED]\n");
            }
        }
}

int main(int argc, char *argv[])
{
    const char *dev = "/dev/ttyUSB0";
        bool verify_only = false;
        if (argc >= 2) {
            if (strcmp(argv[1], "--verify-only") == 0) {
                verify_only = true;
            } else {
                dev = argv[1];
            }
        }
        if (argc >= 3 && strcmp(argv[2], "--verify-only") == 0) {
            verify_only = true;
        }

    printf("Opening %s ...\n", dev);
    s_fd = serial_open(dev, 115200);
    if (s_fd < 0) {
        fprintf(stderr, "Failed to open %s\n", dev);
        return 1;
    }

    /* 初始化 SDK（NORMAL 协议，地址 0x50） */
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitSerialWriteRegister(serial_write_cb);
    WitDelayMsRegister(delay_ms_cb);
    WitRegisterCallBack(sensor_callback);

        if (verify_only) {
            verify_current_config();
            serial_close(s_fd);
            return 0;
        }

        printf("\n--- 写入 JY62 Flash 配置 ---\n");

    /* 1. 硬件 LPF 带宽 256Hz（寄存器 0x1F = 0） */
    check("WitSetBandwidth(256Hz)", WitSetBandwidth(BANDWIDTH_256HZ));
    usleep(50000);

    /* 2. 关闭 ACC 固件 IIR（寄存器 0x2A = 0） */
    check("WitWriteReg(ACCFILT, 0)", WitWriteReg(ACCFILT, 0));
    usleep(50000);

    /* 3. 关闭 GYRO 固件 IIR（寄存器 0x2B = 0） */
    check("WitWriteReg(GYROFILT, 0)", WitWriteReg(GYROFILT, 0));
    usleep(50000);

    /* 4. 输出速率 100Hz（寄存器 0x03 = 0x09） */
    check("WitSetOutputRate(100Hz)", WitSetOutputRate(RRATE_100HZ));
    usleep(50000);

    /* 5. 输出内容：仅 ACC + GYRO，不输出角度（减少串口占用） */
    check("WitSetContent(ACC|GYRO)", WitSetContent(RSW_ACC | RSW_GYRO));
    usleep(50000);

    /* 6. 保存到 Flash */
    check("WitSaveParameter", WitSaveParameter());
    usleep(200000);   /* 等待 Flash 写入完成（至少 100ms） */

    printf("\n配置完成！JY62 已永久写入：\n");
    printf("  BANDWIDTH  = 256Hz  (寄存器 0x1F = 0)\n");
    printf("  ACCFILT    = 0      (寄存器 0x2A = 0, 关闭固件 ACC IIR)\n");
    printf("  GYROFILT   = 0      (寄存器 0x2B = 0, 关闭固件 GYRO IIR)\n");
    printf("  RRATE      = 100Hz  (寄存器 0x03 = 9)\n");
    printf("  RSW        = ACC|GYRO (寄存器 0x02)\n");
    printf("\n现在可以正常运行 sensord_jy62，不需要再运行此工具。\n");

    verify_current_config();

    serial_close(s_fd);
    return 0;
}
