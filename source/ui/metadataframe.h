#pragma once

#include "../decoder/decoder.h"
#include "../soundengine.h"

class MetaDataFrame {

    public:
        struct FrameData {
            SoundEngine::State state;
            Decoder::MetaData metaData;
        };

        MetaDataFrame();
        virtual ~MetaDataFrame();

        void render(const FrameData& frameData);

    private:
        MetaDataFrame(const MetaDataFrame& copy);

        void renderDiskInformation(const FrameData& frameData);
        void renderTrackInformation(const FrameData& frameData);
};
