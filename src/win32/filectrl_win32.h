#ifndef __FILECTRLWIN32_H__
#define __FILECTRLWIN32_H__

#include "filectrl.h"

class FileControlWin32 : FileControl
{
    public:
        int Open( const char* name );
        int Open( const wchar_t* name );
        int Close();
        int Seek( unsigned pos );
        int Read( char* buffer, unsigned len );

    public:
        unsigned Size();

    private:
        HANDLE  hFile;
        DWORD   fileSize;

    private:
        FileControlWin32();
        virtual ~FileControlWin32();
};

#endif /// of __FILECTRLWIN32_H__
