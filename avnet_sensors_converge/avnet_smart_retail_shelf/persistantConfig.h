#pragma once

#include <applibs/log.h>
#include <applibs/storage.h>
#include "app_exit_codes.h"
#include "dx_terminate.h"

#define MIN_SHELF_HEIGHT_MM 100
#define MAX_SHELF_HEIGHT_MM 200

#define MIN_PRODUCT_HEIGHT_MM 20
#define MAX_PRODUCT_HEIGHT_MM 50

#define MIN_PRODUCT_RESERVE 1
#define MAX_PRODUCT_RESERVE 10

#define MIN_SLEEP_PERIOD 60*5       // 5 Minutes
#define MAX_SLEEP_PERIOD (60*60*12) // 12 Hours

typedef struct
{
    int shelfHeight_mm;
    int productHeight_mm;
    int productReserve;
    int currentProductCount;
    int lastProductCount;
    bool stockLevelAlertSent;
    const char* name;
    const char* alertName;
} productShelf_t;

typedef struct
{
    productShelf_t shelf1Copy;
    productShelf_t shelf2Copy;
    bool lowPowerModeEnabled;
    int sleepTime;
} persistantMemory_t;

bool updateConfigInMutableStorage(productShelf_t shelf1, 
                                  productShelf_t shelf2, 
                                  bool lowPowerEnabled, 
                                  int lowPowerSleepPeriod);

void update_config_from_mutable_storage(productShelf_t* shelf1, 
                                        productShelf_t* shelf2, 
                                        bool* lowPowerEnabled, 
                                        int* lowPowerSleepPeriod);

bool initPersistantMemory(productShelf_t shefl1, productShelf_t shelf2, bool lowPowerModeEnabled, int lowPowerSleepPeriod);