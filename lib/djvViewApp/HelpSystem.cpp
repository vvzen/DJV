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

#include <djvViewApp/HelpSystem.h>

#include <djvViewApp/AboutDialog.h>
#include <djvViewApp/DebugWidget.h>
#include <djvViewApp/SystemLogWidget.h>

#include <djvUI/Action.h>
#include <djvUI/EventSystem.h>
#include <djvUI/Menu.h>
#include <djvUI/RowLayout.h>
#include <djvUI/Shortcut.h>
#include <djvUI/Window.h>

#include <djvCore/Context.h>
#include <djvCore/TextSystem.h>

#include <GLFW/glfw3.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        struct HelpSystem::Private
        {
            std::map<std::string, std::shared_ptr<UI::Action> > actions;
            std::shared_ptr<UI::Menu> menu;
            std::shared_ptr<AboutDialog> aboutDialog;
            std::map<std::string, std::shared_ptr<ValueObserver<bool> > > clickedObservers;
            std::shared_ptr<ValueObserver<std::string> > localeObserver;
        };

        void HelpSystem::_init(Context * context)
        {
            IViewSystem::_init("djv::ViewApp::HelpSystem", context);

            DJV_PRIVATE_PTR();

            //! \todo Implement me!
            p.actions["Documentation"] = UI::Action::create();
            p.actions["Documentation"]->setEnabled(false);
            p.actions["About"] = UI::Action::create();
            p.actions["SystemLog"] = UI::Action::create();
            p.actions["SystemLog"]->setButtonType(UI::ButtonType::Toggle);
            p.actions["Debug"] = UI::Action::create();
            p.actions["Debug"]->setButtonType(UI::ButtonType::Toggle);
            p.actions["Debug"]->setShortcut(GLFW_KEY_D, UI::Shortcut::getSystemModifier());

            p.menu = UI::Menu::create(context);
            p.menu->addAction(p.actions["Documentation"]);
            p.menu->addSeparator();
            p.menu->addAction(p.actions["About"]);
            p.menu->addSeparator();
            p.menu->addAction(p.actions["SystemLog"]);
            p.menu->addAction(p.actions["Debug"]);

            auto weak = std::weak_ptr<HelpSystem>(std::dynamic_pointer_cast<HelpSystem>(shared_from_this()));
            _setCloseWidgetCallback(
                [weak](const std::string & name)
            {
                if (auto system = weak.lock())
                {
                    const auto i = system->_p->actions.find(name);
                    if (i != system->_p->actions.end())
                    {
                        i->second->setChecked(false);
                    }
                }
            });

            p.clickedObservers["About"] = ValueObserver<bool>::create(
                p.actions["About"]->observeClicked(),
                [weak, context](bool value)
            {
                if (value)
                {
                    if (auto system = weak.lock())
                    {
                        if (auto windowSystem = context->getSystemT<UI::EventSystem>())
                        {
                            if (auto window = windowSystem->getCurrentWindow().lock())
                            {
                                if (!system->_p->aboutDialog)
                                {
                                    system->_p->aboutDialog = AboutDialog::create(context);
                                    system->_p->aboutDialog->setCloseCallback(
                                        [weak]
                                    {
                                        if (auto system = weak.lock())
                                        {
                                            if (auto parent = system->_p->aboutDialog->getParent().lock())
                                            {
                                                parent->removeChild(system->_p->aboutDialog);
                                            }
                                            system->_p->aboutDialog.reset();
                                        }
                                    });
                                }
                                window->addChild(system->_p->aboutDialog);
                                system->_p->aboutDialog->show();
                            }
                        }
                    }
                }
            });

            p.clickedObservers["SystemLog"] = ValueObserver<bool>::create(
                p.actions["SystemLog"]->observeChecked(),
                [weak, context](bool value)
                {
                    if (auto system = weak.lock())
                    {
                        if (value)
                        {
                            auto widget = SystemLogWidget::create(context);
                            widget->reloadLog();
                            system->_openWidget("SystemLog", widget);
                        }
                        else
                        {
                            system->_closeWidget("SystemLog");
                        }
                    }
                });

            p.clickedObservers["Debug"] = ValueObserver<bool>::create(
                p.actions["Debug"]->observeChecked(),
                [weak, context](bool value)
                {
                    if (auto system = weak.lock())
                    {
                        if (value)
                        {
                            auto widget = DebugWidget::create(context);
                            system->_openWidget("Debug", widget);
                        }
                        else
                        {
                            system->_closeWidget("Debug");
                        }
                    }
                });

            p.localeObserver = ValueObserver<std::string>::create(
                context->getSystemT<TextSystem>()->observeCurrentLocale(),
                [weak](const std::string & value)
            {
                if (auto system = weak.lock())
                {
                    system->_textUpdate();
                }
            });
        }

        HelpSystem::HelpSystem() :
            _p(new Private)
        {}

        HelpSystem::~HelpSystem()
        {}

        std::shared_ptr<HelpSystem> HelpSystem::create(Context * context)
        {
            auto out = std::shared_ptr<HelpSystem>(new HelpSystem);
            out->_init(context);
            return out;
        }

        std::map<std::string, std::shared_ptr<UI::Action> > HelpSystem::getActions() const
        {
            return _p->actions;
        }

        MenuData HelpSystem::getMenu() const
        {
            return
            {
                _p->menu,
                "I"
            };
        }

        void HelpSystem::_textUpdate()
        {
            DJV_PRIVATE_PTR();
            p.actions["Documentation"]->setText(_getText(DJV_TEXT("Documentation")));
            p.actions["Documentation"]->setTooltip(_getText(DJV_TEXT("Documentation tooltip")));
            p.actions["About"]->setText(_getText(DJV_TEXT("About")));
            p.actions["About"]->setTooltip(_getText(DJV_TEXT("About tooltip")));
            p.actions["SystemLog"]->setText(_getText(DJV_TEXT("System Log")));
            p.actions["SystemLog"]->setTooltip(_getText(DJV_TEXT("System log tooltip")));
            p.actions["Debug"]->setText(_getText(DJV_TEXT("Debug Widget")));
            p.actions["Debug"]->setTooltip(_getText(DJV_TEXT("Debug widget tooltip")));

            p.menu->setText(_getText(DJV_TEXT("Help")));
        }

    } // namespace ViewApp
} // namespace djv

