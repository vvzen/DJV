//------------------------------------------------------------------------------
// Copyright (c) 2018 Darby Johnston
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

#include <djvViewLib/PlaybackSystem.h>

#include <djvViewLib/Application.h>
#include <djvViewLib/Media.h>

#include <djvUI/Action.h>
#include <djvUI/Menu.h>

#include <djvCore/Context.h>

#include <GLFW/glfw3.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewLib
    {
        struct PlaybackSystem::Private
        {
            std::shared_ptr<Media> media;
            std::map<std::string, std::shared_ptr<UI::Action> > actions;
            std::shared_ptr<UI::ActionGroup> playbackActionGroup;
            std::map<std::string, std::shared_ptr<ValueObserver<bool> > > clickedObservers;
            std::shared_ptr<ValueObserver<Playback> > playbackObserver;
        };

        void PlaybackSystem::_init(Context * context)
        {
            IViewSystem::_init("djv::ViewLib::PlaybackSystem", context);

            DJV_PRIVATE_PTR();
            p.actions["Stop"] = UI::Action::create();
            p.actions["Stop"]->setIcon("djvIconPlaybackStop");
            p.actions["Stop"]->setText("Stop");
            p.actions["Stop"]->setShortcut(GLFW_KEY_K);
            p.actions["Stop"]->setEnabled(false);

            p.actions["Forward"] = UI::Action::create();
            p.actions["Forward"]->setIcon("djvIconPlaybackForward");
            p.actions["Forward"]->setText("Forward");
            p.actions["Forward"]->setShortcut(GLFW_KEY_L);
            p.actions["Forward"]->setEnabled(false);

            p.actions["Reverse"] = UI::Action::create();
            p.actions["Reverse"]->setIcon("djvIconPlaybackReverse");
            p.actions["Reverse"]->setText("Reverse");
            p.actions["Reverse"]->setShortcut(GLFW_KEY_J);
            p.actions["Reverse"]->setEnabled(false);

            p.playbackActionGroup = UI::ActionGroup::create(UI::ButtonType::Radio);
            p.playbackActionGroup->addAction(p.actions["Stop"]);
            p.playbackActionGroup->addAction(p.actions["Forward"]);
            p.playbackActionGroup->addAction(p.actions["Reverse"]);

            //! \todo Implement me!
            p.actions["TogglePlayback"] = UI::Action::create();
            p.actions["TogglePlayback"]->setText("Toggle Playback");
            p.actions["TogglePlayback"]->addShortcut(GLFW_KEY_SPACE);
            p.actions["TogglePlayback"]->addShortcut(GLFW_KEY_UP);
            p.actions["TogglePlayback"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["ToggleReversePlayback"] = UI::Action::create();
            p.actions["ToggleReversePlayback"]->setText("Toggle Reverse Playback");
            p.actions["ToggleReversePlayback"]->setShortcut(GLFW_KEY_DOWN);
            p.actions["ToggleReversePlayback"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["PlayEveryFrame"] = UI::Action::create();
            p.actions["PlayEveryFrame"]->setButtonType(UI::ButtonType::Toggle);
            p.actions["PlayEveryFrame"]->setText("Play Every Frame");
            p.actions["PlayEveryFrame"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["InPoint"] = UI::Action::create();
            p.actions["InPoint"]->setIcon("djvIconFrameStart");
            p.actions["InPoint"]->setText("In Point");
            p.actions["InPoint"]->setShortcut(GLFW_KEY_HOME);
            p.actions["InPoint"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["OutPoint"] = UI::Action::create();
            p.actions["OutPoint"]->setIcon("djvIconFrameEnd");
            p.actions["OutPoint"]->setText("Out Point");
            p.actions["OutPoint"]->setShortcut(GLFW_KEY_END);
            p.actions["OutPoint"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["StartFrame"] = UI::Action::create();
            p.actions["StartFrame"]->setText("Start Frame");
            p.actions["StartFrame"]->setShortcut(GLFW_KEY_HOME, GLFW_MOD_SHIFT);
            p.actions["StartFrame"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["EndFrame"] = UI::Action::create();
            p.actions["EndFrame"]->setText("End Frame");
            p.actions["EndFrame"]->setShortcut(GLFW_KEY_END, GLFW_MOD_SHIFT);
            p.actions["EndFrame"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["NextFrame"] = UI::Action::create();
            p.actions["NextFrame"]->setIcon("djvIconFrameNext");
            p.actions["NextFrame"]->setText("Next Frame");
            p.actions["NextFrame"]->addShortcut(GLFW_KEY_RIGHT);
            p.actions["NextFrame"]->addShortcut(GLFW_KEY_RIGHT_BRACKET);
            p.actions["NextFrame"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["NextFrame10"] = UI::Action::create();
            p.actions["NextFrame10"]->setText("Next Frame X10");
            p.actions["NextFrame10"]->addShortcut(GLFW_KEY_RIGHT, GLFW_MOD_SHIFT);
            p.actions["NextFrame10"]->addShortcut(GLFW_KEY_RIGHT_BRACKET, GLFW_MOD_SHIFT);
            p.actions["NextFrame10"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["NextFrame100"] = UI::Action::create();
            p.actions["NextFrame100"]->setText("Next Frame X100");
            p.actions["NextFrame100"]->addShortcut(GLFW_KEY_RIGHT, GLFW_MOD_CONTROL);
            p.actions["NextFrame100"]->addShortcut(GLFW_KEY_RIGHT_BRACKET, GLFW_MOD_CONTROL);
            p.actions["NextFrame100"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["PrevFrame"] = UI::Action::create();
            p.actions["PrevFrame"]->setIcon("djvIconFramePrev");
            p.actions["PrevFrame"]->setText("Previous Frame");
            p.actions["PrevFrame"]->addShortcut(GLFW_KEY_LEFT);
            p.actions["PrevFrame"]->addShortcut(GLFW_KEY_LEFT_BRACKET);
            p.actions["PrevFrame"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["PrevFrame10"] = UI::Action::create();
            p.actions["PrevFrame10"]->setText("Previous Frame X10");
            p.actions["PrevFrame10"]->addShortcut(GLFW_KEY_LEFT, GLFW_MOD_SHIFT);
            p.actions["PrevFrame10"]->addShortcut(GLFW_KEY_LEFT_BRACKET, GLFW_MOD_SHIFT);
            p.actions["PrevFrame10"]->setEnabled(false);

            //! \todo Implement me!
            p.actions["PrevFrame100"] = UI::Action::create();
            p.actions["PrevFrame100"]->setText("Previous Frame X100");
            p.actions["PrevFrame100"]->addShortcut(GLFW_KEY_LEFT, GLFW_MOD_CONTROL);
            p.actions["PrevFrame100"]->addShortcut(GLFW_KEY_LEFT_BRACKET, GLFW_MOD_CONTROL);
            p.actions["PrevFrame100"]->setEnabled(false);


            auto weak = std::weak_ptr<PlaybackSystem>(std::dynamic_pointer_cast<PlaybackSystem>(shared_from_this()));
            p.playbackActionGroup->setRadioCallback(
                [weak](int value)
            {
                if (auto system = weak.lock())
                {
                    if (system->_p->media)
                    {
                        system->_p->media->setPlayback(static_cast<Playback>(value));
                    }
                }
            });
        }

        PlaybackSystem::PlaybackSystem() :
            _p(new Private)
        {}

        PlaybackSystem::~PlaybackSystem()
        {}

        std::shared_ptr<PlaybackSystem> PlaybackSystem::create(Context * context)
        {
            auto out = std::shared_ptr<PlaybackSystem>(new PlaybackSystem);
            out->_init(context);
            return out;
        }

        std::map<std::string, std::shared_ptr<UI::Action> > PlaybackSystem::getActions()
        {
            return _p->actions;
        }

        std::string PlaybackSystem::getMenuSortKey() const
        {
            return "4";
        }

        std::shared_ptr<UI::Menu> PlaybackSystem::createMenu()
        {
            DJV_PRIVATE_PTR();
            auto context = getContext();
            auto menu = UI::Menu::create("Playback", context);
            menu->addAction(p.actions["Stop"]);
            menu->addAction(p.actions["Forward"]);
            menu->addAction(p.actions["Reverse"]);
            //! \todo Implement me!
            auto loopMenu = UI::Menu::create(DJV_TEXT("Loop"), context);
            menu->addMenu(loopMenu);
            menu->addAction(p.actions["TogglePlayback"]);
            menu->addAction(p.actions["ToggleReversePlayback"]);
            menu->addAction(p.actions["PlayEveryFrame"]);
            menu->addSeparator();
            menu->addAction(p.actions["InPoint"]);
            menu->addAction(p.actions["OutPoint"]);
            menu->addAction(p.actions["StartFrame"]);
            menu->addAction(p.actions["EndFrame"]);
            menu->addAction(p.actions["NextFrame"]);
            menu->addAction(p.actions["NextFrame10"]);
            menu->addAction(p.actions["NextFrame100"]);
            menu->addAction(p.actions["PrevFrame"]);
            menu->addAction(p.actions["PrevFrame10"]);
            menu->addAction(p.actions["PrevFrame100"]);
            return menu;
        }

        void PlaybackSystem::setCurrentMedia(const std::shared_ptr<Media> & media)
        {
            DJV_PRIVATE_PTR();
            p.media = media;
            p.actions["Stop"]->setEnabled(media ? true : false);
            p.actions["Forward"]->setEnabled(media ? true : false);
            p.actions["Reverse"]->setEnabled(media ? true : false);
            if (media)
            {
                auto weak = std::weak_ptr<PlaybackSystem>(std::dynamic_pointer_cast<PlaybackSystem>(shared_from_this()));
                p.playbackObserver = ValueObserver<Playback>::create(
                    media->getPlayback(),
                    [weak](Playback value)
                {
                    if (auto system = weak.lock())
                    {
                        system->_p->playbackActionGroup->setChecked(static_cast<int>(value));
                    }
                });
            }
            else
            {
                p.playbackActionGroup->setChecked(0);
                p.playbackObserver.reset();
            }
        }

        /*struct PlaybackObject::Private
        {
            std::map<QString, QPointer<QAction> > actions;
            std::map<QString, QPointer<QActionGroup> > actionGroups;
            std::shared_ptr<Core::ValueObserver<AV::Duration> > durationObserver;
            std::shared_ptr<Core::ValueObserver<AV::Timestamp> > currentTimeObserver;
            std::shared_ptr<Core::ValueObserver<Enum::Playback> > playbackObserver;
            std::shared_ptr<Media> currentMedia;
            std::vector<QPointer<TimelineWidget> > timelineWidgets;
        };
        
        PlaybackObject::PlaybackObject(const std::shared_ptr<Context> & context, QObject * parent) :
            IViewObject("PlaybackObject", context, parent),
            _p(new Private)
        {
            DJV_PRIVATE_PTR();            
            p.actions["Stop"] = new QAction("Stop", this);
            p.actions["Stop"]->setCheckable(true);
            p.actions["Stop"]->setShortcut(QKeySequence("K"));
            p.actions["Forward"] = new QAction("Forward", this);
            p.actions["Forward"]->setCheckable(true);
            p.actions["Forward"]->setShortcut(QKeySequence("L"));
            p.actions["Reverse"] = new QAction("Reverse", this);
            p.actions["Reverse"]->setCheckable(true);
            p.actions["Reverse"]->setShortcut(QKeySequence("J"));
            p.actionGroups["Playback"] = new QActionGroup(this);
            p.actionGroups["Playback"]->setExclusive(true);
            p.actionGroups["Playback"]->addAction(p.actions["Stop"]);
            p.actionGroups["Playback"]->addAction(p.actions["Forward"]);
            p.actionGroups["Playback"]->addAction(p.actions["Reverse"]);

            p.actions["Timeline"] = new QAction("Timeline", this);
            p.actions["Timeline"]->setCheckable(true);
            p.actions["Timeline"]->setChecked(true);

            connect(
                p.actionGroups["Playback"],
                &QActionGroup::triggered,
                [this](QAction * action)
            {
                DJV_PRIVATE_PTR();
                if (p.currentMedia)
                {
                    Enum::Playback playback = Enum::Playback::Stop;
                    if (p.actions["Forward"] == action)
                    {
                        playback = Enum::Playback::Forward;
                    }
                    else if (p.actions["Reverse"] == action)
                    {
                        playback = Enum::Playback::Reverse;
                    }
                    p.currentMedia->setPlayback(playback);
                }
            });
        }
        
        PlaybackObject::~PlaybackObject()
        {}

        std::string PlaybackObject::getMenuSortKey() const
        {
            return "5";
        }
        
        QPointer<QMenu> PlaybackObject::createMenu()
        {
            DJV_PRIVATE_PTR();
            auto menu = new QMenu("Playback");
            menu->addAction(p.actions["Stop"]);
            menu->addAction(p.actions["Forward"]);
            menu->addAction(p.actions["Reverse"]);
            menu->addSeparator();
            menu->addAction(p.actions["Timeline"]);
            return menu;
        }

        QPointer<QDockWidget> PlaybackObject::createDockWidget()
        {
            DJV_PRIVATE_PTR();
            QDockWidget * out = nullptr;
            if (auto context = getContext().lock())
            {
                out = new QDockWidget("Timeline");
                auto timelineWidget = new TimelineWidget(context);
                timelineWidget->setEnabled(p.currentMedia ? true : false);
                out->setWidget(timelineWidget);
                p.timelineWidgets.push_back(timelineWidget);

                connect(
                    p.actions["Timeline"],
                    &QAction::toggled,
                    [out](bool value)
                {
                    out->setVisible(value);
                });

                connect(
                    out,
                    &QDockWidget::visibilityChanged,
                    [this](bool value)
                {
                    _p->actions["Timeline"]->setChecked(value);
                });

                connect(
                    timelineWidget,
                    &TimelineWidget::currentTimeChanged,
                    [this](AV::Timestamp value)
                {
                    if (_p->currentMedia)
                    {
                        _p->currentMedia->setCurrentTime(value);
                    }
                });
                connect(
                    timelineWidget,
                    &TimelineWidget::playbackChanged,
                    [this](Enum::Playback value)
                {
                    if (_p->currentMedia)
                    {
                        _p->currentMedia->setPlayback(value);
                    }
                });
            }
            return out;
        }

        Qt::DockWidgetArea PlaybackObject::getDockWidgetArea() const
        {
            return Qt::DockWidgetArea::BottomDockWidgetArea;
        }

        bool PlaybackObject::isDockWidgetVisible() const
        {
            return true;
        }

        void PlaybackObject::setCurrentMedia(const std::shared_ptr<Media> & media)
        {
            DJV_PRIVATE_PTR();
            p.actionGroups["Playback"]->setEnabled(media ? true : false);
            for (auto & i : p.timelineWidgets)
            {
                i->setEnabled(media ? true : false);
            }

            if (media)
            {
                p.durationObserver = Core::ValueObserver<AV::Duration>::create(
                    media->getDuration(),
                    [this](AV::Duration value)
                {
                    for (auto & i : _p->timelineWidgets)
                    {
                        i->setDuration(value);
                    }
                });
                p.currentTimeObserver = Core::ValueObserver<AV::Timestamp>::create(
                    media->getCurrentTime(),
                    [this](AV::Timestamp value)
                {
                    for (auto & i : _p->timelineWidgets)
                    {
                        i->setCurrentTime(value);
                    }
                });
                p.playbackObserver = Core::ValueObserver<Enum::Playback>::create(
                    media->getPlayback(),
                    [this](Enum::Playback value)
                {
                    switch (value)
                    {
                    case Enum::Playback::Stop: _p->actions["Stop"]->setChecked(true); break;
                    case Enum::Playback::Forward: _p->actions["Forward"]->setChecked(true); break;
                    case Enum::Playback::Reverse: _p->actions["Reverse"]->setChecked(true); break;
                    default: break;
                    }
                    for (auto & i : _p->timelineWidgets)
                    {
                        i->setPlayback(value);
                    }
                });
            }
            else
            {
                p.currentTimeObserver = nullptr;
                p.playbackObserver = nullptr;
                for (auto & i : p.timelineWidgets)
                {
                    i->setDuration(0);
                    i->setCurrentTime(0);
                    i->setPlayback(Enum::Playback::Stop);
                }
            }
            p.currentMedia = media;
        }*/

    } // namespace ViewLib
} // namespace djv
