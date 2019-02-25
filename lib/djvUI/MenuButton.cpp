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

#include <djvUI/MenuButton.h>

#include <djvUI/Border.h>
#include <djvUI/Icon.h>
#include <djvUI/Label.h>
#include <djvUI/RowLayout.h>
#include <djvUI/StackLayout.h>

#include <djvAV/Render2D.h>

//#pragma optimize("", off)

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace Button
        {
            struct Menu::Private
            {
                bool checked = false;
                std::shared_ptr<Icon> icon;
                std::shared_ptr<Label> label;
                std::shared_ptr<HorizontalLayout> layout;
                std::shared_ptr<Border> border;
                std::function<void(bool)> checkedCallback;
            };

            void Menu::_init(Context * context)
            {
                Widget::_init(context);

                setClassName("djv::UI::Button::Menu");
                setPointerEnabled(true);

                DJV_PRIVATE_PTR();
                p.icon = Icon::create(context);
                p.icon->setVAlign(VAlign::Center);
                p.icon->hide();

                p.label = Label::create(context);
                p.label->setHAlign(HAlign::Left);
                p.label->hide();

                p.layout = HorizontalLayout::create(context);
                p.layout->addWidget(p.icon);
                p.layout->addWidget(p.label, RowStretch::Expand);

                p.border = Border::create(context);
                p.border->setBorderSize(MetricsRole::None);
                p.border->setInsideMargin(MetricsRole::MarginSmall);
                p.border->addWidget(p.layout);
                p.border->setParent(shared_from_this());
            }

            Menu::Menu() :
                _p(new Private)
            {}

            Menu::~Menu()
            {}

            std::shared_ptr<Menu> Menu::create(Context * context)
            {
                auto out = std::shared_ptr<Menu>(new Menu);
                out->_init(context);
                return out;
            }

            std::shared_ptr<Menu> Menu::create(const std::string & text, Context * context)
            {
                auto out = std::shared_ptr<Menu>(new Menu);
                out->_init(context);
                out->setText(text);
                return out;
            }

            std::shared_ptr<Menu> Menu::create(const std::string & text, const std::string & icon, Context * context)
            {
                auto out = std::shared_ptr<Menu>(new Menu);
                out->_init(context);
                out->setIcon(icon);
                out->setText(text);
                return out;
            }

            const std::string & Menu::getIcon() const
            {
                return _p->icon->getIcon();
            }

            void Menu::setIcon(const std::string & value)
            {
                DJV_PRIVATE_PTR();
                p.icon->setIcon(value);
                p.icon->setVisible(!value.empty());
            }

            const std::string & Menu::getText() const
            {
                return _p->label->getText();
            }

            void Menu::setText(const std::string & value)
            {
                DJV_PRIVATE_PTR();
                p.label->setText(value);
                p.label->setVisible(!value.empty());
            }

            bool Menu::isChecked() const
            {
                return _p->checked;
            }

            void Menu::setChecked(bool value)
            {
                DJV_PRIVATE_PTR();
                if (value == p.checked)
                    return;
                p.checked = value;
                _redraw();
                if (p.checkedCallback)
                {
                    p.checkedCallback(p.checked);
                }
            }

            void Menu::setCheckedCallback(const std::function<void(bool)> & callback)
            {
                _p->checkedCallback = callback;
            }

            bool Menu::hasBorder() const
            {
                return _p->border->getBorderSize() != MetricsRole::Border;
            }

            void Menu::setBorder(bool value)
            {
                _p->border->setBorderSize(value ? MetricsRole::Border : MetricsRole::None);
            }

            const Layout::Margin & Menu::getInsideMargin() const
            {
                return _p->border->getInsideMargin();
            }

            void Menu::setInsideMargin(const Layout::Margin & value)
            {
                _p->border->setInsideMargin(value);
            }

            void Menu::_preLayoutEvent(Event::PreLayout &)
            {
                DJV_PRIVATE_PTR();
                if (auto style = _getStyle().lock())
                {
                    _setMinimumSize(p.border->getMinimumSize() + getMargin().getSize(style));
                }
            }

            void Menu::_layoutEvent(Event::Layout &)
            {
                DJV_PRIVATE_PTR();
                if (auto style = _getStyle().lock())
                {
                    p.border->setGeometry(getMargin().bbox(getGeometry(), style));
                }
            }

            void Menu::_paintEvent(Event::Paint & event)
            {
                Widget::_paintEvent(event);
                if (auto render = _getRender().lock())
                {
                    if (auto style = _getStyle().lock())
                    {
                        const BBox2f & g = getMargin().bbox(getGeometry(), style);
                        if (_p->checked)
                        {
                            render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Pressed)));
                            render->drawRect(g);
                        }
                        else if (_isHovered())
                        {
                            render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Hovered)));
                            render->drawRect(g);
                        }
                    }
                }
            }

            void Menu::_pointerEnterEvent(Event::PointerEnter & event)
            {
                event.accept();
                if (isEnabled(true))
                {
                    _redraw();
                }
            }

            void Menu::_pointerLeaveEvent(Event::PointerLeave & event)
            {
                event.accept();
                if (isEnabled(true))
                {
                    _redraw();
                }
            }

            void Menu::_pointerMoveEvent(Event::PointerMove & event)
            {
                event.accept();
            }

            void Menu::_buttonPressEvent(Event::ButtonPress & event)
            {
                if (isEnabled(true))
                {
                    event.accept();
                    setChecked(!_p->checked);
                }
            }

            void Menu::_updateEvent(Event::Update & event)
            {
                const ColorRole colorRole = !isEnabled(true) ? ColorRole::Disabled : ColorRole::Foreground;
                DJV_PRIVATE_PTR();
                p.icon->setIconColorRole(colorRole);
                p.label->setTextColorRole(colorRole);
            }

            bool Menu::_isHovered() const
            {
                return isEnabled(true) && _getPointerHover().size();
            }

        } // namespace Button
    } // namespace UI
} // namespace Gp
