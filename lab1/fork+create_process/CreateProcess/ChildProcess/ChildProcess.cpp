// ChildProcess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdio.h"
#include "windows.h"

int main()
{
    HANDLE readHandle = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE writeHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD readedBytes, writedBytes;

    int x, y;
    ReadFile(readHandle, &x, sizeof(int), &readedBytes, NULL);
    ReadFile(readHandle, &y, sizeof(int), &readedBytes, NULL);

    int res = x + y;
    WriteFile(writeHandle, &res, sizeof(int), &writedBytes, NULL);

    return 0;
}
