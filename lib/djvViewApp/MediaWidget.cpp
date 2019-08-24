//------------------------------------------------------------------------------
// Copyright (c) 2004-2019 Darby Johnston
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions, and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions, and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the names of the copyright holders nor the names of any
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//------------------------------------------------------------------------------

#include <djvViewApp/MediaWidget.h>

#include <djvViewApp/FileSystem.h>
#include <djvViewApp/ImageSystem.h>
#include <djvViewApp/ImageView.h>
#include <djvViewApp/ImageViewSettings.h>
#include <djvViewApp/Media.h>
#include <djvViewApp/MediaWidgetPrivate.h>
#include <djvViewApp/TimelineSlider.h>
#include <djvViewApp/WindowSystem.h>

#include <djvUI/Action.h>
#include <djvUI/ActionGroup.h>
#include <djvUI/Border.h>
#include <djvUI/ButtonGroup.h>
#include <djvUI/FloatSlider.h>
#include <djvUI/GridLayout.h>
#include <djvUI/IntEdit.h>
#include <djvUI/IntSlider.h>
#include <djvUI/Label.h>
#include <djvUI/LineEdit.h>
#include <djvUI/ListButton.h>
#include <djvUI/MDICanvas.h>
#include <djvUI/MDIWidget.h>
#include <djvUI/PopupWidget.h>
#include <djvUI/RowLayout.h>
#include <djvUI/SettingsSystem.h>
#include <djvUI/StackLayout.h>
#include <djvUI/ToggleButton.h>
#include <djvUI/ToolBar.h>
#include <djvUI/ToolButton.h>

#include <djvAV/AVSystem.h>

#include <djvCore/Context.h>
#include <djvCore/NumericValueModels.h>
#include <djvCore/Path.h>
#include <djvCore/Timer.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        PointerData::PointerData()
        {}

        PointerData::PointerData(PointerState state, const glm::vec2& pos, const std::map<int, bool>& buttons) :
            state(state),
            pos(pos),
            buttons(buttons)
        {}

        bool PointerData::operator == (const PointerData& other) const
        {
            return state == other.state && pos == other.pos;
        }

        struct MediaWidget::Private
        {
            std::shared_ptr<ValueSubject<PointerData> > hover;
            std::shared_ptr<ValueSubject<PointerData> > drag;

            std::shared_ptr<Media> media;
            AV::IO::Info ioInfo;
            std::shared_ptr<AV::Image::Image> image;
            std::vector<Time::Speed> speeds;
            Time::Speed defaultSpeed;
            Time::Speed speed;
            float realSpeed = 0.f;
            bool playEveryFrame = false;
            Frame::Sequence sequence;
            Frame::Number currentFrame = Frame::invalid;
            Playback playbackPrev = Playback::Count;
            AV::TimeUnits timeUnits = AV::TimeUnits::First;
            ImageViewLock viewLock = ImageViewLock::First;
            bool frameStoreEnabled = false;
            std::shared_ptr<AV::Image::Image> frameStore;
            bool audioEnabled = false;
            float audioVolume = 0.f;
            bool audioMute = false;
            bool active = false;
            float fade = 1.f;

            std::map<std::string, std::shared_ptr<UI::Action> > actions;
            std::shared_ptr<UI::ActionGroup> playbackActionGroup;

            std::shared_ptr<UI::Label> titleLabel;
            std::shared_ptr<UI::ToolButton> maximizeButton;
            std::shared_ptr<UI::ToolButton> closeButton;
            std::shared_ptr<UI::HorizontalLayout> titleBar;
            std::shared_ptr<PointerWidget> pointerWidget;
            std::shared_ptr<ImageView> imageView;
            std::shared_ptr<UI::ButtonGroup> speedButtonGroup;
            std::shared_ptr<UI::VerticalLayout> speedButtonLayout;
            std::shared_ptr<UI::ToggleButton> playEveryFrameButton;
            std::shared_ptr<UI::Label> playEveryFrameLabel;
            std::shared_ptr<UI::PopupWidget> speedPopupWidget;
            std::shared_ptr<UI::Label> realSpeedLabel;
            std::shared_ptr<CurrentFrameWidget> currentFrameWidget;
            std::shared_ptr<UI::Label> durationLabel;
            std::shared_ptr<TimelineSlider> timelineSlider;
            std::shared_ptr<UI::BasicFloatSlider> audioVolumeSlider;
            std::shared_ptr<UI::ToolButton> audioMuteButton;
            std::shared_ptr<UI::PopupWidget> audioPopupWidget;
            std::shared_ptr<UI::GridLayout> playbackLayout;
            std::shared_ptr<UI::StackLayout> layout;

            std::map<std::string, std::shared_ptr<ValueObserver<bool> > > actionObservers;
            std::shared_ptr<ValueObserver<AV::IO::Info> > ioInfoObserver;
            std::shared_ptr<ValueObserver<Frame::Number> > currentFrameObserver;
            std::shared_ptr<ValueObserver<bool> > currentFrameChangeObserver;
            std::shared_ptr<ValueObserver<AV::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ValueObserver<std::shared_ptr<AV::Image::Image> > > imageObserver;
            std::shared_ptr<ValueObserver<Time::Speed> > speedObserver;
            std::shared_ptr<ValueObserver<Time::Speed> > defaultSpeedObserver;
            std::shared_ptr<ValueObserver<float> > realSpeedObserver;
            std::shared_ptr<ValueObserver<bool> > playEveryFrameObserver;
            std::shared_ptr<ValueObserver<Frame::Sequence> > sequenceObserver;
            std::shared_ptr<ValueObserver<Frame::Number> > currentFrameObserver2;
            std::shared_ptr<ValueObserver<Playback> > playbackObserver;
            std::shared_ptr<ValueObserver<bool> > audioEnabledObserver;
            std::shared_ptr<ValueObserver<float> > volumeObserver;
            std::shared_ptr<ValueObserver<bool> > muteObserver;
            std::shared_ptr<ListObserver<Frame::Range> > cachedFramesObserver;
            std::shared_ptr<ValueObserver<float> > fadeObserver;
            std::shared_ptr<ValueObserver<ImageViewLock> > viewLockObserver;
            std::shared_ptr<ValueObserver<bool> > frameStoreEnabledObserver;
            std::shared_ptr<ValueObserver<std::shared_ptr<AV::Image::Image> > > frameStoreObserver;
        };

        void MediaWidget::_init(const std::shared_ptr<Media>& media, const std::shared_ptr<Context>& context)
        {
            IWidget::_init(context);

            DJV_PRIVATE_PTR();
            setClassName("djv::ViewApp::MediaWidget");

            p.hover = ValueSubject<PointerData>::create();
            p.drag = ValueSubject<PointerData>::create();

            p.media = media;

            p.actions["Forward"] = UI::Action::create();
            p.actions["Forward"]->setIcon("djvIconPlaybackForward");
            p.actions["Forward"]->setCheckedIcon("djvIconPlaybackStop");
            p.actions["Reverse"] = UI::Action::create();
            p.actions["Reverse"]->setIcon("djvIconPlaybackReverse");
            p.actions["Reverse"]->setCheckedIcon("djvIconPlaybackStop");
            p.playbackActionGroup = UI::ActionGroup::create(UI::ButtonType::Exclusive);
            p.playbackActionGroup->addAction(p.actions["Forward"]);
            p.playbackActionGroup->addAction(p.actions["Reverse"]);

            p.actions["InPoint"] = UI::Action::create();
            p.actions["InPoint"]->setIcon("djvIconFrameStart");
            p.actions["PrevFrame"] = UI::Action::create();
            p.actions["PrevFrame"]->setIcon("djvIconFramePrev");
            p.actions["NextFrame"] = UI::Action::create();
            p.actions["NextFrame"]->setIcon("djvIconFrameNext");
            p.actions["OutPoint"] = UI::Action::create();
            p.actions["OutPoint"]->setIcon("djvIconFrameEnd");

            p.titleLabel = UI::Label::create(context);
            p.titleLabel->setText(media->getFileInfo().getFileName(Frame::invalid, false));
            p.titleLabel->setTextHAlign(UI::TextHAlign::Left);
            p.titleLabel->setMargin(UI::Layout::Margin(UI::MetricsRole::Margin, UI::MetricsRole::Margin, UI::MetricsRole::None, UI::MetricsRole::None));
            p.titleLabel->setTooltip(media->getFileInfo());

            p.maximizeButton = UI::ToolButton::create(context);
            p.maximizeButton->setIcon("djvIconSDI");
            p.maximizeButton->setInsideMargin(UI::MetricsRole::MarginSmall);

            p.closeButton = UI::ToolButton::create(context);
            p.closeButton->setIcon("djvIconClose");
            p.closeButton->setInsideMargin(UI::MetricsRole::MarginSmall);

            p.titleBar = UI::HorizontalLayout::create(context);
            p.titleBar->setSpacing(UI::MetricsRole::None);
            p.titleBar->setBackgroundRole(UI::ColorRole::OverlayLight);
            p.titleBar->addChild(p.titleLabel);
            p.titleBar->setStretch(p.titleLabel, UI::RowStretch::Expand);
            p.titleBar->addChild(p.maximizeButton);
            p.titleBar->addChild(p.closeButton);

            p.pointerWidget = PointerWidget::create(context);

            p.imageView = ImageView::create(context);

            p.speedButtonGroup = UI::ButtonGroup::create(UI::ButtonType::Push);
            p.speedButtonLayout = UI::VerticalLayout::create(context);
            p.speedButtonLayout->setSpacing(UI::MetricsRole::None);
            p.playEveryFrameButton = UI::ToggleButton::create(context);
            p.playEveryFrameLabel = UI::Label::create(context);
            auto vLayout = UI::VerticalLayout::create(context);
            vLayout->setSpacing(UI::MetricsRole::None);
            vLayout->addChild(p.speedButtonLayout);
            vLayout->addSeparator();
            auto hLayout = UI::HorizontalLayout::create(context);
            hLayout->setMargin(UI::MetricsRole::MarginSmall);
            hLayout->setSpacing(UI::MetricsRole::None);
            hLayout->addChild(p.playEveryFrameLabel);
            hLayout->addChild(p.playEveryFrameButton);
            vLayout->addChild(hLayout);
            p.speedPopupWidget = UI::PopupWidget::create(context);
            p.speedPopupWidget->addChild(vLayout);
            p.realSpeedLabel = UI::Label::create(context);

            p.currentFrameWidget = CurrentFrameWidget::create(context);

            p.durationLabel = UI::Label::create(context);
            p.durationLabel->setMargin(UI::MetricsRole::MarginSmall);

            p.timelineSlider = TimelineSlider::create(context);
            p.timelineSlider->setMedia(p.media);

            p.audioVolumeSlider = UI::BasicFloatSlider::create(UI::Orientation::Horizontal, context);
            p.audioVolumeSlider->setMargin(UI::MetricsRole::MarginSmall);
            p.audioMuteButton = UI::ToolButton::create(context);
            p.audioMuteButton->setIcon("djvIconAudioMute");
            p.audioMuteButton->setButtonType(UI::ButtonType::Toggle);
            hLayout = UI::HorizontalLayout::create(context);
            hLayout->setSpacing(UI::MetricsRole::None);
            hLayout->addChild(p.audioVolumeSlider);
            hLayout->addChild(p.audioMuteButton);
            p.audioPopupWidget = UI::PopupWidget::create(context);
            p.audioPopupWidget->addChild(hLayout);

            auto toolbar = UI::ToolBar::create(context);
            toolbar->setBackgroundRole(UI::ColorRole::None);
            toolbar->addAction(p.actions["InPoint"]);
            toolbar->addAction(p.actions["PrevFrame"]);
            toolbar->addAction(p.actions["Reverse"]);
            toolbar->addAction(p.actions["Forward"]);
            toolbar->addAction(p.actions["NextFrame"]);
            toolbar->addAction(p.actions["OutPoint"]);

            p.playbackLayout = UI::GridLayout::create(context);
            p.playbackLayout->setBackgroundRole(UI::ColorRole::OverlayLight);
            p.playbackLayout->setSpacing(UI::MetricsRole::None);
            p.playbackLayout->addChild(toolbar);
            p.playbackLayout->setGridPos(toolbar, 0, 0);
            p.playbackLayout->addChild(p.timelineSlider);
            p.playbackLayout->setGridPos(p.timelineSlider, 1, 0);
            p.playbackLayout->setStretch(p.timelineSlider, UI::GridStretch::Horizontal);
            p.playbackLayout->addChild(p.audioPopupWidget);
            p.playbackLayout->setGridPos(p.audioPopupWidget, 2, 0);
            hLayout = UI::HorizontalLayout::create(context);
            hLayout->addChild(p.speedPopupWidget);
            hLayout->addChild(p.realSpeedLabel);
            p.playbackLayout->addChild(hLayout);
            p.playbackLayout->setGridPos(hLayout, 0, 1);
            hLayout = UI::HorizontalLayout::create(context);
            hLayout->setSpacing(UI::MetricsRole::None);
            hLayout->addChild(p.currentFrameWidget);
            hLayout->addExpander();
            hLayout->addChild(p.durationLabel);
            p.playbackLayout->addChild(hLayout);
            p.playbackLayout->setGridPos(hLayout, 1, 1);

            p.layout = UI::StackLayout::create(context);
            p.layout->setBackgroundRole(UI::ColorRole::OverlayLight);
            p.layout->addChild(p.imageView);
            p.layout->addChild(p.pointerWidget);
            vLayout = UI::VerticalLayout::create(context);
            vLayout->addChild(p.titleBar);
            vLayout->addExpander();
            vLayout->addChild(p.playbackLayout);
            p.layout->addChild(vLayout);
            addChild(p.layout);

            _widgetUpdate();
            _speedUpdate();
            _realSpeedUpdate();
            _audioUpdate();
            _opacityUpdate();

            auto weak = std::weak_ptr<MediaWidget>(std::dynamic_pointer_cast<MediaWidget>(shared_from_this()));
            p.playbackActionGroup->setExclusiveCallback(
                [weak](int index)
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto media = widget->_p->media)
                        {
                            Playback playback = Playback::Stop;
                            switch (index)
                            {
                            case 0: playback = Playback::Forward; break;
                            case 1: playback = Playback::Reverse; break;
                            }
                            media->setPlayback(playback);
                        }
                    }
                });

            p.pointerWidget->setHoverCallback(
                [weak](const PointerData& data)
                {
                    if (auto widget = weak.lock())
                    {
                        const BBox2f& g = widget->_p->imageView->getGeometry();
                        widget->_p->hover->setIfChanged(PointerData(data.state, data.pos - g.min, data.buttons));
                    }
                });

            p.pointerWidget->setDragCallback(
                [weak](const PointerData& data)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->moveToFront();
                        const BBox2f& g = widget->_p->imageView->getGeometry();
                        widget->_p->drag->setIfChanged(PointerData(data.state, data.pos - g.min, data.buttons));
                    }
                });

            p.speedButtonGroup->setPushCallback(
                [weak](int value)
            {
                if (auto widget = weak.lock())
                {
                    if (auto media = widget->_p->media)
                    {
                        const auto speed =
                            value >= 0 && value < widget->_p->speeds.size() ?
                            widget->_p->speeds[value] :
                            widget->_p->defaultSpeed;
                        media->setSpeed(speed);
                    }
                    widget->_p->speedPopupWidget->close();
                }
            });

            p.playEveryFrameButton->setCheckedCallback(
                [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto media = widget->_p->media)
                        {
                            media->setPlayEveryFrame(value);
                        }
                    }
                });

            p.currentFrameWidget->setCallback(
                [weak](Frame::Index value)
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto media = widget->_p->media)
                        {
                            media->setCurrentFrame(value);
                        }
                    }
                });

            p.audioVolumeSlider->setValueCallback(
                [weak](float value)
            {
                if (auto widget = weak.lock())
                {
                    if (auto media = widget->_p->media)
                    {
                        media->setVolume(value);
                    }
                }
            });

            p.audioMuteButton->setCheckedCallback(
                [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto media = widget->_p->media)
                        {
                            media->setMute(value);
                        }
                    }
                });

            p.maximizeButton->setClickedCallback(
                [weak]
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto canvas = std::dynamic_pointer_cast<UI::MDI::Canvas>(widget->getParent().lock()))
                        {
                            widget->moveToFront();
                            canvas->setMaximize(!canvas->isMaximized());
                        }
                    }
                });

            auto contextWeak = std::weak_ptr<Context>(context);
            p.closeButton->setClickedCallback(
                [media, contextWeak]
                {
                    if (auto context = contextWeak.lock())
                    {
                        auto fileSystem = context->getSystemT<FileSystem>();
                        fileSystem->close(media);
                    }
                });

            p.actionObservers["InPoint"] = ValueObserver<bool>::create(
                p.actions["InPoint"]->observeClicked(),
                [weak](bool value)
                {
                    if (value)
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto media = widget->_p->media)
                            {
                                media->inPoint();
                            }
                        }
                    }
                });

            p.actionObservers["PrevFrame"] = ValueObserver<bool>::create(
                p.actions["PrevFrame"]->observeClicked(),
                [weak](bool value)
                {
                    if (value)
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto media = widget->_p->media)
                            {
                                media->prevFrame();
                            }
                        }
                    }
                });

            p.actionObservers["NextFrame"] = ValueObserver<bool>::create(
                p.actions["NextFrame"]->observeClicked(),
                [weak](bool value)
                {
                    if (value)
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto media = widget->_p->media)
                            {
                                media->nextFrame();
                            }
                        }
                    }
                });

            p.actionObservers["OutPoint"] = ValueObserver<bool>::create(
                p.actions["OutPoint"]->observeClicked(),
                [weak](bool value)
                {
                    if (value)
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto media = widget->_p->media)
                            {
                                media->outPoint();
                            }
                        }
                    }
                });

            p.currentFrameObserver = ValueObserver<Frame::Number>::create(
                p.timelineSlider->observeCurrentFrame(),
                [weak](Frame::Number value)
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto media = widget->_p->media)
                        {
                            media->setCurrentFrame(value);
                        }
                    }
                });

            p.currentFrameChangeObserver = ValueObserver<bool>::create(
                p.timelineSlider->observeCurrentFrameChange(),
                [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto media = widget->_p->media)
                        {
                            if (value)
                            {
                                widget->_p->playbackPrev = media->observePlayback()->get();
                            }
                            else if (widget->_p->playbackPrev != Playback::Count)
                            {
                                media->setPlayback(widget->_p->playbackPrev);
                            }
                        }
                    }
                });

            auto avSystem = context->getSystemT<AV::AVSystem>();
            p.timeUnitsObserver = ValueObserver<AV::TimeUnits>::create(
                avSystem->observeTimeUnits(),
                [weak](AV::TimeUnits value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->timeUnits = value;
                        widget->_widgetUpdate();
                    }
                });

            p.ioInfoObserver = ValueObserver<AV::IO::Info>::create(
                p.media->observeInfo(),
                [weak](const AV::IO::Info& value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->ioInfo = value;
                        widget->_widgetUpdate();
                        widget->_audioUpdate();
                    }
                });

            p.imageObserver = ValueObserver<std::shared_ptr<AV::Image::Image> >::create(
                p.media->observeCurrentImage(),
                [weak](const std::shared_ptr<AV::Image::Image>& value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->image = value;
                        widget->_imageUpdate();
                    }
                });

            p.speedObserver = ValueObserver<Time::Speed>::create(
                p.media->observeSpeed(),
                [weak](const Time::Speed& value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->speed = value;
                        widget->_widgetUpdate();
                        widget->_speedUpdate();
                    }
                });

            p.defaultSpeedObserver = ValueObserver<Time::Speed>::create(
                p.media->observeDefaultSpeed(),
                [weak](const Time::Speed& value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->defaultSpeed = value;
                        widget->_widgetUpdate();
                        widget->_speedUpdate();
                    }
                });

            p.realSpeedObserver = ValueObserver<float>::create(
                p.media->observeRealSpeed(),
                [weak](float value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->realSpeed = value;
                        widget->_realSpeedUpdate();
                    }
                });

            p.playEveryFrameObserver = ValueObserver<bool>::create(
                p.media->observePlayEveryFrame(),
                [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->playEveryFrame = value;
                        widget->_widgetUpdate();
                        widget->_speedUpdate();
                    }
                });

            p.sequenceObserver = ValueObserver<Frame::Sequence>::create(
                p.media->observeSequence(),
                [weak](const Frame::Sequence& value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->sequence = value;
                        widget->_widgetUpdate();
                    }
                });

            p.currentFrameObserver2 = ValueObserver<Frame::Number>::create(
                p.media->observeCurrentFrame(),
                [weak](Frame::Number value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->currentFrame = value;
                        widget->_widgetUpdate();
                    }
                });

            p.playbackObserver = ValueObserver<Playback>::create(
                p.media->observePlayback(),
                [weak](Playback value)
                {
                    if (auto widget = weak.lock())
                    {
                        int index = -1;
                        switch (value)
                        {
                        case Playback::Forward: index = 0; break;
                        case Playback::Reverse: index = 1; break;
                        default: break;
                        }
                        widget->_p->playbackActionGroup->setChecked(index);
                    }
                });

            p.audioEnabledObserver = ValueObserver<bool>::create(
                p.media->observeAudioEnabled(),
                [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->audioEnabled = value;
                        widget->_audioUpdate();
                    }
                });

            p.volumeObserver = ValueObserver<float>::create(
                p.media->observeVolume(),
                [weak](float value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->audioVolume = value;
                        widget->_audioUpdate();
                    }
                });

            p.muteObserver = ValueObserver<bool>::create(
                p.media->observeMute(),
                [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->audioMute = value;
                        widget->_audioUpdate();
                    }
                });

            p.cachedFramesObserver = ListObserver<Frame::Range>::create(
                p.media->observeCachedFrames(),
                [weak](const std::vector<Frame::Range>& value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_p->timelineSlider->setCachedFrames(value);
                    }
                });

            if (auto windowSystem = context->getSystemT<WindowSystem>())
            {
                p.fadeObserver = ValueObserver<float>::create(
                    windowSystem->observeFade(),
                    [weak](float value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->fade = value;
                            widget->_opacityUpdate();
                        }
                    });
            }

            auto settingsSystem = context->getSystemT<UI::Settings::System>();
            if (auto imageViewSettings = settingsSystem->getSettingsT<ImageViewSettings>())
            {
                p.viewLockObserver = ValueObserver<ImageViewLock>::create(
                    imageViewSettings->observeLock(),
                    [weak](ImageViewLock value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->viewLock = value;
                        }
                    });
            }

            if (auto imageSystem = context->getSystemT<ImageSystem>())
            {
                p.frameStoreEnabledObserver = ValueObserver<bool>::create(
                    imageSystem->observeFrameStoreEnabled(),
                    [weak](bool value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->frameStoreEnabled = value;
                            widget->_imageUpdate();
                        }
                    });
                p.frameStoreObserver = ValueObserver<std::shared_ptr<AV::Image::Image> >::create(
                    imageSystem->observeFrameStore(),
                    [weak](const std::shared_ptr<AV::Image::Image>& value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->frameStore = value;
                            widget->_imageUpdate();
                        }
                    });
            }
        }

        MediaWidget::MediaWidget() :
            _p(new Private)
        {}

        MediaWidget::~MediaWidget()
        {}

        std::shared_ptr<MediaWidget> MediaWidget::create(const std::shared_ptr<Media>& media, const std::shared_ptr<Context>& context)
        {
            auto out = std::shared_ptr<MediaWidget>(new MediaWidget);
            out->_init(media, context);
            return out;
        }

        const std::shared_ptr<Media>& MediaWidget::getMedia() const
        {
            return _p->media;
        }

        const std::shared_ptr<ImageView>& MediaWidget::getImageView() const
        {
            return _p->imageView;
        }

        void MediaWidget::fitWindow()
        {
            DJV_PRIVATE_PTR();
            const auto& style = _getStyle();
            const float sh = style->getMetric(UI::MetricsRole::Shadow);
            const BBox2f imageBBox = p.imageView->getImageBBox();
            const float zoom = p.imageView->observeImageZoom()->get();
            const glm::vec2 imageSize = imageBBox.getSize() * zoom;
            glm::vec2 size(ceilf(imageSize.x), ceilf(imageSize.y));
            switch (p.viewLock)
            {
            case ImageViewLock::Frame:
                size.y += _getTitleBarHeight() + _getPlaybackHeight();
                break;
            default: break;
            }
            resize(size + sh * 2.f);
        }

        std::shared_ptr<Core::IValueSubject<PointerData> > MediaWidget::observeHover() const
        {
            return _p->hover;
        }

        std::shared_ptr<Core::IValueSubject<PointerData> > MediaWidget::observeDrag() const
        {
            return _p->drag;
        }

        float MediaWidget::_getTitleBarHeight() const
        {
            DJV_PRIVATE_PTR();
            return p.titleBar->getMinimumSize().y;
        }

        float MediaWidget::_getPlaybackHeight() const
        {
            DJV_PRIVATE_PTR();
            return p.playbackLayout->isVisible() ? p.playbackLayout->getMinimumSize().y : 0.f;
        }


        std::map<UI::MDI::Handle, std::vector<Core::BBox2f> > MediaWidget::_getHandles() const
        {
            auto out = IWidget::_getHandles();
            out[UI::MDI::Handle::Move] = { _p->titleBar->getGeometry() };
            return out;
        }

        void MediaWidget::_setMaximize(float value)
        {
            IWidget::_setMaximize(value);
            _opacityUpdate();
            _resize();
        }

        void MediaWidget::_setActiveWidget(bool value)
        {
            IWidget::_setActiveWidget(value);
            DJV_PRIVATE_PTR();
            p.active = value;
            p.titleLabel->setTextColorRole(p.active ? UI::ColorRole::Foreground : UI::ColorRole::ForegroundDim);
            p.maximizeButton->setForegroundColorRole(p.active ? UI::ColorRole::Foreground : UI::ColorRole::ForegroundDim);
            p.closeButton->setForegroundColorRole(p.active ? UI::ColorRole::Foreground : UI::ColorRole::ForegroundDim);
            _imageUpdate();
        }

        void MediaWidget::_preLayoutEvent(Event::PreLayout&)
        {
            DJV_PRIVATE_PTR();
            const auto& style = _getStyle();
            const float sh = style->getMetric(UI::MetricsRole::Shadow);
            const glm::vec2& minimumSize = p.layout->getMinimumSize();

            glm::vec2 imageSize = p.imageView->getMinimumSize();
            const float ar = p.imageView->getImageBBox().getAspect();
            if (ar > 1.f)
            {
                imageSize.x = std::max(imageSize.x * 2.f, minimumSize.x);
                imageSize.y = imageSize.x / ar;
            }
            else if (ar > 0.f)
            {
                imageSize.y = std::max(imageSize.y * 2.f, minimumSize.y);
                imageSize.x = imageSize.y * ar;
            }
            glm::vec2 size(ceilf(imageSize.x), ceilf(imageSize.y));
            
            switch (p.viewLock)
            {
            case ImageViewLock::Frame:
                size.y += _getTitleBarHeight() + _getPlaybackHeight();
                break;
            default: break;
            }

            _setMinimumSize(size + sh * 2.f);
        }

        void MediaWidget::_layoutEvent(Event::Layout&)
        {
            DJV_PRIVATE_PTR();
            const auto& style = _getStyle();
            const float sh = style->getMetric(UI::MetricsRole::Shadow);
            const BBox2f g = getGeometry().margin(-sh);
            p.layout->setGeometry(g);
            const BBox2f imageFrame = BBox2f(
                glm::vec2(
                    g.min.x,
                    g.min.y + _getTitleBarHeight()),
                glm::vec2(
                    g.max.x,
                    g.max.y - _getPlaybackHeight()));
            p.imageView->setImageFrame(imageFrame);
        }

        void MediaWidget::_localeEvent(Event::Locale&)
        {
            DJV_PRIVATE_PTR();
            p.actions["Forward"]->setTooltip(_getText(DJV_TEXT("Forward tooltip")));
            p.actions["Reverse"]->setTooltip(_getText(DJV_TEXT("Reverse tooltip")));
            p.actions["InPoint"]->setTooltip(_getText(DJV_TEXT("Go to in point tooltip")));
            p.actions["NextFrame"]->setTooltip(_getText(DJV_TEXT("Next frame tooltip")));
            p.actions["PrevFrame"]->setTooltip(_getText(DJV_TEXT("Previous frame tooltip")));
            p.actions["OutPoint"]->setTooltip(_getText(DJV_TEXT("Go to out point tooltip")));

            p.maximizeButton->setTooltip(_getText(DJV_TEXT("Maximize tooltip")));
            p.closeButton->setTooltip(_getText(DJV_TEXT("Close tooltip")));

            p.playEveryFrameButton->setTooltip(_getText(DJV_TEXT("Play every frame tooltip")));
            p.speedPopupWidget->setTooltip(_getText(DJV_TEXT("Speed popup tooltip")));
            p.realSpeedLabel->setTooltip(_getText(DJV_TEXT("Real speed tooltip")));
            p.currentFrameWidget->setTooltip(_getText(DJV_TEXT("Current frame tooltip")));
            p.durationLabel->setTooltip(_getText(DJV_TEXT("Duration tooltip")));

            p.audioVolumeSlider->setTooltip(_getText(DJV_TEXT("Volume tooltip")));
            p.audioMuteButton->setTooltip(_getText(DJV_TEXT("Mute tooltip")));
            p.audioPopupWidget->setTooltip(_getText(DJV_TEXT("Audio popup tooltip")));

            _speedUpdate();
        }

        void MediaWidget::_widgetUpdate()
        {
            DJV_PRIVATE_PTR();
            if (auto context = getContext().lock())
            {
                auto playback = Playback::Stop;
                if (p.media)
                {
                    playback = p.media->observePlayback()->get();
                }
                switch (playback)
                {
                case Playback::Stop:    p.playbackActionGroup->setChecked(-1); break;
                case Playback::Forward: p.playbackActionGroup->setChecked( 0); break;
                case Playback::Reverse: p.playbackActionGroup->setChecked( 1); break;
                default: break;
                }

                p.playEveryFrameButton->setChecked(p.playEveryFrame);

                p.currentFrameWidget->setSequence(p.sequence);
                p.currentFrameWidget->setSpeed(p.speed);
                p.currentFrameWidget->setFrame(p.currentFrame);

                auto avSystem = context->getSystemT<AV::AVSystem>();
                p.durationLabel->setText(avSystem->getLabel(p.sequence.getSize(), p.defaultSpeed));

                p.playbackLayout->setVisible(p.sequence.getSize() > 1);
            }
        }

        void MediaWidget::_imageUpdate()
        {
            DJV_PRIVATE_PTR();
            p.imageView->setImage(p.active && p.frameStoreEnabled && p.frameStore ? p.frameStore : p.image);
        }

        void MediaWidget::_speedUpdate()
        {
            DJV_PRIVATE_PTR();
            if (auto context = getContext().lock())
            {
                p.speedButtonGroup->clearButtons();
                p.speedButtonLayout->clearChildren();
                p.speeds =
                {
                    Time::Speed(240),
                    Time::Speed(120),
                    Time::Speed(60),
                    Time::Speed(60000, 1001),
                    Time::Speed(50),
                    Time::Speed(48),
                    Time::Speed(30),
                    Time::Speed(30000, 1001),
                    Time::Speed(25),
                    Time::Speed(24),
                    Time::Speed(24000, 1001),
                    Time::Speed(16),
                    Time::Speed(12),
                    Time::Speed(8),
                    Time::Speed(6)
                };
                for (const auto& i : p.speeds)
                {
                    auto button = UI::ListButton::create(context);
                    std::stringstream ss;
                    ss.precision(3);
                    ss << std::fixed << i.toFloat();
                    button->setText(ss.str());
                    p.speedButtonGroup->addButton(button);
                    p.speedButtonLayout->addChild(button);
                }
                p.speedButtonLayout->addSeparator();
                auto button = UI::ListButton::create(context);
                std::stringstream ss;
                ss << _getText(DJV_TEXT("Default")) << ": ";
                ss.precision(3);
                ss << std::fixed << p.defaultSpeed.toFloat();
                button->setText(ss.str());
                p.speedButtonGroup->addButton(button);
                p.speedButtonLayout->addChild(button);

                p.currentFrameWidget->setSpeed(p.speed);

                p.playEveryFrameLabel->setText(_getText(DJV_TEXT("Play every frame")) + ":");

                {
                    std::stringstream ss;
                    ss.precision(3);
                    ss << _getText(DJV_TEXT("FPS")) << ": " << std::fixed << p.speed.toFloat();
                    p.speedPopupWidget->setText(ss.str());
                }
            }
        }

        void MediaWidget::_realSpeedUpdate()
        {
            DJV_PRIVATE_PTR();
            {
                std::stringstream ss;
                ss.precision(3);
                ss << std::fixed << p.realSpeed;
                p.realSpeedLabel->setText(ss.str());
            }
        }

        void MediaWidget::_audioUpdate()
        {
            DJV_PRIVATE_PTR();
            p.audioVolumeSlider->setValue(p.audioVolume);
            p.audioMuteButton->setChecked(p.audioMute);
            if (!p.audioEnabled || p.audioMute)
            {
                p.audioPopupWidget->setIcon("djvIconAudioMute");
            }
            else if (p.audioVolume < 1.f / 4.f)
            {
                p.audioPopupWidget->setIcon("djvIconAudio0");
            }
            else if (p.audioVolume < 2.f / 4.f)
            {
                p.audioPopupWidget->setIcon("djvIconAudio1");
            }
            else if (p.audioVolume < 3.f / 4.f)
            {
                p.audioPopupWidget->setIcon("djvIconAudio2");
            }
            else
            {
                p.audioPopupWidget->setIcon("djvIconAudio3");
            }
        }

        void MediaWidget::_opacityUpdate()
        {
            DJV_PRIVATE_PTR();
            const float maximize = 1.f - _getMaximize();
            p.titleBar->setOpacity(p.fade * maximize);
            p.playbackLayout->setOpacity(p.fade);
        }

    } // namespace ViewApp
} // namespace djv

