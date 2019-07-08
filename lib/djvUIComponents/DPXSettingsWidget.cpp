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

#include <djvUIComponents/DPXSettingsWidget.h>

#include <djvUI/ComboBox.h>
#include <djvUI/FormLayout.h>
#include <djvUI/GroupBox.h>

#include <djvAV/DPX.h>
#include <djvAV/OCIOSystem.h>

#include <djvCore/Context.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        struct DPXSettingsWidget::Private
        {
            std::vector<std::string> colorSpaces;
            std::shared_ptr<ComboBox> colorSpaceComboBox;
            std::shared_ptr<FormLayout> layout;
        };

        void DPXSettingsWidget::_init(Context * context)
        {
            ISettingsWidget::_init(context);

            DJV_PRIVATE_PTR();
            setClassName("djv::UI::DPXSettingsWidget");

            p.colorSpaceComboBox = ComboBox::create(context);

            p.layout = FormLayout::create(context);
            p.layout->addChild(p.colorSpaceComboBox);
            addChild(p.layout);

            auto weak = std::weak_ptr<DPXSettingsWidget>(std::dynamic_pointer_cast<DPXSettingsWidget>(shared_from_this()));
            p.colorSpaceComboBox->setCallback(
                [weak, context](int value)
            {
                    if (auto widget = weak.lock())
                    {
                        auto io = context->getSystemT<AV::IO::System>();
                        AV::IO::DPX::Options options;
                        fromJSON(io->getOptions(AV::IO::DPX::pluginName), options);
                        options.colorSpace = widget->_p->colorSpaces[value];
                        io->setOptions(AV::IO::DPX::pluginName, toJSON(options));
                    }
            });

            auto ocioSystem = context->getSystemT<AV::OCIO::System>();
            ListObserver<std::string>::create(
                ocioSystem->observeColorSpaces(),
                [weak, context](const std::vector<std::string>&)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->_widgetUpdate();
                    }
                });
        }

        DPXSettingsWidget::DPXSettingsWidget() :
            _p(new Private)
        {}

        std::shared_ptr<DPXSettingsWidget> DPXSettingsWidget::create(Context * context)
        {
            auto out = std::shared_ptr<DPXSettingsWidget>(new DPXSettingsWidget);
            out->_init(context);
            return out;
        }

        std::string DPXSettingsWidget::getSettingsName() const
        {
            return DJV_TEXT("DPX");
        }

        std::string DPXSettingsWidget::getSettingsGroup() const
        {
            return DJV_TEXT("I/O");
        }

        std::string DPXSettingsWidget::getSettingsSortKey() const
        {
            return "Z";
        }

        void DPXSettingsWidget::_localeEvent(Event::Locale& event)
        {
            ISettingsWidget::_localeEvent(event);
            DJV_PRIVATE_PTR();
            p.layout->setText(p.colorSpaceComboBox, _getText(DJV_TEXT("Color space")) + ":");
            _widgetUpdate();
        }

        void DPXSettingsWidget::_widgetUpdate()
        {
            DJV_PRIVATE_PTR();
            auto context = getContext();
            auto io = context->getSystemT<AV::IO::System>();
            AV::IO::DPX::Options options;
            fromJSON(io->getOptions(AV::IO::DPX::pluginName), options);

            auto ocioSystem = context->getSystemT<AV::OCIO::System>();
            p.colorSpaces.clear();
            p.colorSpaces.push_back(std::string());
            for (const auto& i : ocioSystem->observeColorSpaces()->get())
            {
                p.colorSpaces.push_back(i);
            }
            p.colorSpaceComboBox->setItems(p.colorSpaces);

            int index = 0;
            const auto i = std::find(p.colorSpaces.begin(), p.colorSpaces.end(), options.colorSpace);
            if (i != p.colorSpaces.end())
            {
                index = i - p.colorSpaces.begin();
            }
            p.colorSpaceComboBox->setCurrentItem(index);
        }

    } // namespace UI
} // namespace djv

