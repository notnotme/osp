#pragma once

#include "../decoder.h"
#include "../../settings.h"

#include <string>
#include <vector>
#include <memory>
#include <sc68/sc68.h>
#include <SDL2/SDL_audio.h>

class Sc68Decoder : public Decoder {

    public:
        static const std::string NAME;

        Sc68Decoder();
        virtual ~Sc68Decoder();
        
        virtual bool setup() override;
        virtual void cleanup() override;
        virtual std::string getName() override;

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
        int mCurrentTrack;
        bool mIsSongLoaded;
        sc68_create_t mSC68Config;
        sc68_t* mSC68;

        Sc68Decoder(const Sc68Decoder& copy);
        void parseDiskMetaData();
        void parseTrackMetaData(int track);

};
