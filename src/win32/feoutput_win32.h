#ifndef __FEOUTPUT_WIN32_H__
#define __FEOUTPUT_WIN32_H__

#include "feoutput.h"

class FEOutputWin32 : protected FEOutput
{
    public:
        FEOutputWin32();
        virtual ~FEOutputWin32();

    public:
        int  GetFormats( FEContext* context );
        int  Open( FEContext* context );
        int  Write( FEContext* context, unsigned char* buff, unsigned bufflen );
        void Flush( FEContext* context );
        int  Close( FEContext* context );
        int  Reset( FEContext* context );
};

#endif /// of __FEOUTPUT_WIN32_H__
