// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvUI/MDICanvas.h>

#include <djvUI/Icon.h>
#include <djvUI/Label.h>
#include <djvUI/MDIWidget.h>
#include <djvUI/RowLayout.h>

#include <djvRender2D/Render.h>

#include <djvSystem/Animation.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace MDI
        {
            namespace
            {
                //! \todo Should this be configurable?
                const size_t maximizeAnimationDuration = 500;

                struct Hovered
                {
                    std::shared_ptr<IWidget> widget;
                    Handle handle = Handle::None;
                };

                struct Pressed
                {
                    std::shared_ptr<IWidget> widget;
                    glm::vec2 pointer = glm::vec2(0.F, 0.F);
                    Handle handle = Handle::None;
                    glm::vec2 pos = glm::vec2(0.F, 0.F);
                    glm::vec2 size = glm::vec2(0.F, 0.F);
                };

            } // namespace

            struct Canvas::Private
            {
                glm::vec2 canvasSize = glm::vec2(10000.F, 10000.F);
                std::map<std::shared_ptr<IWidget>, bool> widgetInit;
                std::map<std::shared_ptr<IWidget>, Math::BBox2f> widgetToGeometry;
                std::map<System::Event::PointerID, Hovered> hovered;
                std::map<System::Event::PointerID, Pressed> pressed;
                std::shared_ptr<IWidget> activeWidget;
                std::function<void(const std::shared_ptr<IWidget>&)> activeCallback;
                bool maximize = false;
                float maximizeValue = 0.F;
                std::weak_ptr<IWidget> maximizeWidget;
                std::function<void(bool)> maximizeCallback;
                std::shared_ptr<System::Animation::Animation> maximizeAnimation;
                std::map<Handle, std::vector<Math::BBox2f> > handles;
            };

            void Canvas::_init(const std::shared_ptr<System::Context>& context)
            {
                Widget::_init(context);
                DJV_PRIVATE_PTR();

                setClassName("djv::UI::MDI::Canvas");

                p.maximizeAnimation = System::Animation::Animation::create(context);
                p.maximizeAnimation->setType(System::Animation::Type::SmoothStep);
            }

            Canvas::Canvas() :
                _p(new Private)
            {}

            Canvas::~Canvas()
            {}

            std::shared_ptr<Canvas> Canvas::create(const std::shared_ptr<System::Context>& context)
            {
                auto out = std::shared_ptr<Canvas>(new Canvas);
                out->_init(context);
                return out;
            }

            const glm::vec2& Canvas::getCanvasSize() const
            {
                return _p->canvasSize;
            }

            void Canvas::setCanvasSize(const glm::vec2& size)
            {
                DJV_PRIVATE_PTR();
                if (size == p.canvasSize)
                    return;
                p.canvasSize = size;
                _resize();
            }

            const std::shared_ptr<IWidget>& Canvas::getActiveWidget() const
            {
                return _p->activeWidget;
            }

            void Canvas::nextWidget()
            {
                const auto& children = getChildrenT<IWidget>();
                const size_t size = children.size();
                if (size > 1)
                {
                    children.back()->moveToBack();
                }
            }

            void Canvas::prevWidget()
            {
                const auto& children = getChildrenT<IWidget>();
                const size_t size = children.size();
                if (size > 1)
                {
                    children.front()->moveToFront();
                }
            }

            void Canvas::setActiveCallback(const std::function<void(const std::shared_ptr<IWidget>&)>& value)
            {
                _p->activeCallback = value;
            }

            glm::vec2 Canvas::getWidgetPos(const std::shared_ptr<IWidget>& widget) const
            {
                DJV_PRIVATE_PTR();
                const auto i = p.widgetToGeometry.find(widget);
                return i != p.widgetToGeometry.end() ? i->second.min : glm::vec2(0.F, 0.F);
            }

            void Canvas::setWidgetPos(const std::shared_ptr<IWidget>& widget, const glm::vec2& pos)
            {
                DJV_PRIVATE_PTR();
                auto i = p.widgetToGeometry.find(widget);
                if (i != p.widgetToGeometry.end())
                {
                    i->second.min = pos;
                    p.widgetInit[widget] = false;
                    _resize();
                }
            }

            void Canvas::setWidgetGeometry(const std::shared_ptr<IWidget>& widget, const Math::BBox2f& geometry)
            {
                DJV_PRIVATE_PTR();
                auto i = p.widgetToGeometry.find(widget);
                if (i != p.widgetToGeometry.end())
                {
                    i->second = geometry;
                    p.widgetInit[widget] = false;
                    _resize();
                }
            }

            bool Canvas::isMaximized() const
            {
                return _p->maximize;
            }

            namespace
            {
                Math::BBox2f lerp(float value, const Math::BBox2f& min, const Math::BBox2f& max)
                {
                    return Math::BBox2f(
                        glm::vec2(
                            floorf(Math::lerp(value, min.min.x, max.min.x)),
                            floorf(Math::lerp(value, min.min.y, max.min.y))),
                        glm::vec2(
                            ceilf(Math::lerp(value, min.max.x, max.max.x)),
                            ceilf(Math::lerp(value, min.max.y, max.max.y))));
                }

            } // namespace

            void Canvas::setMaximize(bool value)
            {
                DJV_PRIVATE_PTR();
                if (value == p.maximize)
                    return;
                p.maximize = value;
                const auto i = p.widgetToGeometry.find(p.activeWidget);
                if (i != p.widgetToGeometry.end())
                {
                    const auto& style = _getStyle();
                    const float sh = style->getMetric(MetricsRole::Shadow);
                    const Math::BBox2f canvasGeometry = getGeometry().margin(sh);
                    p.maximizeWidget = p.activeWidget;
                    auto maximizeWeak = p.maximizeWidget;
                    const glm::vec2 widgetMinimumSize = p.activeWidget->getMinimumSize();
                    const Math::BBox2f maximizeWidgetGeometry(
                        i->second.min.x,
                        i->second.min.y,
                        std::max(i->second.w(), widgetMinimumSize.x),
                        std::max(i->second.h(), widgetMinimumSize.y));
                    auto weak = std::weak_ptr<Canvas>(std::dynamic_pointer_cast<Canvas>(shared_from_this()));
                    p.maximizeAnimation->start(
                        p.maximize ? 0.F : 1.F,
                        p.maximize ? 1.F : 0.F,
                        std::chrono::milliseconds(maximizeAnimationDuration),
                        [weak, canvasGeometry, maximizeWeak, maximizeWidgetGeometry](float value)
                    {
                        if (auto canvas = weak.lock())
                        {
                            canvas->_p->maximizeValue = value;
                            if (auto maximizeWidget = maximizeWeak.lock())
                            {
                                maximizeWidget->_setMaximize(value);
                                maximizeWidget->setGeometry(lerp(value, maximizeWidgetGeometry, canvasGeometry));
                            }
                        }
                    },
                        [weak, canvasGeometry, maximizeWeak, maximizeWidgetGeometry](float value)
                    {
                        if (auto canvas = weak.lock())
                        {
                            canvas->_p->maximizeValue = value;
                            if (auto maximizeWidget = maximizeWeak.lock())
                            {
                                maximizeWidget->_setMaximize(value);
                                maximizeWidget->setGeometry(lerp(value, maximizeWidgetGeometry, canvasGeometry));
                            }
                            canvas->_p->maximizeWidget.reset();
                        }
                    });
                }
                else
                {
                    p.maximizeValue = value ? 1.F : 0.F;
                }
                if (p.maximizeCallback)
                {
                    p.maximizeCallback(p.maximize);
                }
                _resize();
            }

            void Canvas::setMaximizeCallback(const std::function<void(bool)>& value)
            {
                _p->maximizeCallback = value;
            }

            void Canvas::_preLayoutEvent(System::Event::PreLayout&)
            {
                _setMinimumSize(_p->canvasSize);
            }

            void Canvas::_layoutEvent(System::Event::Layout&)
            {
                DJV_PRIVATE_PTR();
                const auto& style = _getStyle();
                const float sh = style->getMetric(MetricsRole::Shadow);
                const Math::BBox2f& g = getGeometry();
                for (auto& i : p.widgetToGeometry)
                {
                    auto maximizeWidget = p.maximizeWidget.lock();
                    if (i.first != maximizeWidget)
                    {
                        const glm::vec2& widgetMinimumSize = i.first->getMinimumSize();
                        glm::vec2 widgetSize = i.first->getSize();
                        if (p.widgetInit[i.first])
                        {
                            p.widgetInit[i.first] = false;
                            widgetSize.x = ceilf(std::max(widgetSize.x, widgetMinimumSize.x));
                            widgetSize.y = ceilf(std::max(widgetSize.y, widgetMinimumSize.y));
                            const glm::vec2 c = g.getCenter();
                            i.second.min.x = floorf(c.x - widgetSize.x / 2.F);
                            i.second.min.y = floorf(c.y - widgetSize.y / 2.F);
                            i.second.max.x = i.second.min.x + widgetSize.x;
                            i.second.max.y = i.second.min.y + widgetSize.y;
                        }
                        Math::BBox2f widgetGeometry;
                        if (p.maximize && i.first == p.activeWidget)
                        {
                            widgetGeometry = g.margin(sh);
                        }
                        else
                        {
                            float x = Math::clamp(i.second.min.x, g.min.x, g.max.x - widgetMinimumSize.x);
                            float y = Math::clamp(i.second.min.y, g.min.y, g.max.y - widgetMinimumSize.y);
                            i.second.min.x = x;
                            i.second.min.y = y;
                            i.second.max.x = Math::clamp(x + widgetSize.x, x + widgetMinimumSize.x, g.max.x);
                            i.second.max.y = Math::clamp(y + widgetSize.y, y + widgetMinimumSize.y, g.max.y);
                            widgetGeometry = i.second;
                        }
                        i.first->setGeometry(widgetGeometry);
                    }
                }
            }

            void Canvas::_childAddedEvent(System::Event::ChildAdded& value)
            {
                DJV_PRIVATE_PTR();
                if (auto widget = std::dynamic_pointer_cast<IWidget>(value.getChild()))
                {
                    if (p.activeWidget)
                    {
                        p.activeWidget->_setActiveWidget(false);
                        p.activeWidget->_setMaximize(0.F);
                    }
                    p.activeWidget = widget;
                    p.activeWidget->_setActiveWidget(true);
                    p.activeWidget->_setMaximize(p.maximizeValue);
                    p.activeWidget->installEventFilter(shared_from_this());
                    const auto i = p.widgetToGeometry.find(p.activeWidget);
                    if (i == p.widgetToGeometry.end())
                    {
                        p.widgetInit[p.activeWidget] = true;
                        p.widgetToGeometry[p.activeWidget] = Math::BBox2f(0.F, 0.F, 0.F, 0.F);
                    }
                    _resize();
                    _doActiveCallback();
                }
            }

            void Canvas::_childRemovedEvent(System::Event::ChildRemoved& value)
            {
                DJV_PRIVATE_PTR();
                if (auto widget = std::dynamic_pointer_cast<IWidget>(value.getChild()))
                {
                    widget->removeEventFilter(shared_from_this());
                    if (widget == p.activeWidget)
                    {
                        if (p.activeWidget)
                        {
                            p.activeWidget->_setActiveWidget(false);
                            p.activeWidget->_setMaximize(0.F);
                        }
                        const auto& children = getChildrenT<IWidget>();
                        p.activeWidget = children.size() ? children.back() : nullptr;
                        if (p.activeWidget)
                        {
                            p.activeWidget->_setActiveWidget(true);
                            p.activeWidget->_setMaximize(p.maximize);
                        }
                        _doActiveCallback();
                    }
                    {
                        const auto i = p.widgetInit.find(widget);
                        if (i != p.widgetInit.end())
                        {
                            p.widgetInit.erase(i);
                        }
                    }
                    {
                        const auto i = p.widgetToGeometry.find(widget);
                        if (i != p.widgetToGeometry.end())
                        {
                            p.widgetToGeometry.erase(i);
                        }
                    }
                    _resize();
                }
            }

            void Canvas::_childOrderEvent(System::Event::ChildOrder&)
            {
                DJV_PRIVATE_PTR();
                const auto& children = getChildrenT<IWidget>();
                if (children.size())
                {
                    auto widget = children.back();
                    if (widget != p.activeWidget)
                    {
                        if (p.activeWidget)
                        {
                            p.activeWidget->_setActiveWidget(false);
                            p.activeWidget->_setMaximize(0.F);
                            const auto i = p.widgetToGeometry.find(p.activeWidget);
                            if (i != p.widgetToGeometry.end())
                            {
                                i->first->setGeometry(i->second);
                            }
                        }
                        widget->_setMaximize(p.maximizeValue);
                        if (p.maximize)
                        {
                            const auto& style = _getStyle();
                            const float sh = style->getMetric(MetricsRole::Shadow);
                            widget->setGeometry(getGeometry().margin(sh));
                        }
                        p.activeWidget = widget;
                        p.activeWidget->_setActiveWidget(true);
                        _doActiveCallback();
                    }
                }
            }

            bool Canvas::_eventFilter(const std::shared_ptr<IObject>& object, System::Event::Event& event)
            {
                DJV_PRIVATE_PTR();
                switch (event.getEventType())
                {
                case System::Event::Type::PointerEnter:
                {
                    System::Event::PointerEnter& pointerEnterEvent = static_cast<System::Event::PointerEnter&>(event);
                    const auto& pointerInfo = pointerEnterEvent.getPointerInfo();
                    if (auto widget = std::dynamic_pointer_cast<IWidget>(object))
                    {
                        p.handles = widget->_getHandles();
                        _redraw();
                        for (const auto& handle : p.handles)
                        {
                            for (const auto& rect : handle.second)
                            {
                                if (rect.contains(pointerInfo.projectedPos))
                                {
                                    event.accept();
                                    Hovered hovered;
                                    hovered.widget = widget;
                                    hovered.handle = handle.first;
                                    p.hovered[pointerInfo.id] = hovered;
                                    widget->_setHandleHovered(handle.first);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case System::Event::Type::PointerLeave:
                {
                    System::Event::PointerLeave& pointerLeaveEvent = static_cast<System::Event::PointerLeave&>(event);
                    const auto& pointerInfo = pointerLeaveEvent.getPointerInfo();
                    const auto i = p.hovered.find(pointerInfo.id);
                    if (i != p.hovered.end())
                    {
                        event.accept();
                        i->second.widget->_setHandleHovered(Handle::None);
                        p.hovered.erase(i);
                    }
                    p.handles.clear();
                    _redraw();
                    break;
                }
                case System::Event::Type::PointerMove:
                {
                    event.accept();
                    System::Event::PointerMove& pointerMoveEvent = static_cast<System::Event::PointerMove&>(event);
                    const auto& pointerInfo = pointerMoveEvent.getPointerInfo();
                    if (auto widget = std::dynamic_pointer_cast<IWidget>(object))
                    {
                        const auto i = p.pressed.find(pointerInfo.id);
                        if (i != p.pressed.end())
                        {
                            const auto j = p.widgetToGeometry.find(widget);
                            if (j != p.widgetToGeometry.end())
                            {
                                const glm::vec2 widgetSize = j->second.getSize();
                                const glm::vec2& minimumSize = widget->getMinimumSize();
                                const glm::vec2 d = pointerInfo.projectedPos - i->second.pointer;
                                const glm::vec2 d2(
                                    d.x - std::max(0.F, minimumSize.x - (i->second.size.x - d.x)),
                                    d.y - std::max(0.F, minimumSize.y - (i->second.size.y - d.y)));
                                switch (i->second.handle)
                                {
                                case Handle::Move:
                                    j->second.min = i->second.pos + pointerInfo.projectedPos - i->second.pointer;
                                    j->second.max = j->second.min + widgetSize;
                                    break;
                                case Handle::ResizeE:
                                    j->second.min.x = i->second.pos.x + d2.x;
                                    break;
                                case Handle::ResizeN:
                                    j->second.min.y = i->second.pos.y + d2.y;
                                    break;
                                case Handle::ResizeW:
                                    j->second.max.x = i->second.pos.x + i->second.size.x + d.x;
                                    break;
                                case Handle::ResizeS:
                                    j->second.max.y = i->second.pos.y + i->second.size.y + d.y;
                                    break;
                                case Handle::ResizeNE:
                                    j->second.min = i->second.pos + d2;
                                    break;
                                case Handle::ResizeNW:
                                    j->second.max.x = i->second.pos.x + i->second.size.x + d.x;
                                    j->second.min.y = i->second.pos.y + d2.y;
                                    break;
                                case Handle::ResizeSW:
                                    j->second.max = i->second.pos + i->second.size + d;
                                    break;
                                case Handle::ResizeSE:
                                    j->second.min.x = i->second.pos.x + d2.x;
                                    j->second.max.y = i->second.pos.y + i->second.size.y + d.y;
                                    break;
                                default: break;
                                }
                                widget->setGeometry(j->second);
                            }
                        }
                        else
                        {
                            p.handles = widget->_getHandles();
                            _redraw();
                            for (const auto& handle : p.handles)
                            {
                                for (const auto& rect : handle.second)
                                {
                                    if (rect.contains(pointerInfo.projectedPos))
                                    {
                                        Hovered hovered;
                                        hovered.widget = widget;
                                        hovered.handle = handle.first;
                                        const auto j = p.hovered.find(pointerInfo.id);
                                        if (j != p.hovered.end() && (j->second.widget != widget || j->second.handle != handle.first))
                                        {
                                            auto k = p.hovered.find(pointerInfo.id);
                                            if (k != p.hovered.end())
                                            {
                                                k->second.widget->_setHandleHovered(Handle::None);
                                            }
                                            p.hovered[pointerInfo.id] = hovered;
                                            widget->_setHandleHovered(handle.first);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    return true;
                }
                case System::Event::Type::ButtonPress:
                {
                    System::Event::ButtonPress& buttonPressEvent = static_cast<System::Event::ButtonPress&>(event);
                    const auto& pointerInfo = buttonPressEvent.getPointerInfo();
                    if (auto widget = std::dynamic_pointer_cast<IWidget>(object))
                    {
                        const auto i = p.widgetToGeometry.find(widget);
                        if (i != p.widgetToGeometry.end())
                        {
                            for (const auto& handle : widget->_getHandles())
                            {
                                for (const auto& rect : handle.second)
                                {
                                    if (rect.contains(pointerInfo.projectedPos))
                                    {
                                        event.accept();
                                        Pressed pressed;
                                        pressed.widget = widget;
                                        pressed.pointer = pointerInfo.projectedPos;
                                        pressed.handle = handle.first;
                                        pressed.pos = i->second.min;
                                        pressed.size = widget->getSize();
                                        p.pressed[pointerInfo.id] = pressed;
                                        widget->_setHandlePressed(handle.first);
                                        widget->moveToFront();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    return true;
                }
                case System::Event::Type::ButtonRelease:
                {
                    System::Event::ButtonRelease& buttonReleaseEvent = static_cast<System::Event::ButtonRelease&>(event);
                    const auto& pointerInfo = buttonReleaseEvent.getPointerInfo();
                    const auto i = p.pressed.find(pointerInfo.id);
                    if (i != p.pressed.end())
                    {
                        event.accept();
                        i->second.widget->_setHandlePressed(Handle::None);
                        p.pressed.erase(i);
                    }
                    return true;
                }
                default: break;
                }
                return false;
            }

            void Canvas::_doActiveCallback()
            {
                DJV_PRIVATE_PTR();
                if (p.activeCallback)
                {
                    p.activeCallback(p.activeWidget);
                }
            }

        } // namespace MDI
    } // namespace UI
} // namespace djdv
