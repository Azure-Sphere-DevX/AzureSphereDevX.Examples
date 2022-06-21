
#include "persistantConfig.h"

static bool write_config_to_mutable_storage(productShelf_t shelf1, 
                                            productShelf_t shelf2, 
                                            bool lowPowerEnabled, 
                                            int lowPowerSleepPeriod){
    
    int fd = Storage_OpenMutableFile();
    if(fd == -1){
        Log_Debug("Error opening mutable storage!\n");
        dx_terminate(APP_ExitCode_OpenMutableFileFailed);
        return false;
    }

    Log_Debug("Updating Mutable Storage!\n");

    // Copy all the current config data for both shelfs and low power mode into the 
    // local structure

    persistantMemory_t persistantConfig;

    // Low power Config
    persistantConfig.sleepTime = lowPowerSleepPeriod;
    persistantConfig.lowPowerModeEnabled = lowPowerEnabled;
    
    // Shelf #1 Config
    persistantConfig.shelf1Copy.productHeight_mm = shelf1.productHeight_mm;
    persistantConfig.shelf1Copy.productReserve = shelf1.productReserve;
    persistantConfig.shelf1Copy.shelfHeight_mm = shelf1.shelfHeight_mm;

    // Shelf #2 Config
    persistantConfig.shelf2Copy.productHeight_mm = shelf2.productHeight_mm;
    persistantConfig.shelf2Copy.productReserve = shelf2.productReserve;
    persistantConfig.shelf2Copy.shelfHeight_mm = shelf2.shelfHeight_mm;

    // Write the global structure to persistant memory
    int ret = write(fd, &persistantConfig, sizeof(persistantConfig));
    
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while writing to mutable file\n");
        dx_terminate(APP_ExitCode_WriteFileWriteFailed);

    } else if (ret < sizeof(persistantConfig)) {
        Log_Debug("ERROR: Only wrote %d of %d bytes requested\n", ret, (int)sizeof(persistantConfig));
        dx_terminate(APP_ExitCode_WriteFileWriteFailed);
    }
    close(fd);
    return true;
}

static bool read_config_from_mutable_storage(persistantMemory_t* persistantConfig){

    int fd = Storage_OpenMutableFile();
    if (fd == -1) {
        Log_Debug("ERROR: Could not open mutable file, using defaults until device twins update!\n");
        return false;
    }

    int ret = read(fd, persistantConfig, sizeof(*persistantConfig));
    if (ret == -1) {
        Log_Debug("ERROR: An error occurred while reading file, using defaults until device twins update!\n");
    }
    close(fd);

    if (ret < sizeof(*persistantConfig)) {
        return false;
    }

    return true;
}

bool updateConfigInMutableStorage(productShelf_t shelf1, 
                                   productShelf_t shelf2, 
                                   bool lowPowerEnabled, 
                                   int lowPowerSleepTime){

    // Determine if the configuration has changed, if so update mutable storage
    persistantMemory_t localConfigCopy;
    if(read_config_from_mutable_storage(&localConfigCopy)){

        // Start to check each configuration item, if we find one that's been updated,
        // then write the new config and exit

        // Low power mode config
        if(localConfigCopy.lowPowerModeEnabled != lowPowerEnabled){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        if(localConfigCopy.sleepTime != lowPowerSleepTime){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        // Shelf #1 config
        if(localConfigCopy.shelf1Copy.productHeight_mm != shelf1.productHeight_mm){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        if(localConfigCopy.shelf1Copy.productReserve != shelf1.productReserve){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        if(localConfigCopy.shelf1Copy.shelfHeight_mm != shelf1.shelfHeight_mm){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        // Shelf #2 config
        if(localConfigCopy.shelf2Copy.productHeight_mm != shelf2.productHeight_mm){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        if(localConfigCopy.shelf2Copy.productReserve != shelf2.productReserve){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }

        if(localConfigCopy.shelf2Copy.shelfHeight_mm != shelf2.shelfHeight_mm){
            write_config_to_mutable_storage( shelf1, shelf2, lowPowerEnabled, lowPowerSleepTime);
            return true;
        }
    }

//    Log_Debug("Config not updated, no changes detected\n");
    return false;
}

void update_config_from_mutable_storage(productShelf_t* shelf1, productShelf_t* shelf2, bool* lowPowerEnabled, int* lowPowerSleepPeriod ){

    int fd = Storage_OpenMutableFile();
    if(fd == -1){
        Log_Debug("Error opening mutable storage!\n");
        dx_terminate(APP_ExitCode_OpenMutableFileFailed);
    }

    Log_Debug("Updating Config from Mutable Storage!\n");

    // Read the persistant config
    persistantMemory_t localConfigCopy;
    if(read_config_from_mutable_storage(&localConfigCopy)){

        // Low power Config
        *lowPowerSleepPeriod = localConfigCopy.sleepTime;
        *lowPowerEnabled = localConfigCopy.lowPowerModeEnabled;
        
        // Shelf #1 Config
        shelf1->productHeight_mm = localConfigCopy.shelf1Copy.productHeight_mm;
        shelf1->productReserve = localConfigCopy.shelf1Copy.productReserve;
        shelf1->shelfHeight_mm = localConfigCopy.shelf1Copy.shelfHeight_mm;

        // Shelf #2 Config
        shelf2->productHeight_mm = localConfigCopy.shelf2Copy.productHeight_mm;
        shelf2->productReserve = localConfigCopy.shelf2Copy.productReserve;
        shelf2->shelfHeight_mm = localConfigCopy.shelf2Copy.shelfHeight_mm;

        close(fd);
    }
}

bool initPersistantMemory(productShelf_t shefl1, productShelf_t shelf2, bool lowPowerModeEnabled, int lowPowerSleepPeriod){

    persistantMemory_t localConfigCopy;

    // Read the persistant memory, if the call returns false, then we 
    // do not have any data stored and we need to write the first record
    // Determine if the configuration has changed, if so update mutable storage
    if(!read_config_from_mutable_storage(&localConfigCopy)){

        Log_Debug("Creating Persistant file!!\n");

        // Write the default data to establish the "file"
        write_config_to_mutable_storage(shefl1, shelf2, lowPowerModeEnabled, lowPowerSleepPeriod);
        return false;
    }
    else{
        Log_Debug("Persistant file exists!\n");
        return true;
    }
}