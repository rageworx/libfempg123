#ifndef __FEOUTPUT_H__
#define __FEOUTPUT_H__

#include <string>

struct FEInformation
{
    unsigned        version;
    std::string     name;
    std::string     description;
    std::string     revision;
    void*           handle;
};

struct FEContext
{
    unsigned char   flags;      /// deivce flags = > OUT_HEADPHONES, INTER ...
    unsigned        rate;       /// sampling rate
    unsigned        gain;       /// output gaim
    unsigned char   channels;   /// channels.
    bool            opened;     /// device opened?
    unsigned char   auxflag;    /// auxiliary flag : normallyt quient mode ?
    void*           userptr;    /// user pointer.
};

class FEOutputEvents
{
    public:
        virtual int  GetFormats( FEContext* context ) = 0;
        virtual int  Open( FEContext* context ) = 0;
        virtual int  Write( FEContext* context, unsigned char* buff, unsigned bufflen ) = 0;
        virtual void Flush( FEContext* context ) = 0;
        virtual int  Close( FEContext* context ) = 0;
        virtual int  Reset( FEContext* context ) = 0;
};

class FEOutput : public FEOutputEvents
{
    public:
        #define OUT_HEADPHONES          0x01
        #define OUT_INTERNAL_SPEAKER    0x02
        #define OUT_LINE_OUT            0x04

    public:
        enum{
            TYPE_TEST = 0,
            TYPE_AUDIO,
            TYPE_FILE,
            TYPE_BUFFER,
            TYPE_WAV,
            TYPE_AU,
            TYPE_CDR,
            TYPE_AUDIOFILE
        };

    public:
        FEOutput() : _info(NULL) {};

    public:
        FEInformation* info()   { return _info; }
        const char* lasterror() { return _lasterr.c_str(); }

    protected:
        FEInformation*  _info;
        std::string     _lasterr;
};

#endif /// of __FEOUTPUT_H__
