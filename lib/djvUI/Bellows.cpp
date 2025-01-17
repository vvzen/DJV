// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvUI/Bellows.h>

#include <djvUI/IButton.h>
#include <djvUI/Icon.h>
#include <djvUI/Label.h>
#include <djvUI/RowLayout.h>
#include <djvUI/Spacer.h>
#include <djvUI/StackLayout.h>

#include <djvRender2D/Render.h>

#include <djvSystem/Animation.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace Layout
        {
            namespace
            {
                //! \todo Should this be configurable?
                const size_t animationTime = 100;

                class Button : public UI::Button::IButton
                {
                    DJV_NON_COPYABLE(Button);

                protected:
                    void _init(const std::shared_ptr<System::Context>&);
                    Button();

                public:
                    ~Button() override;

                    static std::shared_ptr<Button> create(const std::shared_ptr<System::Context>&);

                    void setIcon(const std::string&);

                    const std::string& getText() const { return _label->getText(); }
                    void setText(const std::string&);

                    float getHeightForWidth(float) const override;

                protected:
                    void _preLayoutEvent(System::Event::PreLayout&) override;
                    void _layoutEvent(System::Event::Layout&) override;
                    void _paintEvent(System::Event::Paint&) override;

                private:
                    std::shared_ptr<Icon> _icon;
                    std::shared_ptr<Text::Label> _label;
                    std::shared_ptr<HorizontalLayout> _layout;
                };

                void Button::_init(const std::shared_ptr<System::Context>& context)
                {
                    Widget::_init(context);

                    setClassName("djv::UI::Layout::Bellows::Button");
                    setButtonType(ButtonType::Toggle);

                    _icon = Icon::create(context);
                    _icon->setIcon("djvIconArrowSmallRight");
                    _icon->setIconSizeRole(MetricsRole::IconSmall);
                    _icon->setVAlign(VAlign::Center);

                    _label = Text::Label::create(context);
                    _label->setTextHAlign(TextHAlign::Left);

                    _layout = HorizontalLayout::create(context);
                    _layout->setSpacing(Spacing(MetricsRole::None));
                    auto hLayout = HorizontalLayout::create(context);
                    hLayout->setMargin(Margin(MetricsRole::MarginSmall));
                    hLayout->setSpacing(Spacing(MetricsRole::SpacingSmall));
                    hLayout->addChild(_icon);
                    hLayout->addChild(_label);
                    hLayout->setStretch(_label);
                    _layout->addChild(hLayout);
                    _layout->setStretch(hLayout);
                    addChild(_layout);
                }

                Button::Button()
                {}

                Button::~Button()
                {}

                std::shared_ptr<Button> Button::create(const std::shared_ptr<System::Context>& context)
                {
                    auto out = std::shared_ptr<Button>(new Button);
                    out->_init(context);
                    return out;
                }

                void Button::setIcon(const std::string& value)
                {
                    _icon->setIcon(value);
                }

                void Button::setText(const std::string& value)
                {
                    _label->setText(value);
                }

                float Button::getHeightForWidth(float value) const
                {
                    return _layout->getHeightForWidth(value);
                }

                void Button::_preLayoutEvent(System::Event::PreLayout&)
                {
                    _setMinimumSize(_layout->getMinimumSize());
                }

                void Button::_layoutEvent(System::Event::Layout&)
                {
                    _layout->setGeometry(getGeometry());
                }

                void Button::_paintEvent(System::Event::Paint& event)
                {
                    Widget::_paintEvent(event);
                    const Math::BBox2f& g = getGeometry();
                    const auto& render = _getRender();
                    const auto& style = _getStyle();
                    if (_isPressed())
                    {
                        render->setFillColor(style->getColor(ColorRole::Pressed));
                        render->drawRect(g);
                    }
                    else if (_isHovered())
                    {
                        render->setFillColor(style->getColor(ColorRole::Hovered));
                        render->drawRect(g);
                    }
                }

                class ChildLayout : public Widget
                {
                    DJV_NON_COPYABLE(ChildLayout);

                protected:
                    void _init(const std::shared_ptr<System::Context>&);
                    ChildLayout();

                public:
                    ~ChildLayout() override;

                    static std::shared_ptr<ChildLayout> create(const std::shared_ptr<System::Context>&);

                    void setOpen(float);

                    float getHeightForWidth(float) const override;

                    void addChild(const std::shared_ptr<IObject>&) override;
                    void removeChild(const std::shared_ptr<IObject>&) override;
                    void clearChildren() override;

                protected:
                    void _preLayoutEvent(System::Event::PreLayout&) override;
                    void _layoutEvent(System::Event::Layout&) override;

                private:
                    float _open = 1.F;
                    std::shared_ptr<StackLayout> _childLayout;
                    std::shared_ptr<VerticalLayout> _layout;
                };

                void ChildLayout::_init(const std::shared_ptr<System::Context>& context)
                {
                    Widget::_init(context);

                    _layout = VerticalLayout::create(context);
                    _layout->setSpacing(MetricsRole::None);
                    _childLayout = StackLayout::create(context);
                    _layout->addChild(_childLayout);
                    _layout->setStretch(_childLayout);
                    _layout->addSeparator();
                    Widget::addChild(_layout);
                }

                ChildLayout::ChildLayout()
                {}

                ChildLayout::~ChildLayout()
                {}

                std::shared_ptr<ChildLayout> ChildLayout::create(const std::shared_ptr<System::Context>& context)
                {
                    auto out = std::shared_ptr<ChildLayout>(new ChildLayout);
                    out->_init(context);
                    return out;
                }

                void ChildLayout::setOpen(float value)
                {
                    _open = value;
                    _resize();
                }

                float ChildLayout::getHeightForWidth(float value) const
                {
                    return _layout->getHeightForWidth(value) * _open;
                }

                void ChildLayout::addChild(const std::shared_ptr<IObject>& value)
                {
                    _childLayout->addChild(value);
                }

                void ChildLayout::removeChild(const std::shared_ptr<IObject>& value)
                {
                    _childLayout->removeChild(value);
                }

                void ChildLayout::clearChildren()
                {
                    _childLayout->clearChildren();
                }

                void ChildLayout::_preLayoutEvent(System::Event::PreLayout& event)
                {
                    glm::vec2 size = _layout->getMinimumSize();
                    _setMinimumSize(glm::vec2(size.x, size.y * _open));
                }

                void ChildLayout::_layoutEvent(System::Event::Layout& event)
                {
                    const Math::BBox2f& g = getGeometry();
                    glm::vec2 size = _layout->getMinimumSize();
                    _layout->setGeometry(Math::BBox2f(g.min.x, g.min.y, g.w(), size.y));
                }

            } // namespace

            struct Bellows::Private
            {
                std::shared_ptr<Button> button;
                std::shared_ptr<Layout::Spacer> spacer;
                std::shared_ptr<HorizontalLayout> buttonLayout;
                std::shared_ptr<ChildLayout> childLayout;
                std::shared_ptr<VerticalLayout> layout;
                bool open = false;
                std::shared_ptr<System::Animation::Animation> openAnimation;
                std::function<void(bool)> openCallback;
            };

            void Bellows::_init(const std::shared_ptr<System::Context>& context)
            {
                Widget::_init(context);

                DJV_PRIVATE_PTR();
                setClassName("djv::UI::Layout::Bellows");
                setVAlign(VAlign::Top);
                
                p.openAnimation = System::Animation::Animation::create(context);

                p.button = Button::create(context);

                p.spacer = Layout::Spacer::create(Orientation::Vertical, context);
                
                p.buttonLayout = HorizontalLayout::create(context);
                p.buttonLayout->setSpacing(MetricsRole::None);
                p.buttonLayout->setBackgroundRole(ColorRole::BackgroundBellows);
                p.buttonLayout->addChild(p.button);
                p.buttonLayout->setStretch(p.button);

                p.childLayout = ChildLayout::create(context);
                p.childLayout->setOpen(p.open);
                p.childLayout->setShadowOverlay({ Side::Top });
                p.childLayout->addChild(p.spacer);

                p.layout = VerticalLayout::create(context);
                p.layout->setSpacing(MetricsRole::None);
                p.layout->addChild(p.buttonLayout);
                p.layout->addSeparator();
                p.layout->addChild(p.childLayout);
                p.layout->setStretch(p.childLayout);
                Widget::addChild(p.layout);

                _widgetUpdate();

                auto weak = std::weak_ptr<Bellows>(std::dynamic_pointer_cast<Bellows>(shared_from_this()));
                p.button->setCheckedCallback(
                    [weak](bool value)
                {
                    if (auto widget = weak.lock())
                    {
                        widget->setOpen(value);
                        if (widget->_p->openCallback)
                        {
                            widget->_p->openCallback(value);
                        }
                    }
                });
            }

            Bellows::Bellows() :
                _p(new Private)
            {}

            Bellows::~Bellows()
            {}

            std::shared_ptr<Bellows> Bellows::create(const std::shared_ptr<System::Context>& context)
            {
                auto out = std::shared_ptr<Bellows>(new Bellows);
                out->_init(context);
                return out;
            }

            const std::string& Bellows::getText() const
            {
                return _p->button->getText();
            }

            void Bellows::setText(const std::string& text)
            {
                _p->button->setText(text);
            }

            bool Bellows::isOpen() const
            {
                return _p->button->isChecked();
            }

            void Bellows::setOpen(bool value, bool animate)
            {
                DJV_PRIVATE_PTR();
                if (value == p.open)
                    return;
                p.open = value;
                _widgetUpdate();
                if (animate)
                {
                    auto weak = std::weak_ptr<Bellows>(std::dynamic_pointer_cast<Bellows>(shared_from_this()));
                    if (p.open)
                    {
                        p.openAnimation->start(
                            0.F,
                            1.F,
                            std::chrono::milliseconds(animationTime),
                            [weak](float value)
                            {
                                if (auto widget = weak.lock())
                                {
                                    widget->_p->childLayout->setOpen(value);
                                }
                            },
                            [weak](float value)
                            {
                                if (auto widget = weak.lock())
                                {
                                    widget->_p->childLayout->setOpen(value);
                                }
                            });
                    }
                    else
                    {
                        p.openAnimation->start(
                            1.F,
                            0.F,
                            std::chrono::milliseconds(animationTime),
                            [weak](float value)
                            {
                                if (auto widget = weak.lock())
                                {
                                    widget->_p->childLayout->setOpen(value);
                                    widget->_resize();
                                }
                            },
                            [weak](float value)
                            {
                                if (auto widget = weak.lock())
                                {
                                    widget->_p->childLayout->setOpen(value);
                                    widget->_resize();
                                }
                            });
                    }
                }
                else
                {
                    p.childLayout->setOpen(value ? 1.F : 0.F);
                }
                _resize();
            }

            void Bellows::open(bool animate)
            {
                setOpen(true, animate);
            }

            void Bellows::close(bool animate)
            {
                setOpen(false, animate);
            }

            void Bellows::setOpenCallback(const std::function<void(bool)>& callback)
            {
                _p->openCallback = callback;
            }

            void Bellows::addButtonWidget(const std::shared_ptr<IObject>& value)
            {
                _p->buttonLayout->addChild(value);
            }

            void Bellows::removeButtonWidget(const std::shared_ptr<IObject>& value)
            {
                _p->buttonLayout->removeChild(value);
            }

            void Bellows::clearButtonWidgets()
            {
                _p->buttonLayout->clearChildren();
            }

            float Bellows::getHeightForWidth(float value) const
            {
                return _p->layout->getHeightForWidth(value);
            }

            void Bellows::addChild(const std::shared_ptr<IObject>& value)
            {
                _p->childLayout->addChild(value);
                _widgetUpdate();
            }

            void Bellows::removeChild(const std::shared_ptr<IObject>& value)
            {
                _p->childLayout->removeChild(value);
                _widgetUpdate();
            }

            void Bellows::clearChildren()
            {
                _p->childLayout->clearChildren();
                _widgetUpdate();
            }

            void Bellows::_preLayoutEvent(System::Event::PreLayout& event)
            {
                _setMinimumSize(_p->layout->getMinimumSize());
            }

            void Bellows::_layoutEvent(System::Event::Layout& event)
            {
                _p->layout->setGeometry(getGeometry());
            }

            void Bellows::_widgetUpdate()
            {
                DJV_PRIVATE_PTR();
                p.button->setChecked(p.open);
                p.button->setIcon(p.open ? "djvIconArrowSmallDown" : "djvIconArrowSmallRight");
                p.spacer->setVisible(p.childLayout->getChildWidgets().size() == 1);
            }

        } // namespace Layout
    } // namespace UI
} // namespace djv
