#include "soundengine.h"

#include "decoder/dumbdecoder.h"
#include "decoder/gmedecoder.h"
#include "decoder/sc68decoder.h"
#include "decoder/sidplaydecoder.h"

#include <algorithm>
#include <SDL2/SDL_log.h>

SoundEngine::SoundEngine() :
    mStateMutex(SDL_CreateMutex()),
    mDecoderMutex(SDL_CreateMutex()),
    mCurrentDecoder(nullptr) {
}

SoundEngine::~SoundEngine() {
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
    mError = "";
    SDL_UnlockMutex(mStateMutex);
}

bool SoundEngine::setup(const std::filesystem::path dataPath) {
    // Setup default sound output
    SDL_AudioSpec obtainedAudioSpec;
    SDL_AudioSpec wantedAudioSpec;
    wantedAudioSpec.callback = SoundEngine::audioCallback;
    wantedAudioSpec.userdata = this;
    wantedAudioSpec.samples = 2048; // 2048 for better latency ?
    wantedAudioSpec.channels = 2;
    wantedAudioSpec.format = AUDIO_S16SYS;
    wantedAudioSpec.freq = 48000;

    // todo check if sdl will resample the data and warn user
    if (mAudioDevice = SDL_OpenAudioDevice(nullptr, 0, &wantedAudioSpec, &obtainedAudioSpec, 0);
        mAudioDevice < 0 ) {

        SDL_LockMutex(mStateMutex);
        mState = ERROR;
        mError = std::string("Couldn't open audio device: ").append(SDL_GetError());
        SDL_UnlockMutex(mStateMutex);
        
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

bool SoundEngine::canHandle(const std::shared_ptr<File> file) const {
    return getDecoder(file) != nullptr;
}

bool SoundEngine::load(const std::shared_ptr<File> file) {
    const auto path = file->getPath();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading %s ...\n", path.c_str());

    //Stop any previous songs
    stop();

    // Try to find if any decoder can handle the file
    SDL_LockMutex(mDecoderMutex);
    mCurrentDecoder = getDecoder(file);
    SDL_UnlockMutex(mDecoderMutex);

    // Decoder not found ? :/
    if (mCurrentDecoder == nullptr) {
        SDL_LockMutex(mStateMutex);
        mState = ERROR;
        mError = std::string("No decoder can handle \"").append(path).append("\"");
        SDL_UnlockMutex(mStateMutex);

        return false;
    }

    // Get file content from File instance
    std::vector<char> buffer;
    if (!file->getAsBuffer(buffer)) {
        SDL_LockMutex(mStateMutex);
        mState = ERROR;
        mError = std::string("Can't read file \"").append(path).append("\"");
        SDL_UnlockMutex(mStateMutex);

        return false;
    }

    // Try to start song in internal decoder
    SDL_LockMutex(mDecoderMutex);
    if (! mCurrentDecoder->play(buffer)) {
        mCurrentDecoder->stop();
        SDL_UnlockMutex(mDecoderMutex);

        SDL_LockMutex(mStateMutex);
        mState = ERROR;
        mError = std::string("Decoder ").append(mCurrentDecoder->getName().c_str())
            .append(" can't play song: ").append(mCurrentDecoder->getError().c_str());
        SDL_UnlockMutex(mStateMutex);
        
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s\n", mError.c_str());
        return false;
    }
    SDL_UnlockMutex(mDecoderMutex);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Song loaded %s ...\n", path.c_str());

    SDL_LockMutex(mStateMutex);
        mState = LOADED;
    SDL_UnlockMutex(mStateMutex);
    return true;
}

void SoundEngine::stop() {
    SDL_PauseAudioDevice(mAudioDevice, true);
    SDL_ClearQueuedAudio(mAudioDevice);

    if (mCurrentDecoder != nullptr) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Stop current decoder.\n");
        SDL_LockMutex(mDecoderMutex);
        mCurrentDecoder->stop();
        mCurrentDecoder = nullptr;
        SDL_UnlockMutex(mDecoderMutex);
    }

    SDL_LockMutex(mStateMutex);
        mState = FINISHED;
    SDL_UnlockMutex(mStateMutex);
}

void SoundEngine::pause() {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Song paused.\n");
    SDL_LockMutex(mStateMutex);
        mState = PAUSED;
    SDL_UnlockMutex(mStateMutex);
    SDL_PauseAudioDevice(mAudioDevice, true);
}

void SoundEngine::play() {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Playing...\n");
    SDL_LockMutex(mStateMutex);
        mState = STARTED;
    SDL_UnlockMutex(mStateMutex);
    SDL_PauseAudioDevice(mAudioDevice, false);
}

bool SoundEngine::nextTrack() {
    switch (mState) {
        case SoundEngine::State::STARTED:
        case SoundEngine::State::PAUSED:
        case SoundEngine::State::FINISHED_NATURAL:
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
        case SoundEngine::State::FINISHED_NATURAL:
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

std::shared_ptr<Decoder> SoundEngine::getDecoder(const std::shared_ptr<File> file) const {
    // Try to find if any decoder can handle the file
    auto extention = std::string(file->getPath().extension());
    std::transform(extention.begin(), extention.end(), extention.begin(), ::tolower);

    std::shared_ptr<Decoder> decoderFound;
    for (const auto decoder : mDecoderList) {
        if (decoder->canRead(extention)) {
            decoderFound = decoder;
            break;
        }
    }

    return decoderFound;
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
            soundEngine->mError = std::string("Decoder error: ").append(soundEngine->mCurrentDecoder->getError());
            SDL_UnlockMutex(soundEngine->mStateMutex);
            break;

        default:
            break;
    }
}
