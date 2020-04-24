#pragma once

#include "../decoder.h"
#include "../../settings.h"

#include <string>
#include <vector>
#include <memory>                    
#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/sidbuilder.h>
#include <SDL2/SDL_audio.h>

class SidPlayDecoder : public Decoder {

    public:
        static const std::string NAME;

        SidPlayDecoder(const std::string dataPath);
        virtual ~SidPlayDecoder();
      
        virtual std::string getName() override;
        virtual bool setup() override;
        virtual void cleanup() override;

        virtual int getAudioFrequency() const override;
        virtual uint8_t getAudioChannels() const override;
        virtual SDL_AudioFormat getAudioSampleFormat() const override;
      
        virtual bool canRead(const std::string extention) const override;
        virtual const MetaData getMetaData() override;
        virtual bool play(const std::vector<char> buffer, std::shared_ptr<Settings> settings) override;
        virtual void stop() override;
        virtual int process(Uint8* stream, const int len) override;

        virtual bool nextTrack() override;
        virtual bool prevTrack() override;

    private:
        std::shared_ptr<char[]> mKernalRom;
        std::shared_ptr<char[]> mBasicRom;
        std::shared_ptr<char[]> mChargenRom;

        std::unique_ptr<sidplayfp> mPlayer;
        std::unique_ptr<sidbuilder> mSIDBuilder;
        std::unique_ptr<SidTune> mTune;

        SidPlayDecoder(const SidPlayDecoder& copy);

        void parseDiskMetaData();
        void parseTrackMetaData();

        static char* loadRom(const std::string path, const size_t romSize);

};
