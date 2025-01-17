// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#pragma once

#include <djvUI/Enum.h>
#include <djvUI/ISettings.h>
#include <djvUI/ShortcutData.h>

#include <djvSystem/FileInfo.h>

#include <djvCore/ListObserver.h>
#include <djvCore/MapObserver.h>
#include <djvCore/ValueObserver.h>

namespace djv
{
    namespace Image
    {
        class Size;

    } // namespace Image

    namespace UIComponents
    {
        namespace Settings
        {
            //! This class provides the file browser settings.
            class FileBrowser : public UI::Settings::ISettings
            {
                DJV_NON_COPYABLE(FileBrowser);

            protected:
                void _init(const std::shared_ptr<System::Context>& context);

                FileBrowser();

            public:
                ~FileBrowser() override;

                static std::shared_ptr<FileBrowser> create(const std::shared_ptr<System::Context>&);

                //! \name Splitter
                ///@{

                std::shared_ptr<Core::Observer::IListSubject<float> > observeSplit() const;

                void setSplit(const std::vector<float>&);

                ///@}

                //! \name Paths
                ///@{

                std::shared_ptr<Core::Observer::IValueSubject<bool> > observePathsOpen() const;
                std::shared_ptr<Core::Observer::IMapSubject<std::string, bool> > observePathsBellowsState() const;

                void setPathsOpen(bool);
                void setPathsBellowsState(const std::map<std::string, bool>&);

                ///@}

                //! \name Shortcuts
                ///@{

                std::shared_ptr<Core::Observer::IListSubject<System::File::Path> > observeShortcuts() const;
                
                void setShortcuts(const std::vector<System::File::Path>&);

                ///@}

                //! \name Recent Paths
                ///@{

                std::shared_ptr<Core::Observer::IListSubject<System::File::Path> > observeRecentPaths() const;
                
                void setRecentPaths(const std::vector<System::File::Path>&);

                ///@}

                //! \name Options
                ///@{

                std::shared_ptr<Core::Observer::IValueSubject<UI::ViewType> > observeViewType() const;
                std::shared_ptr<Core::Observer::IValueSubject<Image::Size> > observeThumbnailSize() const;
                std::shared_ptr<Core::Observer::IListSubject<float> > observeListViewHeaderSplit() const;
                std::shared_ptr<Core::Observer::IValueSubject<bool> > observeFileSequences() const;
                std::shared_ptr<Core::Observer::IValueSubject<bool> > observeShowHidden() const;
                std::shared_ptr<Core::Observer::IValueSubject<System::File::DirectoryListSort> > observeSort() const;
                std::shared_ptr<Core::Observer::IValueSubject<bool> > observeReverseSort() const;
                std::shared_ptr<Core::Observer::IValueSubject<bool> > observeSortDirectoriesFirst() const;

                void setViewType(UI::ViewType);
                void setThumbnailSize(const Image::Size&);
                void setListViewHeaderSplit(const std::vector<float> &);
                void setFileSequences(bool);
                void setShowHidden(bool);
                void setSort(System::File::DirectoryListSort);
                void setReverseSort(bool);
                void setSortDirectoriesFirst(bool);

                ///@}

                //! \name Keyboard Shortcuts
                ///@{

                std::shared_ptr<Core::Observer::MapSubject<std::string, UI::ShortcutDataPair> > observeKeyShortcuts() const;
                
                void setKeyShortcuts(const UI::ShortcutDataMap&);

                ///@}

                void load(const rapidjson::Value &) override;
                rapidjson::Value save(rapidjson::Document::AllocatorType&) override;

            private:
                DJV_PRIVATE();
            };

        } // namespace Settings
    } // namespace UIComponents
} // namespace djv
