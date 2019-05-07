#pragma once
#include <iostream>
#include <Windows.h>
#include <time.h>

class ComLib
{
private:
	enum MSGTYPE
	{
		NORMAL,
		DUMMY
	};

	struct FAILSAFE
	{
		HANDLE hFileMap;
		void* mData;
		bool exists = false;
		unsigned int mSize = 1 << 10;
	};
	
	FAILSAFE fileMap;
	size_t* head;
	size_t* tail;
	char* base;
	size_t buffSize;
	size_t freeMemory;
	std::string memoryName;
	int type;
	bool waitingReset = false;

public:
	struct HEADER
	{
		MSGTYPE type;
		size_t length;
	};
	enum TYPE
	{
		PRODUCER,
		CONSUMER
	};

	ComLib();
	ComLib(const std::string& secret, const size_t& buffSize, TYPE type);
	~ComLib();

	//Make the write/send a recursive function if needed
	
	bool write(const void* msg, const size_t length);
	bool read(char* msg, size_t& length);
	
	size_t nextSize();
	HEADER h;
};