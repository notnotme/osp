#include "file.h"

File::File(const std::filesystem::path path) :
    mPath(path) {
}

File::~File() {
}

std::filesystem::path File::getPath() const {
    return mPath;
}

std::string File::getError() const {
    return mError;
}
