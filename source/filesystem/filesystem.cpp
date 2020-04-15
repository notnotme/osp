#include "filesystem.h"

FileSystem::FileSystem() {
}

FileSystem::~FileSystem() {
}

std::string FileSystem::getError() const {
    return mError;
}
