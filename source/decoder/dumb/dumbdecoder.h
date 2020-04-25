#pragma once

#include "../decoder.h"
#include "../../settings.h"

#include <string>
#include <vector>
#include <memory>
#include <dumb.h>
#include <SDL2/SDL_audio.h>

class DumbDecoder : public Decoder {

    public:
        static const std::string NAME;

        DumbDecoder();
        virtual ~DumbDecoder();
        
        virtual bool setup() override;
        virtual void cleanup() override;

        virtual std::string getName() const override;
        virtual int getAudioFrequency() const override;
        virtual uint8_t getAudioChannels() const override;
        virtual SDL_AudioFormat getAudioSampleFormat() const override;

        virtual bool canRead(const std::string extention) const override;
        virtual const MetaData getMetaData() override;
        virtual bool play(const std::vector<char> buffer, std::shared_ptr<Settings> settings) override;
        virtual void stop() override;   
        virtual int process(Uint8* stream, const int len) override;

    private:
        DUH *mDuh;
        DUMBFILE *mDumbFile;
        DUH_SIGRENDERER *mSigRenderer;

        DumbDecoder(const DumbDecoder& copy);

        void parseMetaData();
};