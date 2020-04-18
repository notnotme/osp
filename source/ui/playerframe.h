#pragma once

#include "../decoder/decoder.h"
#include "../soundengine.h"

#include <string>
#include <functional>

class PlayerFrame {

    public:
        struct FrameData {
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
