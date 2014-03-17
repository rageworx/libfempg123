#ifndef __FECODEC_WIN32_H__
#define __FECODEC_WIN32_H__

#include "fecodec.h"

class FECodecWin32 : protected FECodec
{
    public:
        FECodecWin32();
        virtual ~FECodecWin32();

    public:
        int  GetFormats( FEContext* context );
        int  Open( FEContext* context );
        int  Write( FEContext* context, unsigned char* buff, unsigned bufflen );
        void Flush( FEContext* context );
        int  Close( FEContext* context );
        int  Reset( FEContext* context );
};

#endif /// of __FECODEC_WIN32_H__
