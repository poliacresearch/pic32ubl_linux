#include "bootloader.h"
#include <iostream>
#include <string>

#define SOH 01
#define EOT 04
#define DLE 16

Bootloader::Bootloader() : TxPacket{}, Buff{}, RxData{}
{
    RxFrameValid = false;
    RxDataLen = 0;
    ResetHexFilePtr = true;
	handle = nullptr;
	TxPacketLen = 0;
	LastSentCommand = READ_BOOT_INFO;
}

Bootloader::~Bootloader()
{

}

bool Bootloader::Initialize(std::string &pathToHexFile)
{
    wchar_t wstr[MAX_STR_LEN];

    // Open hex file specified by user in arguments.
    HexManager.HexFilePath = pathToHexFile;
    if (!HexManager.LoadHexFile())
    {
        std::cout << "Error loading hex file" << std::endl;
        return false;
    }
	else
	{
		std::cout << "Hex file loaded successfully" << std::endl;
	}

    // Initialize transport layer.
    if (hid_init() < 0)
    {
        std::cout << "Error initializing transport layer" << std::endl;
        return false;
    }

    // Open USB HID bootloader firmware device.
    handle = hid_open(MANUFACTURER_ID, PRODUCT_ID, NULL);
    if (!handle)
    {
        std::cout << "Error opening USB HID bootloader device. Make sure device is connected. Make sure permissions are set correctly." << std::endl;
        return false;
    }

	// Enable blocking I/O.
    if (hid_set_nonblocking(handle, 0) < 0)
	{
		std::cout << "Error disabling nonblocking" << std::endl;
		return false;
	}

    // Check manufacturer & product strings.
    if (hid_get_manufacturer_string(handle, wstr, MAX_STR_LEN) < 0)
    {
        std::cout << "Error getting manufacturer string" << std::endl;
        return false;
    }

	std::wstring manStr(wstr);
    std::cout << "Manufacturer string: " << std::string(manStr.begin(), manStr.end()) << std::endl;

    if (hid_get_product_string(handle, wstr, MAX_STR_LEN) < 0)
    {
        std::cout << "Error getting product string" << std::endl;
        return false;
    }

	std::wstring prodStr(wstr);
    std::cout << "Product string: " << std::string(prodStr.begin(), prodStr.end()) << std::endl;

    return true;
}

bool Bootloader::Erase()
{
	if (!SendCommand(ERASE_FLASH))
	{
		return false;
	}

	ReceiveTask();

	return true;
}

bool Bootloader::Program()
{
	while (SendCommand(PROGRAM_FLASH))
	{
		ReceiveTask();
	}

	std::cout << "Programming complete" << std::endl;
	return true;
}

bool Bootloader::Verify()
{
	if (!SendCommand(READ_CRC))
	{
		return false;
	}

	ReceiveTask();

	return true;
}

void Bootloader::JumpToApp()
{
	SendCommand(JUMP_TO_APP);
}

bool Bootloader::GetInfo()
{
    if (!SendCommand(READ_BOOT_INFO))
    {
        return false;
    }

    ReceiveTask();

    return true;
}

/****************************************************************************
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const unsigned short crc_table[16] = 
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
};

/****************************************************************************
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param len		Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
unsigned short Bootloader::CalculateCrc(char *data, unsigned int len)
{
    unsigned short crc = 0;
    
    while(len--)
    {
        unsigned i = (crc >> 12) ^ (*data >> 4);
	    crc = crc_table[i & 0x0F] ^ (crc << 4);
	    i = (crc >> 12) ^ (*data >> 0);
	    crc = crc_table[i & 0x0F] ^ (crc << 4);
	    data++;
	} 

    return (crc & 0xFFFF);
}

/****************************************************************************
 *  Send Command
 *
 * \param		cmd:  Command  
 * \return         
 *****************************************************************************/
bool Bootloader::SendCommand(commands_t command)
{
    unsigned short crc;
	unsigned int StartAddress,  Len;
	unsigned short BuffLen = 0;
	unsigned short HexRecLen;
	unsigned int totalRecords = 10;
	TxPacketLen = 0;

    // Store for later use.
    LastSentCommand = command;

	switch(command)
    {
	    case READ_BOOT_INFO:
	    	Buff[BuffLen++] = command;
	    	break;

	    case ERASE_FLASH:
	    	Buff[BuffLen++] = command;
	    	break;

	    case JUMP_TO_APP:
	    	Buff[BuffLen++] = command;
	    	break;
    
	    case PROGRAM_FLASH:
	    	Buff[BuffLen++] = command;
	    	if(ResetHexFilePtr)
	    	{
	    		if(!HexManager.ResetHexFilePointer())
	    		{
	    			// Error resetting the file pointer.
	    			return false;
	    		}
	    	}
	    	HexRecLen = HexManager.GetNextHexRecord(&Buff[BuffLen], (sizeof(Buff) - 5));
	    	if(HexRecLen == 0)
	    	{
	    		// Not a valid hex file.
	    		return false;
	    	}
    
	    	BuffLen = BuffLen + HexRecLen;
	    	while(totalRecords)
	    	{
	    		HexRecLen = HexManager.GetNextHexRecord(&Buff[BuffLen], (sizeof(Buff) - 5));
	    		BuffLen = BuffLen + HexRecLen;
	    		totalRecords--;
	    	}
	    	break;

	    case READ_CRC:
	    	Buff[BuffLen++] = command;
	    	HexManager.VerifyFlash((unsigned int*)&StartAddress, (unsigned int*)&Len, (unsigned short*)&crc);
	    	Buff[BuffLen++] = (StartAddress);
	    	Buff[BuffLen++] = (StartAddress >> 8);
	    	Buff[BuffLen++] = (StartAddress >> 16);
	    	Buff[BuffLen++] = (StartAddress >> 24);
	    	Buff[BuffLen++] = (Len);
	    	Buff[BuffLen++] = (Len >> 8);
	    	Buff[BuffLen++] = (Len >> 16);
	    	Buff[BuffLen++] = (Len >> 24);
	    	Buff[BuffLen++] =  (char)crc;
	    	Buff[BuffLen++] =  (char)(crc >> 8);
	    	break;

	    default:
	    	return false;
	    	break;
    }

    // Calculate CRC for the frame.
    crc = CalculateCrc(Buff, BuffLen);
    Buff[BuffLen++] = (char)crc;
    Buff[BuffLen++] = (char)(crc >> 8);

    // SOH: Start of header
    TxPacket[TxPacketLen++] = SOH;

    // Form TxPacket. Insert DLE in the data field whereever SOH and EOT are present.
    for(int i = 0; i < BuffLen; i++)
    {
        if((Buff[i] == EOT) || (Buff[i] == SOH)
                || (Buff[i] == DLE))
        {
            TxPacket[TxPacketLen++] = DLE;			
        }
        TxPacket[TxPacketLen++] = Buff[i];
    }

    // EOT: End of transmission
    TxPacket[TxPacketLen++] = EOT;

    if (hid_write(handle, TxPacket, TxPacketLen) < 0)
    {
        std::cout << "Error writing to HID device" << std::endl;
        return false;
    }

    return true;
}

/****************************************************************************
 *  Gets locally calculated CRC
 *
 * \param		
 * \param		
 * \param 		
 * \return 16 bit CRC      
 *****************************************************************************/
unsigned short Bootloader::CalculateFlashCRC(void)
{
	unsigned int StartAddress,  Len;
	unsigned short crc;
	HexManager.VerifyFlash((unsigned int*)&StartAddress, (unsigned int*)&Len, (unsigned short*)&crc);
	return crc;
}

void Bootloader::HandleResponse()
{
	unsigned char cmd = RxData[0];

	switch(cmd)
	{
	    case READ_BOOT_INFO:
		{
			unsigned char majorVer = RxData[1];
			unsigned char minorVer = RxData[2];
            std::cout << "Bootloader version: " << static_cast<unsigned>(majorVer) << "." << static_cast<unsigned>(minorVer) << std::endl;
			break;
		}
	    case ERASE_FLASH:
		{
			std::cout << "Flash erased" << std::endl;
			break;
		}
	    case READ_CRC:
		{
			unsigned short crc = ((RxData[2] << 8) & 0xFF00) | (RxData[1] & 0x00FF);
			if (crc == CalculateFlashCRC())
			{
				std::cout << "Verification successful" << std::endl;
			}
			else
			{
				std::cout << "Verification failed" << std::endl;
			}
			
			break;
		}

	    case PROGRAM_FLASH:
		{
	    	ResetHexFilePtr = false; 	// No need to reset hex file pointer since programming sequence has begun.
	    	break;
		}
	}
}

void Bootloader::HandleNoResponse(void)
{
	// Handle no response situation depending on the last sent command.
	switch(LastSentCommand)
	{
		case READ_BOOT_INFO:
		case ERASE_FLASH:
		case PROGRAM_FLASH:
		case READ_CRC:
        case JUMP_TO_APP:
            std::cout << "No response after sending command " << LastSentCommand << std::endl;
			break;
	}
}

/****************************************************************************
 *  Builds the response frame
 *
 * \param  buff: Pointer to the data buffer 
 * \param  buffLen: Buffer length    		
 * \return         
 *****************************************************************************/
void Bootloader::BuildRxFrame(unsigned char *buff, unsigned short buffLen)
{
	static bool Escape = false;
	unsigned short crc;

	while((buffLen > 0) && (RxFrameValid == false))
	{
		buffLen--;

		if(RxDataLen >= (sizeof(RxData)-2))
		{
			RxDataLen = 0;			
		}

		switch(*buff)
		{
			case SOH:
				if(Escape)
				{
					RxData[RxDataLen++] = *buff;	// Received byte is not SOH, but data.
					Escape = false;					// Reset Escape Flag.
				}
				else
				{
					RxDataLen = 0;
				}		
				break;
				
			case EOT:
				if(Escape)
				{
					RxData[RxDataLen++] = *buff;	// Received byte is not EOT, but data.
					Escape = false;					// Reset Escape Flag.
				}
				else
				{
					// Received byte is a EOT which indicates end of frame.
					// Calculate CRC to check the validity of the frame.
					if(RxDataLen > 1)
					{
						crc = (RxData[RxDataLen-2]) & 0x00ff;
						crc = crc | ((RxData[RxDataLen-1] << 8) & 0xFF00);
						if((CalculateCrc(RxData, (RxDataLen-2)) == crc))
						{
							if (RxDataLen > 2)
							{
								RxFrameValid = true;	
							}
						}
						else
						{
							std::cout << "CRC of received frame bad: " << crc << std::endl;
						}
					}
				}							
				break;
				
		    case DLE:
				if(Escape)
				{
					RxData[RxDataLen++] = *buff;	// Received byte is not ESC but data.
					Escape = false;					// Reset Escape Flag.			
				}
				else
				{
					// Received byte is an escape character. Set Escape flag to escape next byte.
					Escape = true;
				}	
				break;
			
			default: // Data field.
			    RxData[RxDataLen++] = *buff;
			    Escape = false;		// Reset Escape Flag.
				break;	
		}

		buff++;	
	}	
}

void Bootloader::ReceiveTask()
{
	unsigned short BuffLen;
	unsigned char Buff[255] { };
	
    BuffLen = hid_read(handle, Buff, sizeof(Buff));
	BuildRxFrame(Buff, BuffLen);
	if(RxFrameValid)
	{
		RxFrameValid = false;
		HandleResponse();
	}
	else
	{
		// There is no reponse from the device.
		HandleNoResponse();
	}
}