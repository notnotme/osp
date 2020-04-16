#include "localfile.h"

#include <fstream>
#include <memory>

LocalFile::LocalFile(const std::filesystem::path path) :
    File(path) {
}

LocalFile::~LocalFile() {
}

bool LocalFile::getAsBuffer(std::vector<char>& buffer) {
    const auto path = getPath();

    std::ifstream ifs(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if (!ifs.good()) {
        ifs.close();
        mError = std::string("Unable to open file: ").append(path);
        return false;
    }

    const auto fileSize = ifs.tellg();
    const auto readBuffer = std::unique_ptr<char[]>(new char[fileSize]);

    ifs.seekg(0, std::ios::beg);
    if (!ifs.read(readBuffer.get(), fileSize)) {
        ifs.close();

        mError = std::string("Unable to read file: ").append(path);
        return false;
    }

    // Ensure empty buffer
    buffer.clear();
    buffer.reserve(fileSize);
    buffer.insert(buffer.begin(), readBuffer.get(), readBuffer.get() + fileSize);
    ifs.close();

    return true;
}
