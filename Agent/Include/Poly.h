//
// Created by 0xtriboulet on 3/4/2023.
//

#ifndef REVENANT_POLY_H
#define REVENANT_POLY_H

#include <Config.h>

#if CONFIG_POLY
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include <time.h>

// Some Functionality based on C++ code from GuidedHacking


// Defining the seed based on compile time in seconds since 00:00:00
#define SEED (((__TIME__[7] - '0') * 1 + (__TIME__[6] - '0') * 10 \
                        + (__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600 \
                        + (__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000) % 420 * 69)

/// $$$ is the polymorphism macro
// $$$:
// push the flag register onto the stack
// push registers RCX, RDX, R8, R9 onto the stack
// set EAX to 0 (two times)
// set EBX to 0
// set EAX to 0 (two times)
// pop registers R9, R8, RDX, RCX from the stack in reverse order
// pop the flag register from the stack

#define $$$ __asm__ (   \
    "pushfq\n"          \
    "push rcx\n"        \
    "push rdx\n"        \
    "push r8\n"         \
    "push r9\n"         \
    "xor eax, eax\n"    \
    "xor eax, eax\n"    \
    "xor ebx, ebx\n"    \
    "xor eax, eax\n"    \
    "xor eax, eax\n"    \
    "pop r9\n"          \
    "pop r8\n"          \
    "pop rdx\n"         \
    "pop rcx\n"         \
    "popfq\n"           \
);

// A sequence of bytes to search for in memory
#define MARKER_BYTES "\x9C\x51\x52\x41\x50\x41\x51\x31\xC0\x31\xC0\x31\xDB\x31\xC0\x31\xC0\x41\x59\x41\x58\x5A\x59\x9D"
// The length of the marker in bytes
#define MARKER_SIZE 24
// A string of characters representing which bytes in the marker to search for ("x" means search, any other character means ignore)
#define MARKER_MASK "xxxxxxxxxxxxxxxxxxxxxxxx"

// assembler opcode defines for inline asm
#define ASM_OPCODE_JMP_REL        0xEB
#define ASM_OPCODE_NOP            0x90

// size of the full assembler instruction in bytes
#define ASM_INSTR_SIZE_JMP_REL    0x2
#define ASM_INSTR_SIZE_NOP        0x1

VOID morphModule();
VOID morphMemory(PBYTE pbyDst, BYTE byLength);
PVOID rev_memcpy (PBYTE dest, PBYTE src, size_t n);
PBYTE findPattern(PBYTE pData, SIZE_T uDataSize, PBYTE pPattern, PCHAR pszMask, SIZE_T uPatternSize);


VOID morphModule()
{
    // Declare the MODULEINFO struct to store module information.
    MODULEINFO modInfo;

    // Obtain the current process handle.
    HANDLE hProcess = GetCurrentProcess();

    // Get a handle to the base module of the current process.
    HMODULE hModule = GetModuleHandleA(NULL);

    // If the module information is obtained successfully, enter the loop.
    if (GetModuleInformation(hProcess, hModule, &modInfo, sizeof(MODULEINFO)))
    {
        // Check if module size is less than MAXDWORD.
        if (modInfo.SizeOfImage < MAXDWORD)
        {
            // Declare the byte pointer to the last matching pattern and the match offset.
            byte* pbyLastMatch;
            DWORD dwMatchOffset = 0;

            // Set the morphing status to not finished.
            BOOL bMorphingFinished = FALSE;

            // Declare a counter for the number of memory regions that have been morphed.
            DWORD dwRegionCount = 0;

            // Iterate through memory regions of the current process's module to search for the marker pattern.
            while (!bMorphingFinished)
            {
                // Call the findPattern function to search for the marker pattern in memory.
                pbyLastMatch = findPattern((byte*)modInfo.lpBaseOfDll + dwMatchOffset, modInfo.SizeOfImage - dwMatchOffset, (byte*)MARKER_BYTES, MARKER_MASK, MARKER_SIZE);

                // If the marker pattern is found, replace it with random opcodes and update the offsets.
                if (pbyLastMatch != NULL)
                {
                    morphMemory(pbyLastMatch, (BYTE) MARKER_SIZE);
                    dwRegionCount++;

                    pbyLastMatch++;
                    dwMatchOffset = (DWORD)pbyLastMatch - (DWORD)modInfo.lpBaseOfDll;
                }
                    // If the marker pattern is not found, set the morphing status to finished.
                else
                {
                    bMorphingFinished = TRUE;
                }
            }
        }
    }

    // Clean up the process handle.
    CloseHandle(hProcess);
}


VOID morphMemory(PBYTE pbyDst, BYTE byLength)
{
    /*                  *
    *** JUNK CODE ALGO ***
    jmp        or      0x90
    rdm                jmp
    rdm                rdm
    rdm                rdm
    */

    // Initialize a flag to seed the random number generator
    static BOOL bSetSeed = TRUE;
    if (bSetSeed)
    {
        srand((UINT)time(NULL));
        bSetSeed = FALSE;
    }

    // Allocate memory for the opcodes to be morphed
    PBYTE morphedOpcodes = (PBYTE)malloc(byLength * sizeof(BYTE));
    BYTE byOpcodeIt = 0;

    // Determine whether to insert a NOP instruction at the beginning of the opcodes
    BOOL bPlaceNop = (rand() % 2) ? TRUE : FALSE;
    if (bPlaceNop)
    {
        morphedOpcodes[byOpcodeIt] = ASM_OPCODE_NOP;
        byOpcodeIt++;
    }

    // Insert a relative JMP instruction at the beginning of the opcodes
    morphedOpcodes[byOpcodeIt] = ASM_OPCODE_JMP_REL;
    byOpcodeIt++;

    // Calculate the length of the JMP instruction and insert it after the JMP instruction
    morphedOpcodes[byOpcodeIt] = byLength - ASM_INSTR_SIZE_JMP_REL - ((bPlaceNop) ? ASM_INSTR_SIZE_NOP : 0);
    byOpcodeIt++;

    // Insert random opcodes after the JMP instruction
    for (; byOpcodeIt < byLength; byOpcodeIt++)
        morphedOpcodes[byOpcodeIt] = rand() % MAXBYTE; // 0xFF

    // Change the protection of the memory to allow execution and write the morphed opcodes to memory
    DWORD dwOldProtect;
    VirtualProtect(pbyDst, byLength, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    rev_memcpy(pbyDst, morphedOpcodes, byLength);

    // Restore the original memory protection
    VirtualProtect(pbyDst, byLength, dwOldProtect, &dwOldProtect);

    // Free the memory allocated for the morphed opcodes
    free(morphedOpcodes);
}


PBYTE findPattern(PBYTE pData, SIZE_T uDataSize, PBYTE pPattern, PCHAR pszMask, SIZE_T uPatternSize)
{
    // loop over pData
    for (size_t i = 0; i < uDataSize - uPatternSize; i++) {
        BOOL bFound = TRUE;

        // loop over pPattern
        for (size_t j = 0; j < uPatternSize; j++) {

            // check if the current character in pszMask is 'x'
            // and if the corresponding bytes in pData and pPattern match
            if (pszMask[j] == 'x' && pData[i + j] != pPattern[j]) {
                bFound = FALSE;
                // exit inner loop if bytes do not match
                break;
            }
        }

        // if all bytes match, return pointer to found pattern in pData
        if (bFound) {
            return &pData[i];
        }
    }

    // pattern not found
    return NULL;
}

PVOID rev_memcpy (PBYTE dest, const PBYTE src, size_t n)
{
    // create a pointer to the beginning of the source buffer
    PBYTE s = src;

    // loop while n is not zero
    while (n--)

        // copy byte from src to dest using the s pointer and increment pointers
        *dest++ = *s++;

    // return pointer to the byte following the last byte written to dest
    return dest;
}



#else //CONFIG_POLY
#define $$$ __asm__ (   \
    "nop"          \
);

VOID morphModule()
{
    return;
}
#endif //CONFIG_POLY
#endif //REVENANT_POLY_H
