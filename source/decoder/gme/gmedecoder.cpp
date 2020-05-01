#include "gmedecoder.h"

#include "gme_settings_strings.h"

#include <SDL2/SDL_log.h>

const std::string GmeDecoder::NAME = "gme";

GmeDecoder::GmeDecoder() :
    Decoder(),
    mMusicEmu(nullptr) {
}

GmeDecoder::~GmeDecoder() {
}

bool GmeDecoder::setup() {
    return true;
}

void GmeDecoder::cleanup() {
    if (mMusicEmu != nullptr) {
        gme_delete(mMusicEmu);
        mMusicEmu = nullptr;
    }
}

uint8_t GmeDecoder::getAudioChannels() const {
    return 2;
}

SDL_AudioFormat GmeDecoder::getAudioSampleFormat() const {
    return AUDIO_S16SYS;
}

const Decoder::MetaData GmeDecoder::getMetaData() {
    if (mMusicEmu == nullptr) {
        return mMetaData;
    }

    mMetaData.trackInformation.position = gme_tell(mMusicEmu) / 1000;
    return mMetaData;
}

int GmeDecoder::getAudioFrequency() const {
    return 48000;
}

std::string GmeDecoder::getName() const {
    return NAME;
}


bool GmeDecoder::canRead(const std::string extention) const {
    if(extention == ".ay") { return true; }
    if(extention == ".gbs") { return true; }
    if(extention == ".gym") { return true; }
    if(extention == ".hes") { return true; }
    if(extention == ".kss") { return true; }
    if(extention == ".nsf") { return true; }
    if(extention == ".nsfe") { return true; }
    if(extention == ".sap") { return true; }
    if(extention == ".spc") { return true; }
    if(extention == ".vgm") { return true; }
    if(extention == ".vgz") { return true; } 
    return false;
}

bool GmeDecoder::play(const std::vector<char> buffer, std::shared_ptr<Settings> settings) {
    if (const auto header = gme_identify_header(buffer.data());
        header[0] == '\0') {

        mError = gme_wrong_file_type;
        return false;
    }

    if (const auto error = gme_open_data(buffer.data(), buffer.size(), &mMusicEmu, 48000);
        error != nullptr) {

        mError = "Can't open file.";
        return false;
    }

    const auto enableAccuracy = settings->getBool(KEY_GME_ENABLE_ACCURACY, GME_ENABLE_ACCURACY_DEFAULT) ? 1 : 0;
    const auto autoloadPlaybackLimit = settings->getBool(KEY_GME_AUTOLOAD_PLAYBACK_LIMIT, GME_AUTOLOAD_PLAYBACK_LIMIT_DEFAULT) ? 1 : 0;
    const auto ignoreSilence = settings->getBool(KEY_GME_IGNORE_SILENCE, GME_GME_IGNORE_SILENCE_DEFAULT) ? 1 : 0;

    gme_enable_accuracy(mMusicEmu, enableAccuracy);
    gme_set_autoload_playback_limit(mMusicEmu, autoloadPlaybackLimit);
    gme_ignore_silence(mMusicEmu, ignoreSilence);

    mCurrentTrack = 0;
    gme_start_track(mMusicEmu, mCurrentTrack);

    mMetaData = MetaData();
    parseDiskMetaData();
    parseTrackMetaData();

    return true;
}

void GmeDecoder::stop() {
    if (mMusicEmu != nullptr) {
        gme_delete(mMusicEmu);
        mMusicEmu = nullptr;
    }
}

bool GmeDecoder::nextTrack() {
    if (mCurrentTrack < mMetaData.diskInformation.trackCount-1) {

        mCurrentTrack++;
        if (const auto error = gme_start_track(mMusicEmu, mCurrentTrack);
            error != nullptr) {
            
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GME: %s", error);
            return false;
        }
    
        parseTrackMetaData();
        return true;
    }

    return false;
}

bool GmeDecoder::prevTrack() {
    if (mCurrentTrack > 0) {
        
        mCurrentTrack--;
        if (const auto error = gme_start_track(mMusicEmu, mCurrentTrack);
            error != nullptr) {

            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GME: %s", error);
            return false;
        }

        parseTrackMetaData();
        return true;
    }

    return false;
}

int GmeDecoder::process(Uint8* stream, const int len) {
    if (mMusicEmu == nullptr) return -1;

    if (const auto error = gme_play(mMusicEmu, len >> 1, (short*) stream);
        error != nullptr) {

        mError = error;
        return -1;
    }

    if (gme_track_ended(mMusicEmu)) {
        if (!nextTrack()) {
            return 1;
        }
    }

    return 0;
}

void GmeDecoder::parseDiskMetaData() {
    if (mMusicEmu == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "GME: Cannot get disk metadata.\n");
        return;
    }

    mMetaData.hasDiskInformation = true;
    mMetaData.diskInformation.trackCount = gme_track_count(mMusicEmu);
    mMetaData.diskInformation.duration = 0;
}

void GmeDecoder::parseTrackMetaData() {
    if (mMusicEmu == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "GME: Cannot get track metadata.\n");
        return;
    }

    gme_info_t* info;
    gme_track_info(mMusicEmu, &info, mCurrentTrack);
    // Maybe two below don't change but I'm not sure
    mMetaData.diskInformation.title = info->game;
    mMetaData.diskInformation.ripper = info->dumper;

    mMetaData.trackInformation.title = info->song;
    mMetaData.trackInformation.author = info->author;
    mMetaData.trackInformation.copyright = info->copyright;
    mMetaData.trackInformation.duration = (info->length > 0 ? info->length : info->play_length) / 1000;
    mMetaData.trackInformation.trackNumber = mCurrentTrack+1;
    mMetaData.trackInformation.comment = info->comment;
    gme_free_info(info);
}
