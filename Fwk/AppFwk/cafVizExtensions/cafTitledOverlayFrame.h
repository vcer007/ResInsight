#pragma once
#include "cvfBase.h"
#include "cvfColor3.h"
#include "cvfColor4.h"
#include "cvfOverlayItem.h"
#include "cvfString.h"

namespace cvf
{
    class Font;    
}

namespace caf {
    //==================================================================================================
    // Base class for ApplicationFwk Legends.
    // Abstract because the pure virtual render methods from cvf::OverlayItem are not implemented.
    //==================================================================================================
    class TitledOverlayFrame : public cvf::OverlayItem
    {
    public:
        TitledOverlayFrame(cvf::Font* font, unsigned int width = 100, unsigned int height = 200);
        virtual ~TitledOverlayFrame();

        void                 setRenderSize(const cvf::Vec2ui& size);
        cvf::Vec2ui          renderSize() const;

        void                 setTextColor(const cvf::Color3f& color);
        void                 setLineColor(const cvf::Color3f& lineColor);
        void                 setLineWidth(int lineWidth);

        void                 setTitle(const cvf::String& title);

        void                 enableBackground(bool enable);
        void                 setBackgroundColor(const cvf::Color4f& backgroundColor);
        void                 setBackgroundFrameColor(const cvf::Color4f& backgroundFrameColor);

        virtual cvf::Vec2ui  preferredSize() = 0;
        
    protected:
        cvf::Color3f              textColor() const;
        cvf::Color3f              lineColor() const;
        int                       lineWidth() const;

        bool                      backgroundEnabled() const;
        cvf::Color4f              backgroundColor() const;
        cvf::Color4f              backgroundFrameColor() const;
        std::vector<cvf::String>& titleStrings();
        cvf::Font*                font();

    private:
        cvf::Vec2ui               sizeHint() override final; // Will return the size to use for rendering, and is really not a hint.
        cvf::Vec2ui               m_renderSize;          // The rendered size of the color legend area in pixels

        cvf::Color3f              m_textColor;
        cvf::Color3f              m_lineColor;
        int                       m_lineWidth;
        
        bool                      m_isBackgroundEnabled;
        cvf::Color4f              m_backgroundColor;
        cvf::Color4f              m_backgroundFrameColor;

        std::vector<cvf::String>  m_titleStrings;
        cvf::ref<cvf::Font>       m_font;
    };
}