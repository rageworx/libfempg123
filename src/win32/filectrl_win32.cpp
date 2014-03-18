#include <windows.h>
#include <cstring>
#include "filectrl_win32.h"

FileControlWin32::FileControlWin32()
 : hFile( INVALID_HANDLE_VALUE ),
   fileSize( 0 )
{

}

FileControlWin32::~FileControlWin32()
{
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }
}

int FileControlWin32::Open( const char* name )
{
    wchar_t remapstr[MAX_PATH] = {0};

    mbstowcs( remapstr, name, MAX_PATH );

    return Open( remapstr );
}

int FileControlWin32::Open( const wchar_t* name )
{
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        Close();
    }

    hFile = CreateFileW( name,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        GetFileSize( hFile, &fileSize );
        return (int)hFile;
    }

    return -1;
}

int FileControlWin32::Close()
{
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;
        fileSize = 0;
    }

    return 0;
}

int FileControlWin32::Seek( unsigned pos )
{
    if ( hFile == INVALID_HANDLE_VALUE )
        return -1;

    if ( pos > fileSize )
        return -2;

    return SetFilePointer( hFile, pos, NULL, FILE_BEGIN );
}

int FileControlWin32::Read( char* buffer, unsigned len )
{
    if ( hFile == INVALID_HANDLE_VALUE )
        return -1;

    if ( buffer == NULL )
        return -2;

    DWORD readout = 0;

    if ( ReadFile( hFile, buffer, len, &readout, NULL ) == TRUE )
    {
        return (unsigned)readout;
    }

    return 0;
}

unsigned FileControlWin32::Size()
{
    return fileSize;
}
