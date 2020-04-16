#pragma once

#include <string>
#include <vector>
#include <SDL2/SDL_audio.h>

class Decoder {
    
    public:
        struct DiskInformation {
            std::string title = "";
            std::string author = "";
            std::string copyright = "";
            std::string ripper = "";
            std::string converter = "";
            int trackCount = 0;
            int duration = 0;
        };

        struct TrackInformation {
            std::string title = "None.";
            std::string author = "";
            std::string comment = "";
            std::string copyright = "";
            int trackNumber = 0;
            int duration = 0;
            int position = -1;
        };

        struct MetaData {
            bool hasDiskInformation = false;
            DiskInformation diskInformation;
            TrackInformation trackInformation;
        };

        Decoder();
        virtual ~Decoder();

        virtual std::string getName() = 0;
        virtual bool setup() = 0;
        virtual void cleanup() = 0;

        virtual int getAudioFrequency() const = 0;
        virtual uint8_t getAudioChannels() const = 0;
        virtual SDL_AudioFormat getAudioSampleFormat() const = 0;

        virtual bool canRead(const std::string extention) const = 0;
        virtual const MetaData getMetaData() = 0;
        virtual bool play(const std::vector<char> buffer) = 0;
        virtual void stop() = 0;
        virtual int process(Uint8* stream, const int len) = 0;

        virtual bool nextTrack();
        virtual bool prevTrack();

        std::string getError() const;

    protected:
        MetaData mMetaData;
        std::string mError;

    private:
        Decoder(const Decoder& copy);

};
