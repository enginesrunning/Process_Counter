#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SHARED_MEMORY_NAME "MySharedMemory"
#define SEMAPHORE_NAME "MySemaphore"
#define COUNT_LIMIT 1000

int main() {
    HANDLE hMapFile, hSemaphore;
    int *sharedCounter;
    int flipResult;

    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        printf("Could not create file mapping object (%d).\n", GetLastError());
        return 1;
    }

    sharedCounter = (int *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (sharedCounter == NULL) {
        printf("Could not map view of file (%d).\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    if (*sharedCounter == 0) *sharedCounter = 1;

    hSemaphore = CreateSemaphore(NULL, 1, 1, SEMAPHORE_NAME);
    if (hSemaphore == NULL) {
        printf("Could not create semaphore (%d).\n", GetLastError());
        UnmapViewOfFile(sharedCounter);
        CloseHandle(hMapFile);
        return 1;
    }

    while (*sharedCounter < COUNT_LIMIT) {
        WaitForSingleObject(hSemaphore, INFINITE);

        int currentValue = *sharedCounter;

        flipResult = rand() % 2 + 1;

        if (flipResult == 1) {
            (*sharedCounter)++;
            printf("Process %d flipped a coin: 1, incremented counter to %d\n", GetCurrentProcessId(), *sharedCounter);
        } else if (flipResult == 2) {
            int followingNumber = currentValue + 1;
            *sharedCounter = followingNumber;
            printf("Process %d flipped a coin: 2, wrote the following number %d to memory\n", GetCurrentProcessId(), followingNumber);
        }

        ReleaseSemaphore(hSemaphore, 1, NULL);

        Sleep(rand() % 100);
    }

    UnmapViewOfFile(sharedCounter);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);

    return 0;
}
