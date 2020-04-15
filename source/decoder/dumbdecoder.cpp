#include "dumbdecoder.h"

#include <memory>
#include <algorithm>
#include <SDL2/SDL_log.h>

DumbDecoder::DumbDecoder() :
    Decoder(),
    mDuh(nullptr),
    mDumbFile(nullptr),
    mSigRenderer(nullptr) {
}

DumbDecoder::~DumbDecoder() {
}

bool DumbDecoder::setup() {
    dumb_resampling_quality = 48000;
    return true;
}

void DumbDecoder::cleanup() {
    dumb_exit();
}

uint8_t DumbDecoder::getAudioChannels() const {
    return 2;
}

SDL_AudioFormat DumbDecoder::getAudioSampleFormat() const {
    return AUDIO_S16SYS;
}

const Decoder::MetaData DumbDecoder::getMetaData() {
    if (mSigRenderer == nullptr) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Unable to get song information.\n");
        return mMetaData;
    }

     return mMetaData;
}

int DumbDecoder::getAudioFrequency() const {
    return dumb_resampling_quality;
}

std::string DumbDecoder::getName() {
    return "dumb";
}


bool DumbDecoder::canRead(const std::string extention) {
    if(extention == ".it") { return true; }
    if(extention == ".xm") { return true; }
    if(extention == ".mod") { return true; }
    if(extention == ".stm") { return true; }
    if(extention == ".s3m") { return true; }
    if(extention == ".669") { return true; }
    if(extention == ".amf") { return true; }
    if(extention == ".dsm") { return true; }
    if(extention == ".mtm") { return true; }
    if(extention == ".okt") { return true; }
    if(extention == ".psm") { return true; }
    if(extention == ".ptm") { return true; }
    if(extention == ".riff") { return true; }
    return false;
}

bool DumbDecoder::play(const std::vector<char> buffer) {
	mDumbFile = dumbfile_open_memory(buffer.data(), buffer.size());
	if (mDumbFile == nullptr) {
        mError = "Dumb cannot open file.";
        return false;
    }
    
    mDuh = dumb_read_any(mDumbFile, 0, 0);
    if (mDuh == nullptr) {
        mError = "Dumb cannot read file.";
        return false;
    }

    mMetaData = MetaData();
    parseMetaData();
	mSigRenderer = duh_start_sigrenderer(mDuh, 0, 2, 0);

    return true;
}

void DumbDecoder::stop() {
    if (mSigRenderer != nullptr) {
        duh_end_sigrenderer(mSigRenderer);
        mSigRenderer = nullptr;
    }

    if (mDuh != nullptr) {
        unload_duh(mDuh);
        mDuh = nullptr;
    }

    if (mDumbFile != nullptr) {
        dumbfile_close(mDumbFile);
        mDumbFile = nullptr;
    }
}

int DumbDecoder::process(Uint8* stream, const int len) {
    if (mSigRenderer == nullptr) return -1;

    const auto toRender = len >> 2;
    const auto rendered = duh_render(mSigRenderer, 16, 0, 1.0f, 65536.0f / 48000.0f, toRender, stream);
    if (rendered != toRender) {
        return 1;
    }

    return 0;
}

void DumbDecoder::parseMetaData() {
    mMetaData.hasDiskInformation = false;
    if (duh_get_tag_iterator_size(mDuh) >= 1) {
        mMetaData.trackInformation.title = duh_get_tag(mDuh, "TITLE");
    }

    auto data = duh_get_it_sigdata(mDuh);
    auto messagePtr = (char*) dumb_it_sd_get_song_message(data);
    if (messagePtr != nullptr) {
        auto message = std::string(messagePtr);
        if (!message.empty()) {
            std::replace(message.begin(), message.end(), '\r', '\n');
            mMetaData.trackInformation.comment = message;
        }
    }

    mMetaData.trackInformation.duration = (int) (duh_get_length(mDuh) / 65536);
}
