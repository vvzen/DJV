// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvUIComponents/FileBrowserPrivate.h>

#include <djvUI/ButtonGroup.h>
#include <djvUI/GridLayout.h>
#include <djvUI/ListButton.h>
#include <djvUI/RowLayout.h>
#include <djvUI/ScrollWidget.h>
#include <djvUI/ToolBar.h>
#include <djvUI/ToolButton.h>

using namespace djv::Core;

namespace djv
{
    namespace UIComponents
    {
        namespace FileBrowser
        {
            struct ShortcutsWidget::Private
            {
                System::File::Path path;
                bool edit = false;
                size_t textElide = 0;
                std::shared_ptr<UI::ToolButton> addButton;
                std::shared_ptr<UI::ToolButton> editButton;
                std::shared_ptr<UI::ButtonGroup> deleteButtonGroup;
                std::shared_ptr<UI::GridLayout> itemLayout;
                std::shared_ptr<UI::VerticalLayout> layout;
                std::function<void(const System::File::Path&)> callback;
                std::shared_ptr<Observer::List<System::File::Path> > shortcutsObserver;
            };

            void ShortcutsWidget::_init(
                const std::shared_ptr<ShortcutsModel> & model,
                size_t textElide,
                const std::shared_ptr<System::Context>& context)
            {
                Widget::_init(context);

                DJV_PRIVATE_PTR();
                setClassName("djv::UIComponents::FileBrowser::ShortcutsWidget");

                p.textElide = textElide;

                p.addButton = UI::ToolButton::create(context);
                p.addButton->setIcon("djvIconAddSmall");
                p.addButton->setInsideMargin(UI::MetricsRole::None);

                p.editButton = UI::ToolButton::create(context);
                p.editButton->setButtonType(UI::ButtonType::Toggle);
                p.editButton->setIcon("djvIconClearSmall");
                p.editButton->setInsideMargin(UI::MetricsRole::None);

                p.deleteButtonGroup = UI::ButtonGroup::create(UI::ButtonType::Push);
                
                p.layout = UI::VerticalLayout::create(context);
                p.layout->setSpacing(UI::MetricsRole::None);
                auto toolBar = UI::ToolBar::create(context);
                toolBar->addExpander();
                toolBar->addChild(p.addButton);
                toolBar->addChild(p.editButton);
                p.layout->addChild(toolBar);
                p.layout->addSeparator();
                p.itemLayout = UI::GridLayout::create(context);
                p.itemLayout->setSpacing(UI::MetricsRole::None);
                p.layout->addChild(p.itemLayout);
                p.layout->setStretch(p.itemLayout);
                addChild(p.layout);

                auto weak = std::weak_ptr<ShortcutsWidget>(std::dynamic_pointer_cast<ShortcutsWidget>(shared_from_this()));
                p.addButton->setClickedCallback(
                    [weak, model]
                    {
                        if (auto widget = weak.lock())
                        {
                            model->addShortcut(widget->_p->path);
                        }
                    });

                p.editButton->setCheckedCallback(
                    [weak](bool value)
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_p->edit = value;
                            for (const auto& i : widget->_p->deleteButtonGroup->getButtons())
                            {
                                i->setVisible(value);
                            }
                        }
                    });
                
                p.deleteButtonGroup->setPushCallback(
                    [model](int value)
                    {
                        model->removeShortcut(value);
                    });

                auto contextWeak = std::weak_ptr<System::Context>(context);
                p.shortcutsObserver = Observer::List<System::File::Path>::create(
                    model->observeShortcuts(),
                    [weak, contextWeak](const std::vector<System::File::Path> & value)
                    {
                        if (auto context = contextWeak.lock())
                        {
                            if (auto widget = weak.lock())
                            {
                                std::vector<std::shared_ptr<UI::Button::IButton> > deleteButtons;
                                widget->_p->itemLayout->clearChildren();
                                size_t j = 0;
                                for (const auto& i : value)
                                {
                                    auto button = UI::ListButton::create(context);
                                    button->setText(getPathLabel(i));
                                    button->setTextElide(widget->_p->textElide);
                                    button->setTooltip(i.get());

                                    auto deleteButton = UI::ToolButton::create(context);
                                    deleteButton->setIcon("djvIconClearSmall");
                                    deleteButton->setInsideMargin(UI::MetricsRole::None);
                                    deleteButton->setVisible(widget->_p->edit);
                                    deleteButtons.push_back(deleteButton);

                                    widget->_p->itemLayout->addChild(button);
                                    widget->_p->itemLayout->setGridPos(button, 0, j);
                                    widget->_p->itemLayout->setStretch(button, UI::GridStretch::Horizontal);
                                    widget->_p->itemLayout->addChild(deleteButton);
                                    widget->_p->itemLayout->setGridPos(deleteButton, 1, j);

                                    const auto path = i;
                                    button->setClickedCallback(
                                        [weak, path]
                                        {
                                            if (auto widget = weak.lock())
                                            {
                                                if (widget->_p->callback)
                                                {
                                                    widget->_p->callback(path);
                                                }
                                            }
                                        });

                                    ++j;
                                }
                                widget->_p->deleteButtonGroup->setButtons(deleteButtons);
                            }
                        }
                    });
            }

            ShortcutsWidget::ShortcutsWidget() :
                _p(new Private)
            {}

            ShortcutsWidget::~ShortcutsWidget()
            {}

            std::shared_ptr<ShortcutsWidget> ShortcutsWidget::create(
                const std::shared_ptr<ShortcutsModel> & model,
                size_t textElide,
                const std::shared_ptr<System::Context>& context)
            {
                auto out = std::shared_ptr<ShortcutsWidget>(new ShortcutsWidget);
                out->_init(model, textElide, context);
                return out;
            }

            void ShortcutsWidget::setPath(const System::File::Path& value)
            {
                _p->path = value;
            }

            void ShortcutsWidget::setCallback(const std::function<void(const System::File::Path&)> & value)
            {
                _p->callback = value;
            }

            void ShortcutsWidget::_preLayoutEvent(System::Event::PreLayout& event)
            {
                _setMinimumSize(_p->layout->getMinimumSize());
            }

            void ShortcutsWidget::_layoutEvent(System::Event::Layout& event)
            {
                _p->layout->setGeometry(getGeometry());
            }
            
            void ShortcutsWidget::_initEvent(System::Event::Init& event)
            {
                DJV_PRIVATE_PTR();
                if (event.getData().text)
                {
                    p.addButton->setTooltip(_getText(DJV_TEXT("file_browser_add_shortcut_tooltip")));
                    p.editButton->setTooltip(_getText(DJV_TEXT("file_browser_edit_shortcuts_tooltip")));
                }
            }

        } // namespace FileBrowser
    } // namespace UIComponents
} // namespace djv
