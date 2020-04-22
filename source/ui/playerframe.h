#pragma once

#include "../decoder/decoder.h"
#include "../soundengine.h"
#include "../spritecatalog.h"

#include <string>
#include <glad/glad.h>
#include <functional>

class PlayerFrame {

    public:
        struct FrameData {
            GLuint texture;
            SpriteCatalog::Frame playFrame;
            SpriteCatalog::Frame pauseFrame;
            SpriteCatalog::Frame stopFrame;
            SpriteCatalog::Frame nextFrame;
            SpriteCatalog::Frame prevFrame;
            SoundEngine::State state;
            Decoder::MetaData metaData;
        };

        enum ButtonId {
            PLAY,
            STOP,
            PREV,
            NEXT
        };

        PlayerFrame();
        virtual ~PlayerFrame();

        void render(const FrameData& frameData,
            std::function<void (ButtonId)> onButtonClick);

    private:
        PlayerFrame(const PlayerFrame& copy);

        void renderTitleAndSeekBar(const FrameData& frameData);
        void renderButtonBar(const FrameData& frameData, bool disabled,
            std::function<void (ButtonId)> onButtonClick);

};
