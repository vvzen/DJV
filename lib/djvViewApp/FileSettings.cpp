// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvViewApp/FileSettings.h>

#include <djvSystem/Context.h>
#include <djvSystem/FileInfoFunc.h>

#include <djvMath/BBoxFunc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// These need to be included last on macOS.
#include <djvCore/RapidJSONTemplates.h>
#include <djvUI/ISettingsTemplates.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        struct FileSettings::Private
        {
            std::shared_ptr<Observer::ValueSubject<size_t> > openMax;
            std::shared_ptr<Observer::ListSubject<System::File::Info> > recentFiles;
            std::shared_ptr<Observer::ValueSubject<size_t> > recentFilesMax;
            std::shared_ptr<Observer::ValueSubject<bool> > autoDetectSequences;
            std::shared_ptr<Observer::ValueSubject<bool> > sequencesFirstFrame;
            std::shared_ptr<Observer::ValueSubject<bool> > cacheEnabled;
            std::shared_ptr<Observer::ValueSubject<int> > cacheSize;
        };

        void FileSettings::_init(const std::shared_ptr<System::Context>& context)
        {
            ISettings::_init("djv::ViewApp::FileSettings", context);
            DJV_PRIVATE_PTR();
            p.openMax = Observer::ValueSubject<size_t>::create(16);
            p.recentFiles = Observer::ListSubject<System::File::Info>::create();
            p.recentFilesMax = Observer::ValueSubject<size_t>::create(10);
            p.autoDetectSequences = Observer::ValueSubject<bool>::create(true);
            p.sequencesFirstFrame = Observer::ValueSubject<bool>::create(true);
            p.cacheEnabled = Observer::ValueSubject<bool>::create(true);
            p.cacheSize = Observer::ValueSubject<int>::create(4);
            _load();
        }

        FileSettings::FileSettings() :
            _p(new Private)
        {}

        FileSettings::~FileSettings()
        {}

        std::shared_ptr<FileSettings> FileSettings::create(const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<FileSettings>(new FileSettings);
            out->_init(context);
            return out;
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > FileSettings::observeOpenMax() const
        {
            return _p->openMax;
        }

        void FileSettings::setOpenMax(size_t value)
        {
            _p->openMax->setIfChanged(value);
        }
        
        std::shared_ptr<Observer::IListSubject<System::File::Info> > FileSettings::observeRecentFiles() const
        {
            return _p->recentFiles;
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > FileSettings::observeRecentFilesMax() const
        {
            return _p->recentFilesMax;
        }

        void FileSettings::setRecentFiles(const std::vector<System::File::Info>& value)
        {
            DJV_PRIVATE_PTR();
            std::vector<System::File::Info> files;
            for (size_t i = 0; i < value.size() && i < p.recentFilesMax->get(); ++i)
            {
                files.push_back(value[i]);
            }
            p.recentFiles->setIfChanged(files);
        }

        void FileSettings::setRecentFilesMax(size_t value)
        {
            DJV_PRIVATE_PTR();
            p.recentFilesMax->setIfChanged(value);
            const auto& files = _p->recentFiles->get();
            std::vector<System::File::Info> filesMax;
            for (size_t i = 0; i < files.size() && i < value; ++i)
            {
                filesMax.push_back(files[i]);
            }
            setRecentFiles(filesMax);
        }

        std::shared_ptr<Observer::IValueSubject<bool> > FileSettings::observeAutoDetectSequences() const
        {
            return _p->autoDetectSequences;
        }

        std::shared_ptr<Observer::IValueSubject<bool> > FileSettings::observeSequencesFirstFrame() const
        {
            return _p->sequencesFirstFrame;
        }

        void FileSettings::setAutoDetectSequences(bool value)
        {
            _p->autoDetectSequences->setIfChanged(value);
        }

        void FileSettings::setSequencesFirstFrame(bool value)
        {
            _p->sequencesFirstFrame->setIfChanged(value);
        }

        std::shared_ptr<Observer::IValueSubject<bool> > FileSettings::observeCacheEnabled() const
        {
            return _p->cacheEnabled;
        }

        std::shared_ptr<Observer::IValueSubject<int> > FileSettings::observeCacheSize() const
        {
            return _p->cacheSize;
        }

        void FileSettings::setCacheEnabled(bool value)
        {
            _p->cacheEnabled->setIfChanged(value);
        }

        void FileSettings::setCacheSize(int value)
        {
            _p->cacheSize->setIfChanged(value);
        }

        void FileSettings::load(const rapidjson::Value & value)
        {
            if (value.IsObject())
            {
                DJV_PRIVATE_PTR();
                UI::Settings::read("OpenMax", value, p.openMax);
                std::vector< System::File::Info> recentFiles;
                UI::Settings::read("RecentFiles", value, recentFiles);
                auto i = recentFiles.begin();
                while (i != recentFiles.end())
                {
                    if (!i->doesExist())
                    {
                        i = recentFiles.erase(i);
                    }
                    else
                    {
                        ++i;
                    }
                }
                p.recentFiles->setIfChanged(recentFiles);
                UI::Settings::read("RecentFilesMax", value, p.recentFilesMax);
                UI::Settings::read("AutoDetectSequences", value, p.autoDetectSequences);
                UI::Settings::read("SequencesFirstFrame", value, p.sequencesFirstFrame);
                UI::Settings::read("CacheEnabled", value, p.cacheEnabled);
                UI::Settings::read("CacheSize", value, p.cacheSize);
            }
        }

        rapidjson::Value FileSettings::save(rapidjson::Document::AllocatorType& allocator)
        {
            DJV_PRIVATE_PTR();
            rapidjson::Value out(rapidjson::kObjectType);
            UI::Settings::write("OpenMax", p.openMax->get(), out, allocator);
            UI::Settings::write("RecentFiles", p.recentFiles->get(), out, allocator);
            UI::Settings::write("RecentFilesMax", p.recentFilesMax->get(), out, allocator);
            UI::Settings::write("AutoDetectSequences", p.autoDetectSequences->get(), out, allocator);
            UI::Settings::write("SequencesFirstFrame", p.sequencesFirstFrame->get(), out, allocator);
            UI::Settings::write("CacheEnabled", p.cacheEnabled->get(), out, allocator);
            UI::Settings::write("CacheSize", p.cacheSize->get(), out, allocator);
            return out;
        }

    } // namespace ViewApp
} // namespace djv

