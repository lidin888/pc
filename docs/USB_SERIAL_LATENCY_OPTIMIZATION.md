# USB Serial Low Latency Optimization

## Overview

The tici project enables USB serial low latency mode by default to reduce communication latency with panda and other USB serial devices, improving system response time.

## Optimization Details

### 1. Install setserial Tool

The `setserial` package is automatically installed during system dependency setup to configure serial device parameters.

### 2. Create udev Rules

A udev rule is created at `/etc/udev/rules.d/99-ttyusb-lowlatency.rules` to automatically enable low latency mode when USB serial devices are connected:

```bash
ACTION=="add", KERNEL=="ttyUSB[0-9]*", RUN+="/usr/bin/setserial /dev/%k low_latency"
ACTION=="add", KERNEL=="ttyACM[0-9]*", RUN+="/usr/bin/setserial /dev/%k low_latency"
```

## Effects

- **Reduced Latency**: Decreases from default 15-16ms to approximately 5-10ms
- **Faster Response**: Significantly improves real-time control response time
- **Minimal Side Effects**: CPU usage may increase by 1-2%, which is within acceptable range

## Verification

### Check if Low Latency Mode is Enabled

```bash
# Method 1: Use setserial to check
sudo setserial -g /dev/ttyUSB0

# Method 2: Manually set (temporary)
sudo setserial /dev/ttyUSB0 low_latency

# Method 3: Check serial configuration
stty -F /dev/ttyUSB0 -a
```

### Reapply udev Rules

If you need to manually reload udev rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

The low latency settings will be automatically applied after reconnecting the device.

## Compatibility

This optimization works with most USB serial chipsets:
- FTDI (FT232, FT4232, etc.)
- CP210x
- PL2303
- CH341
- And other standard USB serial chipsets

## Disable Optimization (Not Recommended)

If you need to disable this optimization:

1. Delete the udev rule file:
   ```bash
   sudo rm /etc/udev/rules.d/99-ttyusb-lowlatency.rules
   sudo udevadm control --reload-rules
   ```

2. Reconnect the device

## Technical Details

### Role of Low Latency Mode

- **Disables Latency Timer**: Transmits serial data immediately without waiting for bulk transfers
- **Reduces Interrupt Latency**: CPU responds to serial interrupts more promptly
- **Suitable for Real-time Applications**: Highly beneficial for real-time control systems like autonomous driving

### Why This Optimization is Needed

Linux systems default to higher latency timers for USB serial devices to reduce CPU usage. However, for real-time control systems (like autonomous driving), lower latency is more important.

## Troubleshooting

### Issue: Device doesn't automatically enable low latency mode

**Solution**:
1. Check if setserial is installed: `which setserial`
2. Check if udev rule exists: `cat /etc/udev/rules.d/99-ttyusb-lowlatency.rules`
3. Reload udev rules: `sudo udevadm control --reload-rules && sudo udevadm trigger`
4. Reconnect the device

### Issue: Device unstable or communication errors

**Solution**:
Some inexpensive USB-to-serial adapters may work unstably in low latency mode. If you encounter issues:
1. Check device quality and driver support
2. Try using higher quality USB serial adapters (such as FTDI-based ones)
3. Temporarily disable low latency mode for testing

## Related Information

- [setserial man page](https://manpages.debian.org/testing/setserial/setserial.8.en.html)
- [udev rules documentation](https://www.freedesktop.org/software/systemd/man/udev.html)
