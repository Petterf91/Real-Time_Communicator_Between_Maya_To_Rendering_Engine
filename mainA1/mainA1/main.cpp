#include "ComLib.h"

#define MAX_MSGSIZE 1024 * 1024

using namespace std;

int delay;
size_t memorySize;
int numMsg;
int msgSize;

void randomMsgGen(char* stringP, const int x);

int main(int argc, char** argv)
{
	srand(time(NULL));
	numMsg = atoi(argv[4]);
	int counter = 0;
	delay = atoi(argv[2]);
	memorySize = atoi(argv[3]);

	if (argv[1][0] == 'p')
	{
		char* buffer;
		buffer = (char*)malloc(1024 * 1024 * 10);

		ComLib producer("sharedMem", memorySize, ComLib::TYPE::PRODUCER);
		while (counter < numMsg)
		{
			Sleep(delay);
			if (argv[5][0] == 'r')
			{
				msgSize = rand() % MAX_MSGSIZE + 1;
			}
			else
			{
				msgSize = atoi(argv[5]);
			}

			randomMsgGen(buffer, msgSize);
			if (producer.write(buffer, msgSize))
			{
				counter++;
				cout << counter << ":\t" << buffer << endl;
			}
		}
		free(buffer);
	}
	else if (argv[1][0] == 'c')
	{
		char* buffer;
		buffer = (char*)malloc(1024 * 1024 * 10);
		ComLib consumer("sharedMem", memorySize, ComLib::TYPE::CONSUMER);
		while (counter < numMsg)
		{
			Sleep(delay);
			size_t msgLength;

			if (consumer.read(buffer, msgLength))
			{
				counter++;
				cout << counter << ":\t" << buffer << endl;
			}
		}
		free(buffer);
	}
	return 0;
}

void randomMsgGen(char* stringP, const int x)
{
	//Numbers start at ASCII 48 -> 57
	//Big letters at 65 -> 90
	//Small letter at 97 -> 122
	for (auto i = 0; i < x; i++)
	{
		int randChar = rand() % 62;

		if (randChar >= 0 && randChar < 10)
		{
			stringP[i] = char(randChar + 48);
		}
		else if (randChar >= 10 && randChar < 36)
		{
			stringP[i] = char(randChar + 55);
		}
		else if (randChar >= 36 && randChar < 62)
		{
			stringP[i] = char(randChar + 61);
		}
		else
		{
			printf("\nThere's an ERROR while creating the random message.\n");
		}
	}

	stringP[x - 1] = 0;
}