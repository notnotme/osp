#include "dumbdecoder.h"

#include "dumb_settings_strings.h"

#include <memory>
#include <algorithm>

const std::string DumbDecoder::NAME = "dumb";

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
        return mMetaData;
    }

    mMetaData.trackInformation.position = (int) (duh_sigrenderer_get_position(mSigRenderer) / 65536);
    return mMetaData;
}

int DumbDecoder::getAudioFrequency() const {
    return dumb_resampling_quality;
}

std::string DumbDecoder::getName() const {
    return NAME;
}

bool DumbDecoder::canRead(const std::string extention) const {
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

bool DumbDecoder::play(const std::vector<char> buffer, std::shared_ptr<Settings> settings) {

    switch(settings->getInt(KEY_DUMB_MAX_TO_MIX, DUMB_MAX_TO_MIX_DEFAULT)) {
        case 0:
            dumb_it_max_to_mix = 64;
            break;
        case 1:
            dumb_it_max_to_mix = 128;
            break;
        case 2:
            dumb_it_max_to_mix = 256;
            break;
        case 3:
            dumb_it_max_to_mix = 512;
            break;
        default:
            dumb_it_max_to_mix = 64;
    }

    if (mDumbFile = dumbfile_open_memory(buffer.data(), buffer.size());
        mDumbFile == nullptr) {
        
        mError = "Can't open file.";
        return false;
    }
    
    if (mDuh = dumb_read_any(mDumbFile, 0, 0);
        mDuh == nullptr) {
        
        mError = "Unknown format.";
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

    // Get track comment
    const auto data = duh_get_it_sigdata(mDuh);
    if (const auto commentPtr = (char*) dumb_it_sd_get_song_message(data);
        commentPtr != nullptr) {

        if (auto comment = std::string(commentPtr);
            !comment.empty()) {
            
            std::replace(comment.begin(), comment.end(), '\r', '\n');
            mMetaData.trackInformation.comment = comment;
        }
    }

    // Seek instruments to show others comments
    const auto nbInstruments = dumb_it_sd_get_n_instruments(data);
    if (nbInstruments > 0 && !mMetaData.trackInformation.comment.empty()) {
        mMetaData.trackInformation.comment.append("\n");
    }

    for (auto i=0; i<nbInstruments; i++) {
        if (const auto instrumentNamePtr = (char*) dumb_it_sd_get_instrument_name(data, i);
            instrumentNamePtr != nullptr) {
             
            if (const auto comment = std::string(instrumentNamePtr);
                !comment.empty()) {

                mMetaData.trackInformation.comment.append(comment).append("\n");
            }
        }
    }

     // Seek samples to show others comments
    const auto nbSamples = dumb_it_sd_get_n_samples(data);
    if (nbSamples > 0 && !mMetaData.trackInformation.comment.empty()) {
        mMetaData.trackInformation.comment.append("\n");
    }

    for (auto i=0; i<nbSamples; i++) {
        if (const auto sampleNamePtr = (char*) dumb_it_sd_get_sample_name(data, i);
            sampleNamePtr != nullptr) {
             
            if (const auto comment = std::string(sampleNamePtr);
                !comment.empty()) {

                mMetaData.trackInformation.comment.append(comment).append("\n");
            }
        }
    }

    mMetaData.trackInformation.duration = (int) (duh_get_length(mDuh) / 65536);
}
