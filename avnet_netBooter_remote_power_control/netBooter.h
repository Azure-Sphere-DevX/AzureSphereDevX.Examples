
#pragma once

void pollNetBooterCurrentData(void);
bool EnableDisableNetBooterDevice(int, bool);

#define RESPONSE_OK "$A0"
#define DEVICE_ONE_MASK 0x01
#define DEVICE_TWO_MASK 0x02