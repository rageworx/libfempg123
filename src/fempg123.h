#ifndef __FEMPG123_H__
#define __FEMPG123_H__

class FECodec;

class FEMpg123
{
    public:
        FEMpg123( FECodec* codec );
        virtual ~FEMpg123();

    public:
        int  InitCodec();
        int  InitCodec( FECodec* codec );
        int  FinalCodec();

    public:
        int  LoadFile( const char* refmp3 );
        int  LoadFile( const wchar_t* refmp3 );
        int  UnloadFile();
};

#endif /// of __FEMPG123_H__
