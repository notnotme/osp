#include "soundengine.h"

#include "decoder/dumbdecoder.h"
#include "decoder/gmedecoder.h"
#include "decoder/sc68decoder.h"
#include "decoder/sidplaydecoder.h"

#include <algorithm>
#include <SDL2/SDL_log.h>

SoundEngine::SoundEngine() :
    mErrorMutex(SDL_CreateMutex()),
    mStateMutex(SDL_CreateMutex()),
    mDecoderMutex(SDL_CreateMutex()),
    mCurrentDecoder(nullptr) {
}

SoundEngine::~SoundEngine() {
    if (mErrorMutex != nullptr) {
        SDL_DestroyMutex(mErrorMutex);
        mErrorMutex = nullptr;
    }

    if (mStateMutex != nullptr) {
        SDL_DestroyMutex(mStateMutex);
        mStateMutex = nullptr;
    }

    if (mDecoderMutex != nullptr) {
        SDL_DestroyMutex(mDecoderMutex);
        mDecoderMutex = nullptr;
    }
}

SoundEngine::State SoundEngine::getState() const {
    return mState;
}

std::string SoundEngine::getError() const {
    return mError;
}

Decoder::MetaData SoundEngine::getMetaData() const {
    // No need to check engine state, if current deocder is not null
    // it can send us the meta data
    if (mCurrentDecoder != nullptr) {
        return mCurrentDecoder->getMetaData();
    }

    return mEmptyMetaData;
}

void SoundEngine::clearError() {
    SDL_LockMutex(mStateMutex);
    mState = READY;
    SDL_UnlockMutex(mStateMutex);

    SDL_LockMutex(mErrorMutex);
    mError = "";
    SDL_UnlockMutex(mErrorMutex);
}

bool SoundEngine::setup(const std::filesystem::path dataPath) {
    // Setup default sound output
    SDL_AudioSpec obtainedAudioSpec;
    SDL_AudioSpec wantedAudioSpec;
    wantedAudioSpec.callback = SoundEngine::audioCallback;
    wantedAudioSpec.userdata = this;
    wantedAudioSpec.samples = 4096; // 2048 for better latency ?
    wantedAudioSpec.channels = 2;
    wantedAudioSpec.format = AUDIO_S16SYS;
    wantedAudioSpec.freq = 48000;

    // todo check if sdl will resample the data and warn user
    if (mAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantedAudioSpec, &obtainedAudioSpec, 0);
        mAudioDevice < 0 ) {

        SDL_LockMutex(mErrorMutex);
        mError = std::string("Couldn't open audio device: ").append(SDL_GetError());
        SDL_UnlockMutex(mErrorMutex);
        
        return false;
    }

    mAudioChannels = obtainedAudioSpec.channels;
    mAudioFrequency = obtainedAudioSpec.freq;
    mAudioSampleFormat = obtainedAudioSpec.format;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Audio current driver: %s, channels: %d, frequency: %d, sample format: 0x%X",
        SDL_GetCurrentAudioDriver(), mAudioChannels, mAudioFrequency, mAudioSampleFormat);

    loadAllDecoder(dataPath);
    mCurrentDecoder = nullptr;

    SDL_LockMutex(mStateMutex);
    mState = READY;
    SDL_UnlockMutex(mStateMutex);

    return true;
}

void SoundEngine::cleanup() {
    stop();
    SDL_CloseAudioDevice(mAudioDevice);

    mCurrentDecoder = nullptr;
    for (const auto decoder : mDecoderList) {
        decoder->cleanup();
    }    
    mDecoderList.clear();
}

bool SoundEngine::load(const std::shared_ptr<File> file) {
    const auto path = file->getPath();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading %s ...\n", path.c_str());

    //Stop any previous songs
    stop();

    // Try to find if any decoder can handle the file
    auto extention = std::string(path.extension());
    std::transform(extention.begin(), extention.end(), extention.begin(), ::tolower);

    SDL_LockMutex(mDecoderMutex);
    for (const auto decoder : mDecoderList) {
        if (decoder->canRead(extention)) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Selected decoder: %s.\n", decoder->getName().c_str());
            mCurrentDecoder = decoder;
            break;
        }
    }
    SDL_UnlockMutex(mDecoderMutex);

    // Decoder not found ? :/
    if (mCurrentDecoder == nullptr) {
        SDL_LockMutex(mStateMutex);
            mState = ERROR;
        SDL_UnlockMutex(mStateMutex);

        SDL_LockMutex(mErrorMutex);
        mError = std::string("No decoder can handle \"").append(path).append("\"");
        SDL_UnlockMutex(mErrorMutex);

        return false;
    }

    // Get file content from File instance
    std::vector<char> buffer;
    if (!file->getAsBuffer(buffer)) {
        SDL_LockMutex(mStateMutex);
            mState = ERROR;
        SDL_UnlockMutex(mStateMutex);

        SDL_LockMutex(mErrorMutex);
        mError = std::string("Can't read file \"").append(path).append("\"");
        SDL_UnlockMutex(mErrorMutex);

        return false;
    }

    // Try to play song
    if (! mCurrentDecoder->play(buffer)) {
        mCurrentDecoder->stop();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Decoder %s can't play song \"%s\".\n",
            mCurrentDecoder->getName().c_str(), mCurrentDecoder->getError().c_str());

    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Song is read to play %s ...\n", path.c_str());

    SDL_LockMutex(mStateMutex);
        mState = LOADED;
    SDL_UnlockMutex(mStateMutex);
    
    play();
    return true;
}

void SoundEngine::stop() {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Stop current song.\n");

    SDL_PauseAudioDevice(mAudioDevice, true);
    SDL_ClearQueuedAudio(mAudioDevice);

    SDL_LockMutex(mStateMutex);
        mState = READY;
    SDL_UnlockMutex(mStateMutex);

    if (mCurrentDecoder != nullptr) {
        SDL_LockMutex(mDecoderMutex);
        mCurrentDecoder->stop();
        mCurrentDecoder = nullptr;
        SDL_UnlockMutex(mDecoderMutex);
    }
}

void SoundEngine::pause() {
    SDL_LockMutex(mStateMutex);
        mState = PAUSED;
    SDL_UnlockMutex(mStateMutex);
    SDL_PauseAudioDevice(mAudioDevice, true);
}

void SoundEngine::play() {
    SDL_LockMutex(mStateMutex);
        mState = STARTED;
    SDL_UnlockMutex(mStateMutex);
    SDL_PauseAudioDevice(mAudioDevice, false);
}

bool SoundEngine::nextTrack() {
    switch (mState) {
        case SoundEngine::State::STARTED:
        case SoundEngine::State::PAUSED:
            if (mCurrentDecoder != nullptr) {
                SDL_PauseAudioDevice(mAudioDevice, true);
                SDL_ClearQueuedAudio(mAudioDevice);

                SDL_LockMutex(mDecoderMutex);
                auto retCode = mCurrentDecoder->nextTrack();
                SDL_UnlockMutex(mDecoderMutex);

                SDL_PauseAudioDevice(mAudioDevice, false);
                return retCode;
            }
            break;
        default:
            break;
    }

    return false;
}

bool SoundEngine::prevTrack() {
    switch (mState) {
        case SoundEngine::State::STARTED:
        case SoundEngine::State::PAUSED:
            if (mCurrentDecoder != nullptr) {
                SDL_PauseAudioDevice(mAudioDevice, true);
                SDL_ClearQueuedAudio(mAudioDevice);

                SDL_LockMutex(mDecoderMutex);
                auto retCode = mCurrentDecoder->prevTrack();
                SDL_UnlockMutex(mDecoderMutex);

                SDL_PauseAudioDevice(mAudioDevice, false);
                return retCode;
            }
            break;
        default:
            break;
    }

    return false;
}


void SoundEngine::loadAllDecoder(std::filesystem::path dataPath) {
    if (const auto decoder = std::shared_ptr<Decoder>(new DumbDecoder());
        decoder->setup() == false) {
        
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load GME.\n");
        decoder->cleanup();
    } else {
        mDecoderList.push_back(decoder);
    }

    if (const auto decoder = std::shared_ptr<Decoder>(new GmeDecoder());
        decoder->setup() == false) {
        
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load GME.\n");
        decoder->cleanup();
    } else {
        mDecoderList.push_back(decoder);
    }

    if (const auto decoder = std::shared_ptr<Decoder>(new Sc68Decoder());
        decoder->setup() == false) {
        
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load SC68.\n");
        decoder->cleanup();
    } else {
        mDecoderList.push_back(decoder);
    }

    const auto sidplayDataPath = std::string(dataPath).append("/c64roms"); 
    if (const auto decoder = std::shared_ptr<Decoder>(new SidPlayDecoder(sidplayDataPath));
        decoder->setup() == false) {
        
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot load SIDPLAYFP.\n");
        decoder->cleanup();
    } else {
        mDecoderList.push_back(decoder);
    }
}

void SoundEngine::audioCallback(void *userdata, Uint8* stream, int len) {
    const auto soundEngine = static_cast<SoundEngine*>(userdata);

    if (soundEngine->mState != STARTED) {
        // In case we come here.
        memset(stream, 0, len);
        return;
    }

    if (soundEngine->mCurrentDecoder == nullptr) {
        // In case we come here.
        SDL_LockMutex(soundEngine->mStateMutex);
            soundEngine->mState = FINISHED_NATURAL;
        SDL_UnlockMutex(soundEngine->mStateMutex);
        memset(stream, 0, len);
        return;
    }

    SDL_LockMutex(soundEngine->mDecoderMutex);
    const auto retCode = soundEngine->mCurrentDecoder->process(stream, len);
    SDL_UnlockMutex(soundEngine->mDecoderMutex);
    
    switch (retCode) {
        case 0:
                // OK
            break;

        case 1: 
            // NATURAL FINISH
            SDL_LockMutex(soundEngine->mStateMutex);
                soundEngine->mState = FINISHED_NATURAL;
            SDL_UnlockMutex(soundEngine->mStateMutex);
            break;
        
        case -1:
            // DECODER ERROR
            SDL_LockMutex(soundEngine->mStateMutex);
                soundEngine->mState = ERROR;
            SDL_UnlockMutex(soundEngine->mStateMutex);

            SDL_LockMutex(soundEngine->mErrorMutex);
            soundEngine->mError = std::string("Decoder error: ").append(soundEngine->mCurrentDecoder->getError());
            SDL_UnlockMutex(soundEngine->mErrorMutex);
            break;

        default:
            break;
    }
}
