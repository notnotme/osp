#include "localfile.h"

#include <fstream>
#include <memory>

LocalFile::LocalFile(const std::filesystem::path path) :
    File(path) {
}

LocalFile::~LocalFile() {
}

bool LocalFile::getAsBuffer(std::vector<char>& buffer) {
    std::ifstream ifs(mPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!ifs.good()) {
        ifs.close();
        mError = mPath;
        return false;
    }

    const auto fileSize = ifs.tellg();
    const auto readBuffer = std::unique_ptr<char[]>(new char[fileSize]);

    ifs.seekg(0, std::ios::beg);
    if (!ifs.read(readBuffer.get(), fileSize)) {
        ifs.close();

        mError = mPath;
        return false;
    }

    // Ensure empty buffer
    buffer.clear();
    buffer.reserve(fileSize);
    buffer.insert(buffer.begin(), readBuffer.get(), readBuffer.get() + fileSize);
    ifs.close();

    return true;
}
