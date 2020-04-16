#include "sc68decoder.h"

#include <SDL2/SDL_log.h>

Sc68Decoder::Sc68Decoder() :
    Decoder(),
    mSC68(nullptr) {
}

Sc68Decoder::~Sc68Decoder() {
}

bool Sc68Decoder::setup() {
    if(sc68_init(0)) {
        mError = "init sc68 error.";
        return false;
    }

    memset(&mSC68Config, 0, sizeof(mSC68Config));
    mSC68Config.sampling_rate = 48000;
    if (mSC68 = sc68_create(&mSC68Config),
        mSC68 == nullptr) {

        mError = sc68_error(mSC68);
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SC68: %s", sc68_error(mSC68));
        return false;
    }

    return true;
}

void Sc68Decoder::cleanup() {
    if (mSC68 != nullptr) {
        sc68_destroy(mSC68);
        sc68_shutdown();
        mSC68 = nullptr;
    }
}

uint8_t Sc68Decoder::getAudioChannels() const {
    return 2;
}

SDL_AudioFormat Sc68Decoder::getAudioSampleFormat() const {
    return AUDIO_S16SYS;
}

const Decoder::MetaData Sc68Decoder::getMetaData() {
    return mMetaData;
}

int Sc68Decoder::getAudioFrequency() const {
    return mSC68Config.sampling_rate;
}

std::string Sc68Decoder::getName() {
    return "sc68";
}

bool Sc68Decoder::canRead(const std::string extention) const {
    if(extention == ".snd") { return true; }
    if(extention == ".sndh") { return true; }
    if(extention == ".sc68") { return true; }
    return false;
}

bool Sc68Decoder::play(const std::vector<char> buffer) {
    if (sc68_load_mem(mSC68, buffer.data(), buffer.size()) != 0) {
        mIsSongLoaded = false;
        mError = std::string("Can't play file.");
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SC68: %s", sc68_error(mSC68));
        return false;
    }

    mCurrentTrack = 1;
    if (auto track = sc68_play(mSC68, mCurrentTrack, 1);
        track < 0) {
            
        sc68_close(mSC68);
        mIsSongLoaded = false;
        mError = std::string("No track to play");
        return false;
    }

    mMetaData = MetaData();
    parseDiskMetaData();
    parseTrackMetaData();

    mIsSongLoaded = true;
    return true;
}

void Sc68Decoder::stop() {
    mIsSongLoaded = false;
    sc68_stop(mSC68);
    sc68_close(mSC68);
}


bool Sc68Decoder::nextTrack() {
    if (mCurrentTrack < mMetaData.diskInformation.trackCount) {
        mCurrentTrack++;
        if (sc68_play(mSC68, mCurrentTrack, 1) != 0) {
            return false;
        }

        parseTrackMetaData();
        return true;
    }

    return false;
}

bool Sc68Decoder::prevTrack() {
    if (mCurrentTrack > 1) {
        mCurrentTrack--;
        if (sc68_play(mSC68, mCurrentTrack, 1) != 0) {
            return false;
        }
        
        parseTrackMetaData();
        return true;
    }

    return false;
}

int Sc68Decoder::process(Uint8* stream, const int len) {
    if (!mIsSongLoaded || mSC68 == nullptr) return -1;

    auto amount = len >> 2;
    auto retCode = sc68_process(mSC68, stream, &amount);
    
    if ((retCode & SC68_END)) {
        if (!nextTrack()) {
            return 1;
        }
    }

    return 0;
}

void Sc68Decoder::parseDiskMetaData() {
    sc68_music_info_t musicInfo;
    if (sc68_music_info(mSC68, &musicInfo, -1, 0) == 0) {
        mMetaData.hasDiskInformation = true;
        mMetaData.diskInformation.title = musicInfo.album;
        mMetaData.diskInformation.ripper = musicInfo.ripper;
        mMetaData.diskInformation.trackCount = musicInfo.tracks;
        mMetaData.diskInformation.converter = musicInfo.converter;
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SC68: Cannot get disk metadata.\n");
    }
}

void Sc68Decoder::parseTrackMetaData() {
    sc68_music_info_t musicInfo;
    if (sc68_music_info(mSC68, &musicInfo, mCurrentTrack, 0) == 0) {
        mMetaData.trackInformation.title = musicInfo.title;
        mMetaData.trackInformation.author = musicInfo.artist;
        mMetaData.trackInformation.trackNumber = musicInfo.trk.track;
        mMetaData.trackInformation.duration = (int) (musicInfo.trk.time_ms / 1000);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SC68: Cannot get track metadata.\n");
    }
}
