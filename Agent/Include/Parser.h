#ifndef REVENANT_PARSER_H
#define REVENANT_PARSER_H

#include <windows.h>

typedef struct {
    PCHAR  Original;
    PCHAR  Buffer;
    UINT32 Size; // Should be change to total_size?
    UINT32 Length;

    UINT32 ChunkID; // Tell the client and server what the ChunkID is when the file transferred
    UINT32 TotalChunkNumber; //  Tell the client and server how many chunks the file is divided into


    BOOL   Endian;
} PARSER, *PPARSER;

void  ParserNew(PPARSER parser, PVOID buffer, UINT32 size);
void  ParserDecrypt(PPARSER parser, PBYTE Key, PBYTE IV);
int   ParserGetInt32(PPARSER parser);
PCHAR ParserGetBytes(PPARSER parser, PUINT32 size);
void  ParserDestroy(PPARSER Parser);


#endif //REVENANT_PARSER_H
