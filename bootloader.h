#pragma once
#include "hidapi.h"
#include "hexmanager.h"

typedef enum
{
    READ_BOOT_INFO = 1,
    ERASE_FLASH,
    PROGRAM_FLASH,
    READ_CRC,
    JUMP_TO_APP
} commands_t;

class Bootloader
{
public:
    static unsigned short CalculateCrc(char *data, unsigned int len);
    Bootloader();
    ~Bootloader();
    bool Initialize(std::string &pathToHexFile);
    bool GetInfo();
    bool Erase();
    bool Program();
    bool Verify();
    void JumpToApp();
private:
    bool SendCommand(commands_t command);
    void HandleResponse();
    void HandleNoResponse(void);
    void ReceiveTask(void);
    void BuildRxFrame(unsigned char *buff, unsigned short buffLen);
    unsigned short CalculateFlashCRC(void);
    hid_device *handle;
    unsigned char TxPacket[1000];
    unsigned short TxPacketLen;
    char Buff[1000];
    bool ResetHexFilePtr;
    const int MAX_STR_LEN = 255;
    const int MANUFACTURER_ID = 0x4d8;
    const int PRODUCT_ID = 0x3c;
    CHexManager HexManager;
    char RxData[255];
    unsigned short RxDataLen;
    bool RxFrameValid;
    commands_t LastSentCommand;
};