// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvViewApp/Annotate.h>

#include <djvViewApp/Media.h>

#include <djvRender2D/Render.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        struct AnnotatePrimitive::Private
        {
            std::weak_ptr<System::Context> context;
            AnnotateOptions options;
        };
        
        void AnnotatePrimitive::_init(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            DJV_PRIVATE_PTR();
            p.context = context;
            p.options = options;
        }
        
        AnnotatePrimitive::AnnotatePrimitive() :
            _p(new Private)
        {}
        
        AnnotatePrimitive::~AnnotatePrimitive()
        {}
        
        const AnnotateOptions& AnnotatePrimitive::getOptions() const
        {
            return _p->options;
        }
        
        const std::weak_ptr<System::Context>& AnnotatePrimitive::getContext() const
        {
            return _p->context;
        }

        struct AnnotatePolyline::Private
        {
            std::vector<glm::vec2> points;
        };
        
        void AnnotatePolyline::_init(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            AnnotatePrimitive::_init(options, context);
        }
        
        AnnotatePolyline::AnnotatePolyline() :
            _p(new Private)
        {}
            
        AnnotatePolyline::~AnnotatePolyline()
        {}
        
        std::shared_ptr<AnnotatePolyline> AnnotatePolyline::create(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<AnnotatePolyline>(new AnnotatePolyline);
            out->_init(options, context);
            return out;
        }
            
        void AnnotatePolyline::draw(const std::shared_ptr<Render2D::Render>& render)
        {
            DJV_PRIVATE_PTR();
            const auto& options = getOptions();
            render->setFillColor(options.color);
            render->setLineWidth(options.lineSize);
            render->drawPolyline(p.points);
        }
        
        void AnnotatePolyline::addPoint(const glm::vec2& value)
        {
            _p->points.push_back(value);
        }

        struct AnnotateLine::Private
        {
            std::vector<glm::vec2> points;
        };

        void AnnotateLine::_init(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            AnnotatePrimitive::_init(options, context);
        }

        AnnotateLine::AnnotateLine() :
            _p(new Private)
        {}

        AnnotateLine::~AnnotateLine()
        {}

        std::shared_ptr<AnnotateLine> AnnotateLine::create(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<AnnotateLine>(new AnnotateLine);
            out->_init(options, context);
            return out;
        }

        void AnnotateLine::draw(const std::shared_ptr<Render2D::Render>& render)
        {
            DJV_PRIVATE_PTR();
            const auto& options = getOptions();
            render->setFillColor(options.color);
            render->setLineWidth(options.lineSize);
            render->drawPolyline(p.points);
        }

        void AnnotateLine::addPoint(const glm::vec2& value)
        {
            while (_p->points.size() > 1)
            {
                _p->points.pop_back();
            }
            _p->points.push_back(value);
        }

        struct AnnotateRectangle::Private
        {
            std::vector<glm::vec2> points;
        };

        void AnnotateRectangle::_init(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            AnnotatePrimitive::_init(options, context);
        }

        AnnotateRectangle::AnnotateRectangle() :
            _p(new Private)
        {}

        AnnotateRectangle::~AnnotateRectangle()
        {}

        std::shared_ptr<AnnotateRectangle> AnnotateRectangle::create(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<AnnotateRectangle>(new AnnotateRectangle);
            out->_init(options, context);
            return out;
        }

        void AnnotateRectangle::draw(const std::shared_ptr<Render2D::Render>& render)
        {
            DJV_PRIVATE_PTR();
            const auto& options = getOptions();
            render->setFillColor(options.color);
            render->setLineWidth(options.lineSize);
            render->drawPolyline(p.points);
        }

        void AnnotateRectangle::addPoint(const glm::vec2& value)
        {
            _p->points.push_back(value);
        }

        struct AnnotateEllipse::Private
        {
            std::vector<glm::vec2> points;
        };

        void AnnotateEllipse::_init(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            AnnotatePrimitive::_init(options, context);
        }

        AnnotateEllipse::AnnotateEllipse() :
            _p(new Private)
        {}

        AnnotateEllipse::~AnnotateEllipse()
        {}

        std::shared_ptr<AnnotateEllipse> AnnotateEllipse::create(const AnnotateOptions& options, const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<AnnotateEllipse>(new AnnotateEllipse);
            out->_init(options, context);
            return out;
        }

        void AnnotateEllipse::draw(const std::shared_ptr<Render2D::Render>& render)
        {
            DJV_PRIVATE_PTR();
            const auto& options = getOptions();
            render->setFillColor(options.color);
            render->setLineWidth(options.lineSize);
            render->drawPolyline(p.points);
        }

        void AnnotateEllipse::addPoint(const glm::vec2& value)
        {
            _p->points.push_back(value);
        }
        
        struct AnnotateCommand::Private
        {
            std::shared_ptr<AnnotatePrimitive> primitive;
            std::weak_ptr<Media> media;
            std::shared_ptr<Observer::ValueSubject<bool> > undo;
        };

        void AnnotateCommand::_init(
            const std::shared_ptr<AnnotatePrimitive>& primitive,
            const std::shared_ptr<Media>& media)
        {
            DJV_PRIVATE_PTR();
            p.primitive = primitive;
            p.media = media;
            p.undo = Observer::ValueSubject<bool>::create();
        }

        AnnotateCommand::AnnotateCommand() :
            _p(new Private)
        {}

        std::shared_ptr<AnnotateCommand> AnnotateCommand::create(
            const std::shared_ptr<AnnotatePrimitive>& primitive,
            const std::shared_ptr<Media>& media)
        {
            auto out = std::shared_ptr<AnnotateCommand>(new AnnotateCommand);
            out->_init(primitive, media);
            return out;
        }

        std::shared_ptr<Core::Observer::IValueSubject<bool> > AnnotateCommand::observeUndo() const
        {
            return _p->undo;
        }

        const std::shared_ptr<AnnotatePrimitive>& AnnotateCommand::getPrimitive() const
        {
            return _p->primitive;
        }

        const std::weak_ptr<Media>& AnnotateCommand::getMedia() const
        {
            return _p->media;
        }

        void AnnotateCommand::exec()
        {
            DJV_PRIVATE_PTR();
            if (auto media = p.media.lock())
            {
                media->addAnnotation(p.primitive);
            }
        }

        void AnnotateCommand::undo()
        {
            DJV_PRIVATE_PTR();
            if (auto media = p.media.lock())
            {
                media->removeAnnotation(p.primitive);
            }
            p.undo->setAlways(true);
        }

    } // namespace ViewApp
} // namespace djv
