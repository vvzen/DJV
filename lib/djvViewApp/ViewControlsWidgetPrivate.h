// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston
// All rights reserved.

#pragma once

#include <djvUI/ToolButton.h>

namespace djv
{
    namespace ViewApp
    {
        class ViewControlsViewWidget : public UI::Widget
        {
            DJV_NON_COPYABLE(ViewControlsViewWidget);

        protected:
            void _init(const std::shared_ptr<System::Context>&);
            ViewControlsViewWidget();

        public:
            ~ViewControlsViewWidget() override;

            static std::shared_ptr<ViewControlsViewWidget> create(const std::shared_ptr<System::Context>&);

        protected:
            void _preLayoutEvent(System::Event::PreLayout&) override;
            void _layoutEvent(System::Event::Layout&) override;

            void _initEvent(System::Event::Init&) override;

        private:
            void _setPos(const glm::vec2&);
            void _setZoom(float);
            void _widgetUpdate();

            DJV_PRIVATE();
        };

        class ViewControlsGridWidget : public UI::Widget
        {
            DJV_NON_COPYABLE(ViewControlsGridWidget);

        protected:
            void _init(const std::shared_ptr<System::Context>&);
            ViewControlsGridWidget();

        public:
            ~ViewControlsGridWidget() override;

            static std::shared_ptr<ViewControlsGridWidget> create(const std::shared_ptr<System::Context>&);

            const std::shared_ptr<UI::ToolButton>& getEnabledButton() const;

        protected:
            void _preLayoutEvent(System::Event::PreLayout&) override;
            void _layoutEvent(System::Event::Layout&) override;

            void _initEvent(System::Event::Init&) override;

        private:
            void _widgetUpdate();

            DJV_PRIVATE();
        };

        class ViewControlsHUDWidget : public UI::Widget
        {
            DJV_NON_COPYABLE(ViewControlsHUDWidget);

        protected:
            void _init(const std::shared_ptr<System::Context>&);
            ViewControlsHUDWidget();

        public:
            ~ViewControlsHUDWidget() override;

            static std::shared_ptr<ViewControlsHUDWidget> create(const std::shared_ptr<System::Context>&);

            const std::shared_ptr<UI::ToolButton>& getEnabledButton() const;

        protected:
            void _preLayoutEvent(System::Event::PreLayout&) override;
            void _layoutEvent(System::Event::Layout&) override;

            void _initEvent(System::Event::Init&) override;

        private:
            void _widgetUpdate();

            DJV_PRIVATE();
        };

        class ViewControlsBackgroundWidget : public UI::Widget
        {
            DJV_NON_COPYABLE(ViewControlsBackgroundWidget);

        protected:
            void _init(const std::shared_ptr<System::Context>&);
            ViewControlsBackgroundWidget();

        public:
            ~ViewControlsBackgroundWidget() override;

            static std::shared_ptr<ViewControlsBackgroundWidget> create(const std::shared_ptr<System::Context>&);

        protected:
            void _preLayoutEvent(System::Event::PreLayout&) override;
            void _layoutEvent(System::Event::Layout&) override;

            void _initEvent(System::Event::Init&) override;

        private:
            void _widgetUpdate();

            DJV_PRIVATE();
        };

        class ViewControlsBorderWidget : public UI::Widget
        {
            DJV_NON_COPYABLE(ViewControlsBorderWidget);

        protected:
            void _init(const std::shared_ptr<System::Context>&);
            ViewControlsBorderWidget();

        public:
            ~ViewControlsBorderWidget() override;

            static std::shared_ptr<ViewControlsBorderWidget> create(const std::shared_ptr<System::Context>&);

            const std::shared_ptr<UI::ToolButton>& getEnabledButton() const;

        protected:
            void _preLayoutEvent(System::Event::PreLayout&) override;
            void _layoutEvent(System::Event::Layout&) override;

            void _initEvent(System::Event::Init&) override;

        private:
            void _widgetUpdate();

            DJV_PRIVATE();
        };

    } // namespace ViewApp
} // namespace djv

