#include "soundengine.h"

#include "decoder/dumb/dumbdecoder.h"
#include "decoder/gme/gmedecoder.h"
#include "decoder/sc68/sc68decoder.h"
#include "decoder/sidplayfp/sidplaydecoder.h"
#include "strings.h"

#include <algorithm>
#include <SDL2/SDL_log.h>

SoundEngine::SoundEngine() :
    mStateMutex(SDL_CreateMutex()),
    mCurrentDecoder(nullptr) {
}

SoundEngine::~SoundEngine() {
    if (mStateMutex != nullptr) {
        SDL_DestroyMutex(mStateMutex);
        mStateMutex = nullptr;
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
    if (mCurrentDecoder != nullptr && mState != ERROR) {
        return mCurrentDecoder->getMetaData();
    }

    return mEmptyMetaData;
}

void SoundEngine::clearError() {
    SDL_LockMutex(mStateMutex);
    mState = FINISHED;
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
        mAudioDevice < 0) {

        mState = ERROR;
        mError = std::string(STR_ERROR_OPEN_AUDIO_DEVICE " : ").append(SDL_GetError());
        
        return false;
    }

    mAudioChannels = obtainedAudioSpec.channels;
    mAudioFrequency = obtainedAudioSpec.freq;
    mAudioSampleFormat = obtainedAudioSpec.format;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Audio current driver: %s, channels: %d, frequency: %d, sample format: 0x%X",
        SDL_GetCurrentAudioDriver(), mAudioChannels, mAudioFrequency, mAudioSampleFormat);

    // Instanciate all decoders
    mDecoderList.push_back(std::shared_ptr<Decoder>(new DumbDecoder()));
    mDecoderList.push_back(std::shared_ptr<Decoder>(new GmeDecoder()));
    mDecoderList.push_back(std::shared_ptr<Decoder>(new Sc68Decoder()));
    mDecoderList.push_back(std::shared_ptr<Decoder>(new SidPlayDecoder(std::string(dataPath).append("/c64roms"))));
    mCurrentDecoder = nullptr;

    mState = FINISHED;
    return true;
}

void SoundEngine::cleanup() {
    stop();
    SDL_CloseAudioDevice(mAudioDevice);
    mDecoderList.clear();
}

bool SoundEngine::canHandle(const std::shared_ptr<File> file) const {
    return getDecoder(file) != nullptr;
}

bool SoundEngine::load(const std::shared_ptr<File> file, std::shared_ptr<Settings> settings) {
    const auto path = file->getPath();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading %s ...\n", path.c_str());

    //Stop any previous songs
    stop();

    // Try to find if any decoder can handle the file
    mCurrentDecoder = getDecoder(file);

    // Decoder not found ? :/
    if (mCurrentDecoder == nullptr) {
        mState = ERROR;
        mError = std::string(STR_ERROR_NO_DECODER_CAN_HANDLE " \"").append(path).append("\"");
        return false;
    }

    // Get file content from File instance
    std::vector<char> buffer;
    if (!file->getAsBuffer(buffer)) {
        mState = ERROR;

        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error opening file: %s\n", file->getError().c_str());
        mError = std::string(STR_ERROR_CANT_OPEN_FILE " \"").append(path).append("\"");
        return false;
    }

    // Try to start song in internal decoder
    if (!mCurrentDecoder->setup()) {
        mState = ERROR;
        mError = STR_ERROR_DECODER_ERROR;
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error while initializing decoder: %s\n", mCurrentDecoder->getError().c_str());
        return false;
    }

    if (! mCurrentDecoder->play(buffer, settings)) {
        mState = ERROR;
        mError = STR_ERROR_CANT_PLAY_SONG;
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error trying to play song: %s\n", mCurrentDecoder->getError().c_str());
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Song loaded %s ...\n", path.c_str());
    mState = FINISHED;
    mError = "";
    return true;
}

void SoundEngine::stop() {
    SDL_PauseAudioDevice(mAudioDevice, true);
    SDL_ClearQueuedAudio(mAudioDevice);

    SDL_LockAudioDevice(mAudioDevice);
    if (mCurrentDecoder != nullptr) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Stop current decoder.\n");
        mCurrentDecoder->stop();
        mCurrentDecoder->cleanup();
        mCurrentDecoder = nullptr;
    }
    SDL_UnlockAudioDevice(mAudioDevice);

    mState = FINISHED;
    mError = "";
}

void SoundEngine::pause() {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Song paused.\n");
    
    SDL_PauseAudioDevice(mAudioDevice, true);
    mState = PAUSED;
    mError = "";
}

void SoundEngine::play() {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Playing...\n");

    mState = STARTED;
    mError = "";
    SDL_PauseAudioDevice(mAudioDevice, false);
}

bool SoundEngine::nextTrack() {
    switch (mState) {
        case SoundEngine::State::STARTED:
        case SoundEngine::State::PAUSED:
        case SoundEngine::State::FINISHED_NATURAL:
            if (mCurrentDecoder != nullptr) {
                SDL_ClearQueuedAudio(mAudioDevice);
                SDL_LockAudioDevice(mAudioDevice);
                auto retCode = mCurrentDecoder->nextTrack();
                SDL_UnlockAudioDevice(mAudioDevice);
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
                SDL_ClearQueuedAudio(mAudioDevice);
                SDL_LockAudioDevice(mAudioDevice);
                auto retCode = mCurrentDecoder->prevTrack();
                SDL_UnlockAudioDevice(mAudioDevice);
                return retCode;
            }
            break;
        default:
            break;
    }

    return false;
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

    memset(stream, 0, len);
    
    if (soundEngine->mState != STARTED) {
        // In case we come here.
        return;
    }

    if (soundEngine->mCurrentDecoder == nullptr) {
        // In case we come here.
        SDL_LockMutex(soundEngine->mStateMutex);
        soundEngine->mState = FINISHED_NATURAL;
        soundEngine->mError = "";
        SDL_UnlockMutex(soundEngine->mStateMutex);
        return;
    }
    
    switch (const auto retCode = soundEngine->mCurrentDecoder->process(stream, len);
            retCode) {

        case 0:
                // OK
            break;

        case 1: 
            // NATURAL FINISH
            SDL_LockMutex(soundEngine->mStateMutex);
            soundEngine->mState = FINISHED_NATURAL;
            soundEngine->mError = "";
            SDL_UnlockMutex(soundEngine->mStateMutex);
            break;
        
        case -1:
            // DECODER ERROR
            SDL_LockMutex(soundEngine->mStateMutex);
            soundEngine->mState = ERROR;
            soundEngine->mError = STR_ERROR_DECODER_ERROR;
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SoundEngine error: %s\n", soundEngine->mCurrentDecoder->getError().c_str());
            SDL_UnlockMutex(soundEngine->mStateMutex);
            break;

        default:
            break;
    }
}
