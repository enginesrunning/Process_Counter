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

    // Seed random number generator with process-specific seed
    srand((unsigned int)time(NULL) ^ GetCurrentProcessId());

    // Create or open a memory-mapped file
    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        printf("Could not create file mapping object (%d).\n", GetLastError());
        return 1;
    }

    // Map the shared memory to this process's address space
    sharedCounter = (int *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (sharedCounter == NULL) {
        printf("Could not map view of file (%d).\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // Initialize the shared counter only once (in the first process)
    if (*sharedCounter == 0) *sharedCounter = 1;

    // Create or open a named semaphore
    hSemaphore = CreateSemaphore(NULL, 1, 1, SEMAPHORE_NAME);
    if (hSemaphore == NULL) {
        printf("Could not create semaphore (%d).\n", GetLastError());
        UnmapViewOfFile(sharedCounter);
        CloseHandle(hMapFile);
        return 1;
    }

    // Collaborative counting loop
    while (*sharedCounter < COUNT_LIMIT) {
        // Wait to acquire the semaphore
        WaitForSingleObject(hSemaphore, INFINITE);

        // Read the current shared counter value
        int currentValue = *sharedCounter;

        // Flip a coin to get 1 or 2
        flipResult = rand() % 2 + 1;

        if (flipResult == 1) {
            // Increment the shared counter
            (*sharedCounter)++;
            printf("Process %d flipped a coin: 1, incremented counter to %d\n", GetCurrentProcessId(), *sharedCounter);
        } else if (flipResult == 2) {
            // If flipResult is 2, write the following number (current value + 1) to memory
            int followingNumber = currentValue + 1;
            *sharedCounter = followingNumber; // Update memory with the following number
            printf("Process %d flipped a coin: 2, wrote the following number %d to memory\n", GetCurrentProcessId(), followingNumber);
        }

        // Release the semaphore
        ReleaseSemaphore(hSemaphore, 1, NULL);

        // Simulate a delay to allow the other process to execute
        Sleep(rand() % 100);
    }

    // Clean up resources
    UnmapViewOfFile(sharedCounter);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);

    return 0;
}
