// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvUI/ActionButton.h>

#include <djvUI/Action.h>
#include <djvUI/DrawUtil.h>
#include <djvUI/Icon.h>
#include <djvUI/Label.h>
#include <djvUI/RowLayout.h>
#include <djvUI/Shortcut.h>
#include <djvUI/ShortcutDataFunc.h>

#include <djvRender2D/Render.h>

#include <djvSystem/Context.h>
#include <djvSystem/TextSystem.h>

#include <djvCore/StringFunc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace Button
        {
            namespace
            {
                class CheckBox : public Widget
                {
                    DJV_NON_COPYABLE(CheckBox);

                protected:
                    void _init(const std::shared_ptr<System::Context>&);
                    CheckBox();

                public:
                    ~CheckBox() override;

                    static std::shared_ptr<CheckBox> create(const std::shared_ptr<System::Context>&);

                    void setChecked(bool);

                protected:
                    void _preLayoutEvent(System::Event::PreLayout&) override;
                    void _paintEvent(System::Event::Paint&) override;

                private:
                    bool _checked = false;
                };

                void CheckBox::_init(const std::shared_ptr<System::Context>& context)
                {
                    Widget::_init(context);
                }

                CheckBox::CheckBox()
                {}

                CheckBox::~CheckBox()
                {}

                std::shared_ptr<CheckBox> CheckBox::create(const std::shared_ptr<System::Context>& context)
                {
                    auto out = std::shared_ptr<CheckBox>(new CheckBox);
                    out->_init(context);
                    return out;
                }

                void CheckBox::setChecked(bool value)
                {
                    if (value == _checked)
                        return;
                    _checked = value;
                    _redraw();
                }

                void CheckBox::_preLayoutEvent(System::Event::PreLayout&)
                {
                    const auto& style = _getStyle();
                    const float m = style->getMetric(MetricsRole::MarginInside);
                    const glm::vec2 checkBoxSize = getCheckBoxSize(style);
                    _setMinimumSize(checkBoxSize + m * 2.F);
                }

                void CheckBox::_paintEvent(System::Event::Paint&)
                {
                    const auto& style = _getStyle();
                    const float m = style->getMetric(MetricsRole::MarginInside);
                    const Math::BBox2f g = getGeometry().margin(-m);
                    const glm::vec2 checkBoxSize = getCheckBoxSize(style);
                    const Math::BBox2f checkBoxGeometry(g.min.x, g.min.y + floorf(g.h() / 2.F - checkBoxSize.y / 2.F), checkBoxSize.x, checkBoxSize.y);
                    const auto& render = _getRender();
                    drawCheckBox(render, style, checkBoxGeometry, _checked);
                }

            } // namespace

            struct ActionButton::Private
            {
                std::shared_ptr<Action> action;
                std::shared_ptr<CheckBox> checkBox;
                std::shared_ptr<Icon> icon;
                std::shared_ptr<Text::Label> textLabel;
                std::shared_ptr<Text::Label> shortcutsLabel;
                std::shared_ptr<HorizontalLayout> layout;
                std::shared_ptr<Observer::Value<std::string> > iconObserver;
                std::shared_ptr<Observer::Value<std::string> > textObserver;
                std::shared_ptr<Observer::List<std::shared_ptr<Shortcut> > > shortcutsObserver;
            };

            void ActionButton::_init(const std::shared_ptr<System::Context>& context)
            {
                Widget::_init(context);

                DJV_PRIVATE_PTR();
                setClassName("djv::UI::Button::ActionButton");

                p.checkBox = CheckBox::create(context);
                p.icon = Icon::create(context);
                p.textLabel = Text::Label::create(context);
                p.textLabel->setTextHAlign(TextHAlign::Left);
                p.textLabel->setMargin(MetricsRole::MarginSmall);
                p.shortcutsLabel = Text::Label::create(context);
                p.shortcutsLabel->setMargin(MetricsRole::MarginSmall);

                p.layout = HorizontalLayout::create(context);
                p.layout->setSpacing(MetricsRole::None);
                p.layout->addChild(p.checkBox);
                p.layout->addChild(p.icon);
                p.layout->addChild(p.textLabel);
                p.layout->setStretch(p.textLabel);
                p.layout->addChild(p.shortcutsLabel);
                addChild(p.layout);

                _actionUpdate();
                _widgetUpdate();

                auto weak = std::weak_ptr<ActionButton>(std::dynamic_pointer_cast<ActionButton>(shared_from_this()));
                setClickedCallback(
                    [weak]
                    {
                        if (auto widget = weak.lock())
                        {
                            if (widget->_p->action)
                            {
                                widget->_p->action->doClick();
                            }
                        }
                    });
            }

            ActionButton::ActionButton() :
                _p(new Private)
            {}

            ActionButton::~ActionButton()
            {}

            std::shared_ptr<ActionButton> ActionButton::create(const std::shared_ptr<System::Context>& context)
            {
                auto out = std::shared_ptr<ActionButton>(new ActionButton);
                out->_init(context);
                return out;
            }

            bool ActionButton::hasShowShortcuts() const
            {
                return _p->shortcutsLabel->isVisible();
            }

            void ActionButton::setShowShortcuts(bool value)
            {
                _p->shortcutsLabel->setVisible(value);
            }

            void ActionButton::setChecked(bool value)
            {
                IButton::setChecked(value);
                _p->checkBox->setChecked(value);
            }

            void ActionButton::setButtonType(ButtonType value)
            {
                IButton::setButtonType(value);
                _widgetUpdate();
            }

            void ActionButton::addAction(const std::shared_ptr<Action>& value)
            {
                IButton::addAction(value);
                _actionUpdate();
                _widgetUpdate();
            }

            void ActionButton::removeAction(const std::shared_ptr<Action>& value)
            {
                IButton::removeAction(value);
                _actionUpdate();
                _widgetUpdate();
            }

            void ActionButton::clearActions()
            {
                IButton::clearActions();
                _actionUpdate();
                _widgetUpdate();
            }

            bool ActionButton::acceptFocus(TextFocusDirection)
            {
                bool out = false;
                if (isEnabled(true) && isVisible(true) && !isClipped())
                {
                    takeTextFocus();
                    out = true;
                }
                return out;
            }

            void ActionButton::_preLayoutEvent(System::Event::PreLayout& event)
            {
                DJV_PRIVATE_PTR();
                const auto& style = _getStyle();
                const float bt = style->getMetric(MetricsRole::BorderTextFocus);
                _setMinimumSize(p.layout->getMinimumSize() + bt * 2.F);
            }

            void ActionButton::_layoutEvent(System::Event::Layout&)
            {
                DJV_PRIVATE_PTR();
                const auto& style = _getStyle();
                const float bt = style->getMetric(MetricsRole::BorderTextFocus);
                p.layout->setGeometry(getGeometry().margin(-bt));
            }

            void ActionButton::_paintEvent(System::Event::Paint& event)
            {
                IButton::_paintEvent(event);
                const auto& style = _getStyle();
                const float bt = style->getMetric(MetricsRole::BorderTextFocus);
                const Math::BBox2f& g = getGeometry();

                const auto& render = _getRender();
                if (hasTextFocus())
                {
                    render->setFillColor(style->getColor(ColorRole::TextFocus));
                    drawBorder(render, g, bt);
                }

                const Math::BBox2f g2 = g.margin(-bt);
                if (_isPressed())
                {
                    render->setFillColor(style->getColor(ColorRole::Pressed));
                    render->drawRect(g2);
                }
                else if (_isHovered())
                {
                    render->setFillColor(style->getColor(ColorRole::Hovered));
                    render->drawRect(g2);
                }
            }

            void ActionButton::_keyPressEvent(System::Event::KeyPress& event)
            {
                IButton::_keyPressEvent(event);
                DJV_PRIVATE_PTR();
                if (!event.isAccepted() && hasTextFocus())
                {
                    switch (event.getKey())
                    {
                    case GLFW_KEY_ENTER:
                    case GLFW_KEY_SPACE:
                        event.accept();
                        switch (getButtonType())
                        {
                        case ButtonType::Push:
                            _doClick();
                            break;
                        case ButtonType::Toggle:
                            _doCheck(!isChecked());
                            break;
                        case ButtonType::Radio:
                            if (!isChecked())
                            {
                                _doCheck(true);
                            }
                            break;
                        case ButtonType::Exclusive:
                            _doCheck(!isChecked());
                            break;
                        default: break;
                        }
                        break;
                    case GLFW_KEY_ESCAPE:
                        event.accept();
                        releaseTextFocus();
                        break;
                    default: break;
                    }
                }
            }

            void ActionButton::_textFocusEvent(System::Event::TextFocus&)
            {
                _redraw();
            }

            void ActionButton::_textFocusLostEvent(System::Event::TextFocusLost&)
            {
                _redraw();
            }

            void ActionButton::_actionUpdate()
            {
                DJV_PRIVATE_PTR();
                const auto& actions = getActions();
                if (actions.size())
                {
                    p.action = actions.front();
                    auto weak = std::weak_ptr<ActionButton>(std::dynamic_pointer_cast<ActionButton>(shared_from_this()));
                    p.iconObserver = Observer::Value<std::string>::create(
                        p.action->observeIcon(),
                        [weak](const std::string& value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->icon->setIcon(value);
                        }
                    });
                    p.textObserver = Observer::Value<std::string>::create(
                        p.action->observeText(),
                        [weak](const std::string& value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->textLabel->setText(value);
                        }
                    });
                    p.shortcutsObserver = Observer::List<std::shared_ptr<Shortcut> >::create(
                        p.action->observeShortcuts(),
                        [weak](const std::vector<std::shared_ptr<Shortcut> >& value)
                    {
                        if (auto widget = weak.lock())
                        {
                            if (auto context = widget->getContext().lock())
                            {
                                auto textSystem = context->getSystemT<System::TextSystem>();
                                std::vector<std::string> labels;
                                for (const auto& i : value)
                                {
                                    const auto& shortcut = i->observeShortcut()->get();
                                    if (shortcut.isValid())
                                    {
                                        labels.push_back(getText(shortcut.key, shortcut.modifiers, textSystem));
                                    }
                                }
                                widget->_p->shortcutsLabel->setText(String::join(labels, ", "));
                            }
                        }
                    });
                }
                else
                {
                    p.action.reset();
                    p.iconObserver.reset();
                    p.textObserver.reset();
                    p.shortcutsObserver.reset();
                }
            }

            void ActionButton::_widgetUpdate()
            {
                DJV_PRIVATE_PTR();
                if (p.action)
                {
                    _p->checkBox->setVisible(p.action->observeButtonType()->get() != ButtonType::Push);
                }
            }

        } // namespace Button
    } // namespace UI
} // namespace djv

