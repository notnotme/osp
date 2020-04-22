#pragma once

#include <glm/glm.hpp>
#include <map>
#include <string>

class SpriteCatalog {

    public:
        struct Frame {
            glm::vec2 size;
            glm::vec2 uv0;
            glm::vec2 uv1;
        };

        SpriteCatalog();
        virtual ~SpriteCatalog();

        virtual bool setup(std::string jsonString);
        virtual void cleanup();
        const Frame getFrame(const std::string name);
        std::string getError() const;

    private:
        SpriteCatalog(const SpriteCatalog& copy);
        std::string mError;
        std::map<std::string, Frame> mCatalog;

};
