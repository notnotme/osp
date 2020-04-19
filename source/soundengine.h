#pragma once

#include "filesystem/file.h"
#include "decoder/decoder.h"

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <SDL2/SDL_mutex.h>

class SoundEngine {

    public:
        enum State {
            STARTED,
            PAUSED,
            FINISHED,
            FINISHED_NATURAL,
            ERROR
        };

        SoundEngine();
        virtual ~SoundEngine();

        bool setup(const std::filesystem::path dataPath);
        void cleanup();

        bool canHandle(const std::shared_ptr<File> file) const;
        bool load(const std::shared_ptr<File> file, bool defaultTune);
        void stop();
        void pause();
        void play();

        bool nextTrack();
        bool prevTrack();

        Decoder::MetaData getMetaData() const;
        State getState() const;
        std::string getError() const;
        void clearError();

    private:
        const Decoder::MetaData mEmptyMetaData;
        SDL_mutex* mStateMutex;
        SDL_mutex* mDecoderMutex;
        std::string mError;
        State mState;

        SDL_AudioDeviceID mAudioDevice;
        SDL_AudioFormat mAudioSampleFormat;
        uint8_t mAudioChannels;
        int mAudioFrequency;

        std::vector<std::shared_ptr<Decoder>> mDecoderList;
        std::shared_ptr<Decoder> mCurrentDecoder;
        
        SoundEngine(const SoundEngine& copy);
        void loadAllDecoder(std::filesystem::path dataPath);
        std::shared_ptr<Decoder> getDecoder(const std::shared_ptr<File> file) const;
        static void audioCallback(void *userdata, Uint8* stream, int len);

};
