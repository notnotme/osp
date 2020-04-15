#pragma once

#include <filesystem>
#include <vector>

class File {

    public:
        File(const std::filesystem::path path);
        virtual ~File();

        virtual bool getAsBuffer(std::vector<char>& buffer) = 0;

        std::filesystem::path getPath() const;
        std::string getError() const;

    protected:
        std::filesystem::path mPath;
        std::string mError;
        
    private:
        File(const File& copy);

};
