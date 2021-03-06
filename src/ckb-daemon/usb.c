#include "device.h"
#include "devnode.h"
#include "input.h"
#include "led.h"
#include "notify.h"
#include "usb.h"

// OS-specific USB reset
extern int os_resetusb(usbdevice* kb);

int usbqueue(usbdevice* kb, uchar* messages, int count){
    // Don't add messages unless the queue has enough room for all of them
    if(!kb->handle || kb->queuecount + count > QUEUE_LEN)
        return -1;
    for(int i = 0; i < count; i++)
        memcpy(kb->queue[kb->queuecount + i], messages + MSG_SIZE * i, MSG_SIZE);
    kb->queuecount += count;
    return 0;
}

int setupusb(usbdevice* kb){
    pthread_mutex_init(&kb->mutex, 0);
    pthread_mutex_init(&kb->keymutex, 0);
    pthread_mutex_lock(&kb->mutex);

    // Make /dev path
    if(makedevpath(kb)){
        pthread_mutex_unlock(&kb->mutex);
        pthread_mutex_destroy(&kb->mutex);
        pthread_mutex_destroy(&kb->keymutex);
        return -1;
    }

    // Set up an input device for key events
    if(!inputopen(kb)){
        rmdevpath(kb);
        pthread_mutex_unlock(&kb->mutex);
        pthread_mutex_destroy(&kb->mutex);
        pthread_mutex_destroy(&kb->keymutex);
        return -1;
    }
    updateindicators(kb, 1);

    // Create the USB queue
    for(int q = 0; q < QUEUE_LEN; q++)
        kb->queue[q] = malloc(MSG_SIZE);

    // Put the M-keys (K95) as well as the Brightness/Lock keys into software-controlled mode.
    // This packet disables their hardware-based functions.
    uchar msg[MSG_SIZE] = { 0x07, 0x04, 0x02 };
    usbqueue(kb, msg, 1);

    // Set all keys to use the Corsair input. HID input is unused.
    setinput(kb, IN_CORSAIR);

    // Restore profile (if any)
    usbprofile* store = findstore(kb->profile.serial);
    if(store){
        memcpy(&kb->profile, store, sizeof(*store));
        if(hwloadprofile(kb, 0))
            return -2;
    } else {
        // If there is no profile, load it from the device
        kb->profile.keymap = keymap_system;
        kb->profile.currentmode = getusbmode(0, &kb->profile, keymap_system);
        getusbmode(1, &kb->profile, keymap_system);
        getusbmode(2, &kb->profile, keymap_system);
        if(hwloadprofile(kb, 1))
            return -2;
    }
    updatergb(kb);
    return 0;
}

int resetusb(usbdevice* kb){
    // Perform a USB reset
    int res = os_resetusb(kb);
    if(res)
        return res;
    // Empty the queue. Re-send the software key message as well as the input mode.
    kb->queuecount = 0;
    uchar msg[MSG_SIZE] = { 0x07, 0x04, 0x02 };
    usbqueue(kb, msg, 1);
    setinput(kb, IN_CORSAIR);
    updatergb(kb);
    // If the hardware profile hasn't been loaded yet, load it here
    res = 0;
    if(!kb->profile.hw){
        if(findstore(kb->profile.serial))
            res = hwloadprofile(kb, 0);
        else
            res = hwloadprofile(kb, 1);
    }
    updatergb(kb);
    return res;
}

int closeusb(usbdevice* kb){
    // Close file handles
    if(!kb->infifo)
        return 0;
    pthread_mutex_lock(&kb->keymutex);
    if(kb->handle){
        printf("Disconnecting %s (S/N: %s)\n", kb->name, kb->profile.serial);
        inputclose(kb);
        // Delete USB queue
        for(int i = 0; i < QUEUE_LEN; i++)
            free(kb->queue[i]);
        // Move the profile data into the device store
        usbprofile* store = addstore(kb->profile.serial, 0);
        memcpy(store, &kb->profile, sizeof(kb->profile));
        // Close USB device
        closehandle(kb);
        updateconnected();
        notifyconnect(kb, 0);
    }
    // Delete the control path
    rmdevpath(kb);

    pthread_mutex_unlock(&kb->keymutex);
    pthread_mutex_destroy(&kb->keymutex);
    pthread_mutex_unlock(&kb->mutex);
    pthread_mutex_destroy(&kb->mutex);
    memset(kb, 0, sizeof(*kb));
    return 0;
}
