#pragma once

#include "../../decoder/decoder.h"
#include "../../soundengine.h"
#include "../../spritecatalog.h"

#include <string>
#include <glad/glad.h>
#include <functional>
#include <memory>

class PlayerFrame {

    public:
        struct FrameData {
            GLuint texture;
            SoundEngine::State state;
            Decoder::MetaData metaData;
            std::shared_ptr<SpriteCatalog> catalog;
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
            const std::function<void (ButtonId)>& onButtonClick);

    private:
        PlayerFrame(const PlayerFrame& copy);

        void renderTitleAndSeekBar(const FrameData& frameData);
        void renderButtonBar(const FrameData& frameData, bool disabled,
            const std::function<void (ButtonId)>& onButtonClick);

};
