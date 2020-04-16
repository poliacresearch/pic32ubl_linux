#pragma once
#include <string>
#include <stdio.h>

typedef struct 
{
	unsigned char RecDataLen;
	unsigned int Address;
	unsigned int MaxAddress;
	unsigned int MinAddress;
	unsigned char RecType;
	unsigned char* Data;
	unsigned char CheckSum;	
	unsigned int ExtSegAddress;
	unsigned int ExtLinAddress;
} hexrecord_t;

// Hex Manager class
class CHexManager
{
public:
    std::string HexFilePath;
	unsigned int HexTotalLines;
	unsigned int HexCurrLineNo;
	bool ResetHexFilePointer(void);
	bool LoadHexFile(void);
	unsigned short GetNextHexRecord(char *HexRec, unsigned int BuffLen);
	unsigned short ConvertAsciiToHex(void *VdAscii, void *VdHexRec);
	void VerifyFlash(unsigned int* StartAdress, unsigned int* ProgLen, unsigned short* crc);

	//Constructor
	CHexManager()
	{
		HexFilePtr = nullptr;
		HexTotalLines = 0;
		HexCurrLineNo = 0;
	}
	//Destructor
	~CHexManager()
	{
		// If hex file is open close it.
		if(HexFilePtr)
		{
			fclose(HexFilePtr);
		}
	}

private:
	FILE *HexFilePtr;
};