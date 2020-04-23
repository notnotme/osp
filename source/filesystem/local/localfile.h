#pragma once

#include "../file.h"

#include <filesystem>
#include <vector>

class LocalFile : public File {

    public:
        LocalFile(const std::filesystem::path name);
        virtual ~LocalFile();

        virtual bool getAsBuffer(std::vector<char>& buffer) override;

    private:
        LocalFile(const LocalFile& copy);

};
