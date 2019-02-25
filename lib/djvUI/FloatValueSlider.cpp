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

#include <djvUI/FloatValueSlider.h>

#include <djvAV/Render2D.h>

#include <djvCore/NumericValueModels.h>
#include <djvCore/ValueObserver.h>

using namespace djv::Core;

namespace djv
{
    namespace UI
    {
        namespace Slider
        {
            struct FloatValue::Private
            {
                std::shared_ptr<FloatValueModel> model;
                Orientation orientation = Orientation::First;
                float handleWidth = 0.f;
                Event::PointerID pressedID = Event::InvalidID;
                glm::vec2 pressedPos = glm::vec2(0.f, 0.f);
                std::shared_ptr<ValueObserver<float> > valueObserver;
            };

            void FloatValue::_init(Orientation orientation, Context * context)
            {
                Widget::_init(context);

                DJV_PRIVATE_PTR();

                setClassName("djv::UI::Slider::FloatValue");
                setModel(FloatValueModel::create());
                setPointerEnabled(true);

                p.orientation = orientation;
            }

            FloatValue::FloatValue() :
                _p(new Private)
            {}

            FloatValue::~FloatValue()
            {}

            std::shared_ptr<FloatValue> FloatValue::create(Orientation orientation, Context * context)
            {
                auto out = std::shared_ptr<FloatValue>(new FloatValue);
                out->_init(orientation, context);
                return out;
            }

            const std::shared_ptr<FloatValueModel> & FloatValue::getModel() const
            {
                return _p->model;
            }

            void FloatValue::setModel(const std::shared_ptr<FloatValueModel> & model)
            {
                DJV_PRIVATE_PTR();
                if (p.model)
                {
                    p.valueObserver.reset();
                }
                p.model = model;
                if (p.model)
                {
                    auto weak = std::weak_ptr<FloatValue>(std::dynamic_pointer_cast<FloatValue>(shared_from_this()));
                    p.valueObserver = ValueObserver<float>::create(
                        p.model->observeValue(),
                        [weak](float value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_redraw();
                        }
                    });
                }
            }

            void FloatValue::_preLayoutEvent(Event::PreLayout& event)
            {
                DJV_PRIVATE_PTR();
                if (auto style = _getStyle().lock())
                {
                    const float tc = style->getMetric(MetricsRole::TextColumn);
                    p.handleWidth = style->getMetric(MetricsRole::Handle);
                    _setMinimumSize(glm::vec2(tc, p.handleWidth) + getMargin().getSize(style));
                }
            }

            void FloatValue::_paintEvent(Event::Paint& event)
            {
                Widget::_paintEvent(event);
                DJV_PRIVATE_PTR();
                if (auto render = _getRender().lock())
                {
                    if (auto style = _getStyle().lock())
                    {
                        const BBox2f & g = getMargin().bbox(getGeometry(), style);
                        const glm::vec2 c = g.getCenter();
                        const float m = style->getMetric(MetricsRole::MarginSmall);
                        render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Trough)));
                        render->drawRect(BBox2f(g.min.x + p.handleWidth / 2.f - m / 2.f, c.y - m / 2.f, g.w() - p.handleWidth + m, m));
                        if (p.model)
                        {
                            render->setFillColor(_getColorWithOpacity(style->getColor(_getColorRole())));
                            const glm::vec2 pos(_valueToPos(p.model->observeValue()->get()), c.y);
                            const float r = ceilf(p.handleWidth / 2.f - 1.f);
                            render->drawCircle(pos, r);
                            if (p.pressedID != Event::InvalidID)
                            {
                                render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Pressed)));
                                render->drawCircle(pos, r);
                            }
                            else if (_getPointerHover().size())
                            {
                                render->setFillColor(_getColorWithOpacity(style->getColor(ColorRole::Hovered)));
                                render->drawCircle(pos, r);
                            }
                        }
                    }
                }
            }

            void FloatValue::_pointerEnterEvent(Event::PointerEnter& event)
            {
                if (!event.isRejected())
                {
                    event.accept();
                    if (isEnabled(true))
                    {
                        _redraw();
                    }
                }
            }

            void FloatValue::_pointerLeaveEvent(Event::PointerLeave& event)
            {
                event.accept();
                if (isEnabled(true))
                {
                    _redraw();
                }
            }

            void FloatValue::_pointerMoveEvent(Event::PointerMove& event)
            {
                DJV_PRIVATE_PTR();
                event.accept();
                if (p.model)
                {
                    const auto & pointerInfo = event.getPointerInfo();
                    if (pointerInfo.id == p.pressedID)
                    {
                        float v = 0.f;
                        switch (p.orientation)
                        {
                        case Orientation::Horizontal:
                            v = pointerInfo.projectedPos.x;
                            break;
                        case Orientation::Vertical:
                            v = pointerInfo.projectedPos.y;
                            break;
                        default: break;
                        }
                        p.model->setValue(_posToValue(v));
                    }
                }
            }

            void FloatValue::_buttonPressEvent(Event::ButtonPress& event)
            {
                DJV_PRIVATE_PTR();
                if (p.pressedID)
                    return;
                event.accept();
                if (p.model)
                {
                    const auto & pointerInfo = event.getPointerInfo();
                    p.pressedID = pointerInfo.id;
                    p.pressedPos = pointerInfo.projectedPos;
                    float v = 0.f;
                    switch (p.orientation)
                    {
                    case Orientation::Horizontal:
                        v = pointerInfo.projectedPos.x;
                        break;
                    case Orientation::Vertical:
                        v = pointerInfo.projectedPos.y;
                        break;
                    default: break;
                    }
                    p.model->setValue(_posToValue(v));
                    _redraw();
                }
            }

            void FloatValue::_buttonReleaseEvent(Event::ButtonRelease& event)
            {
                DJV_PRIVATE_PTR();
                const auto & pointerInfo = event.getPointerInfo();
                if (pointerInfo.id == p.pressedID)
                {
                    event.accept();
                    p.pressedID = Event::InvalidID;
                    _redraw();
                }
            }

            ColorRole FloatValue::_getColorRole() const
            {
                DJV_PRIVATE_PTR();
                return !isEnabled(true) ? ColorRole::Disabled : ColorRole::Button;
            }

            float FloatValue::_valueToPos(float value) const
            {
                DJV_PRIVATE_PTR();
                float out = 0.f;
                if (p.model)
                {
                    const BBox2f & g = getGeometry();
                    const auto & range = p.model->observeRange()->get();
                    const float v = (value - range.min) / static_cast<float>(range.max - range.min);
                    switch (p.orientation)
                    {
                    case Orientation::Horizontal:
                        out = g.x() + ceilf(p.handleWidth / 2.f + (g.w() - p.handleWidth) * v);
                        break;
                    case Orientation::Vertical:
                        out = g.y() + ceilf(p.handleWidth / 2.f + (g.h() - p.handleWidth) * v);
                        break;
                    default: break;
                    }
                }
                return out;
            }

            float FloatValue::_posToValue(float value) const
            {
                DJV_PRIVATE_PTR();
                float out = 0.f;
                if (p.model)
                {
                    const BBox2f & g = getGeometry();
                    const auto & range = p.model->observeRange()->get();
                    float v = 0.f;
                    switch (p.orientation)
                    {
                    case Orientation::Horizontal:
                        v = Math::clamp((value - g.x() - p.handleWidth / 2.f) / (g.w() - p.handleWidth), 0.f, 1.f);
                        break;
                    case Orientation::Vertical:
                        v = Math::clamp((value - g.y() - p.handleWidth / 2.f) / (g.h() - p.handleWidth), 0.f, 1.f);
                        break;
                    default: break;
                    }
                    out = v * (range.max - range.min) + range.min;
                }
                return out;
            }

        } // namespace Slider
    } // namespace UI
} // namespace djv
