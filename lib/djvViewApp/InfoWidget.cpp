// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvViewApp/InfoWidget.h>

#include <djvViewApp/FileSystem.h>
#include <djvViewApp/Media.h>

#include <djvUI/FormLayout.h>
#include <djvUI/GroupBox.h>
#include <djvUI/Label.h>
#include <djvUI/RowLayout.h>
#include <djvUI/TextBlock.h>

#include <djvAV/AVSystem.h>
#include <djvAV/TimeFunc.h>

#include <djvSystem/Context.h>

#include <djvCore/StringFunc.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        struct InfoWidget::Private
        {
            AV::IO::Info info;
            std::string filter;
            std::vector<std::shared_ptr<UI::GroupBox> > groupBoxes;
            std::shared_ptr<UI::VerticalLayout> layout;
            std::shared_ptr<Observer::Value<std::shared_ptr<Media> > > currentMediaObserver;
            std::shared_ptr<Observer::Value<AV::IO::Info> > infoObserver;

            std::shared_ptr<UI::Text::Block> createTextBlock(const std::shared_ptr<System::Context>& context)
            {
                auto textBlock = UI::Text::Block::create(context);
                textBlock->setFontFamily(Render2D::Font::familyMono);
                textBlock->setMargin(UI::MetricsRole::MarginSmall);
                return textBlock;
            }

            std::shared_ptr<UI::FormLayout> createFormLayout(const std::shared_ptr<System::Context>& context)
            {
                auto formLayout = UI::FormLayout::create(context);
                formLayout->setAlternateRowsRoles(UI::ColorRole::None, UI::ColorRole::Trough);
                formLayout->setLabelVAlign(UI::VAlign::Top);
                formLayout->setSpacing(UI::MetricsRole::None);
                return formLayout;
            }
        };

        void InfoWidget::_init(const std::shared_ptr<System::Context>& context)
        {
            Widget::_init(context);
            DJV_PRIVATE_PTR();

            setClassName("djv::ViewApp::InfoWidget");

            p.layout = UI::VerticalLayout::create(context);
            addChild(p.layout);

            _widgetUpdate();

            auto weak = std::weak_ptr<InfoWidget>(std::dynamic_pointer_cast<InfoWidget>(shared_from_this()));
            if (auto fileSystem = context->getSystemT<FileSystem>())
            {
                p.currentMediaObserver = Observer::Value<std::shared_ptr<Media> >::create(
                    fileSystem->observeCurrentMedia(),
                    [weak](const std::shared_ptr<Media>& value)
                    {
                        if (auto widget = weak.lock())
                        {
                            if (value)
                            {
                                widget->_p->infoObserver = Observer::Value<AV::IO::Info>::create(
                                    value->observeInfo(),
                                    [weak](const AV::IO::Info& value)
                                    {
                                        if (auto widget = weak.lock())
                                        {
                                            widget->_p->info = value;
                                            widget->_widgetUpdate();
                                        }
                                    });
                            }
                            else
                            {
                                widget->_p->info = AV::IO::Info();
                                widget->_p->infoObserver.reset();
                                widget->_widgetUpdate();
                            }
                        }
                    });
            }
        }

        InfoWidget::InfoWidget() :
            _p(new Private)
        {}

        InfoWidget::~InfoWidget()
        {}

        std::shared_ptr<InfoWidget> InfoWidget::create(const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<InfoWidget>(new InfoWidget);
            out->_init(context);
            return out;
        }

        void InfoWidget::setFilter(const std::string& value)
        {
            DJV_PRIVATE_PTR();
            if (value == p.filter)
                return;
            p.filter = value;
            _widgetUpdate();
        }

        void InfoWidget::_preLayoutEvent(System::Event::PreLayout&)
        {
            _setMinimumSize(_p->layout->getMinimumSize());
        }

        void InfoWidget::_layoutEvent(System::Event::Layout&)
        {
            _p->layout->setGeometry(getGeometry());
        }

        void InfoWidget::_initEvent(System::Event::Init & event)
        {
            Widget::_initEvent(event);
            DJV_PRIVATE_PTR();
            if (event.getData().text)
            {
                _widgetUpdate();
            }
        }

        std::string InfoWidget::_text(int value) const
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }

        std::string InfoWidget::_text(const Math::IntRational& value) const
        {
            std::stringstream ss;
            ss.precision(2);
            ss << std::fixed << value.toFloat();
            return ss.str();
        }

        std::string InfoWidget::_text(const Math::Frame::Sequence& sequence, const Math::IntRational& speed) const
        {
            std::stringstream ss;
            if (auto context = getContext().lock())
            {
                auto avSystem = context->getSystemT<AV::AVSystem>();
                const AV::Time::Units timeUnits = avSystem->observeTimeUnits()->get();
                ss << AV::Time::toString(sequence.getFrameCount(), speed, timeUnits);
                switch (timeUnits)
                {
                case AV::Time::Units::Frames:
                    ss << " " << _getText(DJV_TEXT("widget_info_frames"));
                    break;
                default: break;
                }
            }
            return ss.str();
        }

        std::string InfoWidget::_text(Image::Type value) const
        {
            std::stringstream ss;
            ss << value;
            return _getText(ss.str());
        }

        std::string InfoWidget::_text(const Image::Size& value) const
        {
            std::stringstream ss;
            ss << value.w << "x" << value.h;
            ss.precision(2);
            ss << ":" << std::fixed << value.getAspectRatio();
            return ss.str();
        }

        std::string InfoWidget::_text(Audio::Type value) const
        {
            std::stringstream ss;
            ss << value;
            return _getText(ss.str());
        }

        std::string InfoWidget::_textSampleRate(size_t value) const
        {
            std::stringstream ss;
            ss << value / 1000.F << _getText(DJV_TEXT("widget_info_khz"));
            return ss.str();
        }

        std::string InfoWidget::_textDuration(size_t sampleCount, size_t sampleRate) const
        {
            std::stringstream ss;
            ss << (sampleRate > 0 ? (sampleCount / sampleRate) : 0) << " " << _getText(DJV_TEXT("widget_info_seconds"));
            return ss.str();
        }

        void InfoWidget::_widgetUpdate()
        {
            DJV_PRIVATE_PTR();
            if (auto context = getContext().lock())
            {
                p.groupBoxes.clear();
                p.layout->clearChildren();

                const std::string fileNameLabel = _getText(DJV_TEXT("widget_info_file_name"));
                std::string speedLabel;
                std::string speedText;
                std::string durationLabel;
                std::string durationText;
                const bool frames = p.info.videoSequence.getFrameCount() > 1;
                if (frames)
                {
                    speedLabel = _getText(DJV_TEXT("widget_info_speed"));
                    speedText = _text(p.info.videoSpeed);
                    durationLabel = _getText(DJV_TEXT("widget_info_duration"));
                    durationText = _text(p.info.videoSequence, p.info.videoSpeed);
                }

                const std::string general = _getText(DJV_TEXT("widget_info_general"));
                const bool generalMatch = String::match(general, p.filter);
                const bool fileNameMatch = !p.info.fileName.empty() &&
                    (String::match(fileNameLabel, p.filter) || String::match(p.info.fileName, p.filter));
                bool speedMatch = false;
                bool durationMatch = false;
                if (frames)
                {
                    speedMatch |= String::match(speedLabel, p.filter) || String::match(speedText, p.filter);
                    durationMatch |= String::match(durationLabel, p.filter) || String::match(durationText, p.filter);
                }
                if (generalMatch || fileNameMatch || speedMatch || durationMatch)
                {
                    auto formLayout = p.createFormLayout(context);
                    if (generalMatch || fileNameMatch)
                    {
                        auto textBlock = p.createTextBlock(context);
                        textBlock->setText(p.info.fileName);
                        formLayout->addChild(textBlock);
                        formLayout->setText(textBlock, fileNameLabel + ":");
                    }
                    if (frames && (generalMatch || speedMatch))
                    {
                        auto textBlock = p.createTextBlock(context);
                        textBlock->setText(speedText);
                        formLayout->addChild(textBlock);
                        formLayout->setText(textBlock, speedLabel + ":");
                    }
                    if (frames && (generalMatch || durationMatch))
                    {
                        auto textBlock = p.createTextBlock(context);
                        textBlock->setText(durationText);
                        formLayout->addChild(textBlock);
                        formLayout->setText(textBlock, durationLabel + ":");
                    }
                    auto groupBox = UI::GroupBox::create(context);
                    groupBox->setText(general);
                    groupBox->addChild(formLayout);
                    p.groupBoxes.push_back(groupBox);
                    p.layout->addChild(groupBox);
                }

                size_t j = 0;
                for (const auto& i : p.info.video)
                {
                    const std::string sizeLabel = _getText(DJV_TEXT("widget_info_dimensions"));
                    const std::string sizeText = _text(i.size);
                    const std::string typeLabel = _getText(DJV_TEXT("widget_info_type"));
                    const std::string typeText = _text(i.type);
                    const std::string codecLabel = _getText(DJV_TEXT("widget_info_codec"));

                    const bool nameMatch = String::match(i.name, p.filter);
                    const bool sizeMatch = String::match(sizeLabel, p.filter) || String::match(sizeText, p.filter);
                    const bool typeMatch = String::match(typeLabel, p.filter) || String::match(typeText, p.filter);
                    const bool codecMatch = String::match(codecLabel, p.filter) || String::match(i.codec, p.filter);
                    if (nameMatch || sizeMatch || typeMatch || codecMatch)
                    {
                        auto formLayout = p.createFormLayout(context);
                        if (nameMatch || sizeMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(sizeText);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, sizeLabel + ":");
                        }
                        if (nameMatch || typeMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(typeText);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, typeLabel + ":");
                        }
                        if (nameMatch || codecMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(i.codec);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, codecLabel + ":");
                        }
                        auto groupBox = UI::GroupBox::create(context);
                        groupBox->setText(i.name);
                        groupBox->addChild(formLayout);
                        p.groupBoxes.push_back(groupBox);
                        p.layout->addChild(groupBox);
                    }

                    ++j;
                }

                if (p.info.audio.isValid())
                {
                    const std::string channelLabel = _getText(DJV_TEXT("widget_info_channels"));
                    const std::string channelText = _text(p.info.audio.channelCount);
                    const std::string typeLabel = _getText(DJV_TEXT("widget_info_type"));
                    const std::string typeText = _text(p.info.audio.type);
                    const std::string sampleRateLabel = _getText(DJV_TEXT("widget_info_sample_rate"));
                    const std::string sampleRateText = _textSampleRate(p.info.audio.sampleRate);
                    const std::string durationLabel = _getText(DJV_TEXT("widget_info_duration"));
                    const std::string durationText = _textDuration(p.info.audioSampleCount, p.info.audio.sampleRate);
                    const std::string codecLabel = _getText(DJV_TEXT("widget_info_codec"));

                    const bool nameMatch = String::match(p.info.audio.name, p.filter);
                    const bool channelMatch = String::match(channelLabel, p.filter) || String::match(channelText, p.filter);
                    const bool typeMatch = String::match(typeLabel, p.filter) || String::match(typeText, p.filter);
                    const bool sampleRateMatch = String::match(sampleRateLabel, p.filter) || String::match(sampleRateText, p.filter);
                    const bool durationMatch = String::match(durationLabel, p.filter) || String::match(durationText, p.filter);
                    const bool codecMatch = String::match(codecLabel, p.filter) || String::match(p.info.audio.codec, p.filter);
                    if (nameMatch || channelMatch || typeMatch || sampleRateMatch || durationMatch || codecMatch)
                    {
                        auto formLayout = p.createFormLayout(context);
                        if (nameMatch || channelMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(channelText);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, channelLabel + ":");
                        }
                        if (nameMatch || typeMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(_getText(typeText));
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, typeLabel + ":");
                        }
                        if (nameMatch || sampleRateMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(sampleRateText);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, sampleRateLabel + ":");
                        }
                        if (nameMatch || durationMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(durationText);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, durationLabel + ":");
                        }
                        if (nameMatch || codecMatch)
                        {
                            auto textBlock = p.createTextBlock(context);
                            textBlock->setText(p.info.audio.codec);
                            formLayout->addChild(textBlock);
                            formLayout->setText(textBlock, codecLabel + ":");
                        }
                        auto groupBox = UI::GroupBox::create(context);
                        groupBox->setText(p.info.audio.name);
                        groupBox->addChild(formLayout);
                        p.groupBoxes.push_back(groupBox);
                        p.layout->addChild(groupBox);
                    }
                }

                if (!p.info.tags.isEmpty())
                {
                    const std::string name = _getText(DJV_TEXT("widget_info_tags"));
                    const bool nameMatch = String::match(name, p.filter);
                    bool match = false;
                    for (const auto& i : p.info.tags.get())
                    {
                        match |= String::match(i.first, p.filter);
                        match |= String::match(i.second, p.filter);
                    }
                    if (nameMatch || match)
                    {
                        auto formLayout = p.createFormLayout(context);
                        for (const auto& i : p.info.tags.get())
                        {
                            if (nameMatch || String::match(i.first, p.filter) || String::match(i.second, p.filter))
                            {
                                auto textBlock = p.createTextBlock(context);
                                textBlock->setText(i.second);
                                formLayout->addChild(textBlock);
                                std::stringstream ss;
                                ss << i.first << ":";
                                formLayout->setText(textBlock, ss.str());
                            }
                        }
                        auto groupBox = UI::GroupBox::create(context);
                        {
                            groupBox->setText(name);
                        }
                        groupBox->addChild(formLayout);
                        p.groupBoxes.push_back(groupBox);
                        p.layout->addChild(groupBox);
                    }
                }
            }
        }

    } // namespace ViewApp
} // namespace djv

