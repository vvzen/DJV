// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvUI/GroupBox.h>

#include <djvUI/Label.h>
#include <djvUI/RowLayout.h>
#include <djvUI/StackLayout.h>

#include <djvRender2D/Render.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace Layout
        {
            struct GroupBox::Private
            {
                std::shared_ptr<Text::Label> titleLabel;
                std::shared_ptr<StackLayout> childLayout;
                std::shared_ptr<VerticalLayout> layout;
            };

            void GroupBox::_init(const std::shared_ptr<System::Context>& context)
            {
                Widget::_init(context);
                DJV_PRIVATE_PTR();

                setClassName("djv::UI::Layout::GroupBox");
                setVAlign(VAlign::Top);

                p.titleLabel = Text::Label::create(context);
                p.titleLabel->setTextHAlign(TextHAlign::Left);
                p.titleLabel->setMargin(MetricsRole::MarginSmall);
                p.titleLabel->setBackgroundRole(ColorRole::BackgroundBellows);

                p.childLayout = StackLayout::create(context);
                p.childLayout->setMargin(MetricsRole::None);

                p.layout = VerticalLayout::create(context);
                p.layout->setSpacing(MetricsRole::None);
                p.layout->addChild(p.titleLabel);
                p.layout->addChild(p.childLayout);
                p.layout->setStretch(p.childLayout);
                Widget::addChild(p.layout);
            }

            GroupBox::GroupBox() :
                _p(new Private)
            {}

            GroupBox::~GroupBox()
            {}

            std::shared_ptr<GroupBox> GroupBox::create(const std::shared_ptr<System::Context>& context)
            {
                auto out = std::shared_ptr<GroupBox>(new GroupBox);
                out->_init(context);
                return out;
            }

            const std::string& GroupBox::getText() const
            {
                return _p->titleLabel->getText();
            }

            void GroupBox::setText(const std::string& text)
            {
                _p->titleLabel->setText(text);
            }

            float GroupBox::getHeightForWidth(float value) const
            {
                return _p->layout->getHeightForWidth(value);
            }

            void GroupBox::addChild(const std::shared_ptr<IObject>& value)
            {
                _p->childLayout->addChild(value);
            }

            void GroupBox::removeChild(const std::shared_ptr<IObject>& value)
            {
                _p->childLayout->removeChild(value);
            }

            void GroupBox::clearChildren()
            {
                _p->childLayout->clearChildren();
            }

            void GroupBox::_preLayoutEvent(System::Event::PreLayout& event)
            {
                _setMinimumSize(_p->layout->getMinimumSize());
            }

            void GroupBox::_layoutEvent(System::Event::Layout& event)
            {
                _p->layout->setGeometry(getGeometry());
            }

        } // namespace Layout
    } // namespace UI
} // namespace djv
