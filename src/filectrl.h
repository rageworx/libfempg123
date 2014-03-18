#ifndef __FILECTRL_H__
#define __FILECTRL_H__

class FileControl
{
    public:
        virtual int Open( const char* name ) = 0;
        virtual int Open( const wchar_t* name ) = 0;
        virtual int Close() = 0;
        virtual int Seek( unsigned pos ) = 0;
        virtual int Read( char* buffer, unsigned len ) = 0;

    public:
        virtual unsigned Size() = 0;
};

#endif /// of __FILECTRL_H__
