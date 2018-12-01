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

#include <djvCore/DirectoryModel.h>

#include <djvCore/DirectoryWatcher.h>
#include <djvCore/FileInfo.h>
#include <djvCore/Timer.h>
#include <djvCore/OS.h>

#include <future>

namespace djv
{
    namespace
    {
        //! \todo [1.0 S] Should this be configurable?
        const size_t timeout = 100;
        const size_t historyMax = 10;

    } // namespace

    namespace Core
    {
        struct DirectoryModel::Private
        {
            std::shared_ptr<ValueSubject<Path> > path;
            std::shared_ptr<ValueSubject<bool> > fileSequences;
            std::shared_ptr<ListSubject<FileInfo> > list;
            std::shared_ptr<ListSubject<std::string> > fileNames;
            std::shared_ptr<ListSubject<Path> > history;
            std::future<std::pair<std::vector<FileInfo>, std::vector<std::string> > > future;
            std::shared_ptr<Timer> futureTimer;
            std::shared_ptr<DirectoryWatcher> directoryWatcher;
        };

        void DirectoryModel::_init(const std::shared_ptr<Context>& context)
        {
            _p->path = ValueSubject<Path>::create();
            _p->fileSequences = ValueSubject<bool>::create(true);
            _p->list = ListSubject<FileInfo>::create();
            _p->fileNames = ListSubject<std::string>::create();
            _p->history = ListSubject<Path>::create();

            _p->futureTimer = Timer::create(context);
            _p->futureTimer->setRepeating(true);

            _p->directoryWatcher = DirectoryWatcher::create(context);

            auto weak = std::weak_ptr<DirectoryModel>(shared_from_this());
            _p->directoryWatcher->setCallback(
                [weak]
            {
                if (auto model = weak.lock())
                {
                    model->reload();
                }
            });
        }

        DirectoryModel::DirectoryModel() :
            _p(new Private)
        {}

        DirectoryModel::~DirectoryModel()
        {}
        
        std::shared_ptr<DirectoryModel> DirectoryModel::create(const std::shared_ptr<Context>& context)
        {
            auto out = std::shared_ptr<DirectoryModel>(new DirectoryModel);
            out->_init(context);
            return out;
        }

        std::shared_ptr<DirectoryModel> DirectoryModel::create(const Path& path, const std::shared_ptr<Context>& context)
        {
            auto out = std::shared_ptr<DirectoryModel>(new DirectoryModel);
            out->_init(context);
            out->setPath(path);
            return out;
        }

        std::shared_ptr<IValueSubject<Path> > DirectoryModel::getPath() const
        {
            return _p->path;
        }

        void DirectoryModel::setPath(const Path& value)
        {
            const auto& path = _p->path->get();
            if (value == path)
                return;

            if (!path.isEmpty())
            {
                std::vector<Path> history = _p->history->get();
                if (history.size())
                {
                    if (path != history[0])
                    {
                        history.insert(history.begin(), path);
                    }
                }
                else
                {
                    history.push_back(path);
                }
                while (history.size() > historyMax)
                {
                    history.pop_back();
                }
                _p->history->setIfChanged(history);
            }
            
            _p->path->setIfChanged(value);            

            _updatePath();
        }

        std::shared_ptr<IValueSubject<bool> > DirectoryModel::getFileSequences() const
        {
            return _p->fileSequences;
        }

        void DirectoryModel::setFileSequences(bool value)
        {
            if (value == _p->fileSequences->get())
                return;

            _p->fileSequences->setIfChanged(value);

            _updatePath();
        }

        std::shared_ptr<IListSubject<FileInfo> > DirectoryModel::getList() const
        {
            return _p->list;
        }

        std::shared_ptr<IListSubject<std::string> > DirectoryModel::getFileNames() const
        {
            return _p->fileNames;
        }

        std::shared_ptr<IListSubject<Path> > DirectoryModel::getHistory() const
        {
            return _p->history;
        }

        void DirectoryModel::reload()
        {
            _updatePath();
        }

        void DirectoryModel::goUp()
        {
            setPath(Path::upDir(_p->path->get()));
        }

        void DirectoryModel::_updatePath()
        {
            _p->list->clear();
            _p->fileNames->clear();
            
            const Path path = _p->path->get();
            const bool fileSequences = _p->fileSequences->get();
            _p->future = std::async(
                std::launch::async,
                [path, fileSequences]
            {
                std::pair<std::vector<FileInfo>, std::vector<std::string> > out;
                DirListOptions options;
                options.fileSequences = fileSequences;
                out.first = FileInfo::dirList(path, options);
                for (const auto& fileInfo : out.first)
                {
                    out.second.push_back(fileInfo.getFileName(-1, false));
                }
                return out;
            });

            _p->futureTimer->start(
                std::chrono::milliseconds(timeout),
                [this](float)
            {
                if (_p->future.valid() &&
                    _p->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    _p->futureTimer->stop();

                    const auto& out = _p->future.get();
                    _p->list->setIfChanged(out.first);
                    _p->fileNames->setIfChanged(out.second);
                }
            });

            _p->directoryWatcher->setPath(_p->path->get());
        }

    } // namespace Core
} // namespace djv

