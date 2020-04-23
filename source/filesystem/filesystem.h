#pragma once

#include "file.h"

#include <string>
#include <vector>
#include <memory>

class FileSystem {

    public:
        struct Entry {
            bool folder;
            std::string name;
            uintmax_t size;
        };

        FileSystem();
        virtual ~FileSystem();

        virtual bool setup() = 0;
        virtual void cleanup() = 0;
        
        virtual std::string getMountPoint() const = 0;
        virtual bool navigate(const std::string path, std::vector<Entry>& list) = 0;
        virtual std::shared_ptr<File> getFile(const std::string path) const = 0;

        std::string getError() const;
        
    protected:
        std::string mError;

    private:
        FileSystem(const FileSystem& copy);

};