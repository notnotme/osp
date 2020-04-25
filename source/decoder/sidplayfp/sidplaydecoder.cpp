#include "sidplaydecoder.h"

#include "../../app_settings_strings.h"
#include "sidplayfp_settings_strings.h"

#include <fstream>
#include <sidplayfp/SidConfig.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/builders/residfp.h>
#include <sidplayfp/builders/resid.h>
#include <sidplayfp/builders/hardsid.h>
#include <SDL2/SDL_log.h>

const std::string SidPlayDecoder::NAME = "sidplayfp";

SidPlayDecoder::SidPlayDecoder(const std::string dataPath) :
    Decoder(),
    mKernalRom(std::shared_ptr<char[]>(loadRom(std::string(dataPath).append("/kernal").c_str(), 8192))),
    mBasicRom(std::shared_ptr<char[]>(loadRom(std::string(dataPath).append("/basic").c_str(), 8192))),
    mChargenRom(std::shared_ptr<char[]>(loadRom(std::string(dataPath).append("/chargen").c_str(), 4096))),
    mPlayer(nullptr),
    mSIDBuilder(nullptr),
    mTune(nullptr) {

}

SidPlayDecoder::~SidPlayDecoder() {
}

bool SidPlayDecoder::setup() {
    mPlayer = std::unique_ptr<sidplayfp>(new sidplayfp());
    mPlayer->setRoms((const uint8_t*) mKernalRom.get(), (const uint8_t*) mBasicRom.get(), (const uint8_t*) mChargenRom.get());

    return true;
}

void SidPlayDecoder::cleanup() {
    if (mPlayer != nullptr) {
        mPlayer = nullptr;
    }

    if (mTune != nullptr) {
        mTune = nullptr;
    }

    if (mSIDBuilder != nullptr) {
        mSIDBuilder = nullptr;
    }
}

uint8_t SidPlayDecoder::getAudioChannels() const {
    return mPlayer != nullptr ?  mPlayer->config().playback : 0;
}

SDL_AudioFormat SidPlayDecoder::getAudioSampleFormat() const {
    return AUDIO_S16SYS;
}

int SidPlayDecoder::getAudioFrequency() const {
    return mPlayer != nullptr ? mPlayer->config().frequency : 0;
}

const Decoder::MetaData SidPlayDecoder::getMetaData() {
    if (mPlayer == nullptr) {
        return mMetaData;
    }

    mMetaData.trackInformation.position = mPlayer->time();
    return mMetaData;
}

std::string SidPlayDecoder::getName() const {
    return NAME;
}

bool SidPlayDecoder::canRead(const std::string extention) const {
    if(extention == ".sid") { return true; }
    if(extention == ".psid") { return true; }
    if(extention == ".rsid") { return true; }
    if(extention == ".mus") { return true; }
    return false;
}

bool SidPlayDecoder::play(const std::vector<char> buffer, std::shared_ptr<Settings> settings) {

    switch (settings->getInt(KEY_SIDPLAYFP_SID_EMULATION, SIDPLAYFP_SID_EMULATION_DEFAULT)) {
        case 1:
            mSIDBuilder = std::unique_ptr<sidbuilder>(new ReSIDBuilder("OSP"));
            break;
        default:
        case 0:
            mSIDBuilder = std::unique_ptr<sidbuilder>(new ReSIDfpBuilder("OSP"));
            break;
    }

    mSIDBuilder->create(mPlayer->info().maxsids());
    if (!mSIDBuilder->getStatus()) {
        mError = mSIDBuilder->error();
        return false;
    }

    SidConfig cfg;
    cfg.frequency = 48000;
    cfg.samplingMethod = (SidConfig::sampling_method_t) settings->getInt(KEY_SIDPLAYFP_SAMPLING_METHOD, SIDPLAYFP_SAMPLING_METHOD_DEFAULT);
    cfg.fastSampling = settings->getBool(KEY_SIDPLAYFP_FAST_SAMPLING, SIDPLAYFP_FAST_SAMPLING_DEFAULT);
    cfg.digiBoost = settings->getBool(KEY_SIDPLAYFP_ENABLE_DIGIBOOST, SIDPLAYFP_ENABLE_DIGIBOOST_DEFAULT);
    cfg.playback = SidConfig::STEREO;
    cfg.sidEmulation = mSIDBuilder.get();
    
    if (!mPlayer->config(cfg)) {
        mError = mPlayer->error();
        return false;
    }

    if (mTune = std::unique_ptr<SidTune>(new SidTune((const uint_least8_t*) buffer.data(), buffer.size()));
        mTune->getStatus() == false) {
        
        mError = mTune->statusString();
        return false;
    }

    // Load tune into engine
    mTune->selectSong(settings->getBool(KEY_APP_ALWAYS_START_FIRST_TUNE, APP_ALWAYS_START_FIRST_TUNE_DEFAULT) ? 0 : 1);
    if (!mPlayer->load(mTune.get())) {
        mError = mPlayer->error();
        return false;
    }

    mMetaData = MetaData();
    parseDiskMetaData();
    parseTrackMetaData();
    
    return true;
}

void SidPlayDecoder::stop() {
    if (mPlayer != nullptr && mPlayer->isPlaying()) {
        mPlayer->stop();
    }
}

bool SidPlayDecoder::nextTrack() {
    if (mPlayer == nullptr || mTune == nullptr) {
        return false;
    }
    
    if (const auto musicInfo = mTune->getInfo();
        (int) musicInfo->currentSong() < mMetaData.diskInformation.trackCount) {
        
        mPlayer->load(nullptr);
        if (mTune->selectSong(musicInfo->currentSong()+1) == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: %s\n", mTune->statusString());
            return false;
        }

        mPlayer->load(mTune.get());
        parseTrackMetaData();
        return true;
    }

    return false;
}

bool SidPlayDecoder::prevTrack() {
    if (mPlayer == nullptr || mTune == nullptr) {
        return false;
    }

    if (const auto musicInfo = mTune->getInfo();
        musicInfo->currentSong() > 1) {

        mPlayer->load(nullptr);
        if (mTune->selectSong(musicInfo->currentSong()-1) == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: %s\n", mTune->statusString());
            return false;
        }

        mPlayer->load(mTune.get());
        parseTrackMetaData();
        return true;
    }

    return false;
}

int SidPlayDecoder::process(Uint8* stream, const int len) {
    if (mPlayer == nullptr) return -1;

    unsigned int sz = len >> 1;
    unsigned int played = mPlayer->play((short*) stream, sz);

    if(played < sz && mPlayer->isPlaying()) {
        mError = mPlayer->error();
        return -1;
    }
    else if (played < sz) {
        return 1;
    }

    // doesn't update song number
    //const auto musicInfo = mTune->getInfo();
    //mMetaData.trackInformation.trackNumber = musicInfo->currentSong();

    return 0;
}

void SidPlayDecoder::parseDiskMetaData() {
    if (mTune == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: Cannot get track metadata.\n");
        return;
    }

    const auto musicInfo = mTune->getInfo();
    mMetaData.hasDiskInformation = musicInfo->songs() >= 1;
    mMetaData.diskInformation.trackCount = musicInfo->songs();
}

void SidPlayDecoder::parseTrackMetaData() {
    if (mTune == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: Cannot get track metadata.\n");
        return;
    }

    const auto musicInfo = mTune->getInfo();
    mMetaData.trackInformation.title = musicInfo->infoString(0);
    mMetaData.trackInformation.author = musicInfo->infoString(1);
    mMetaData.trackInformation.copyright = musicInfo->infoString(2);
    mMetaData.trackInformation.duration = 0; // todo: how to get it ?
    mMetaData.trackInformation.trackNumber = musicInfo->currentSong();
}

char* SidPlayDecoder::loadRom(const std::string path, const size_t romSize) {
    char* buffer = nullptr;
    std::ifstream is(path, std::ios::binary);
    if (is.good()) {
        buffer = new char[romSize];
        is.read(buffer, romSize);
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "SIDPLAYFP: %s\n", path.c_str());
    }
    is.close();
    return buffer;
}
