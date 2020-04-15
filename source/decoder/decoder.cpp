#include "decoder.h"

Decoder::Decoder() {
}

Decoder::~Decoder() {
}

std::string Decoder::getError() const {
    return mError;
}

bool Decoder::nextTrack() {
    return false;
}

bool Decoder::prevTrack() {
    return false;
}
