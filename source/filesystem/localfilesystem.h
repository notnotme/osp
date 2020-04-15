#pragma once

#include "file.h"
#include "filesystem.h"

#include <string>
#include <vector>

class LocalFileSystem : public FileSystem {

    public:
        LocalFileSystem(const std::string mountPoint);
        virtual ~LocalFileSystem();

        virtual bool setup() override;
        virtual void cleanup() override;
        
        virtual std::string getMountPoint() const override;
        virtual bool navigate(const std::string path, std::vector<Entry>& list) override;
        virtual File* getFile(const std::string path) const override;

    private:
        const std::string mMountPoint;
        
        LocalFileSystem(const LocalFileSystem& copy);

};
