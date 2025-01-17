// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2020 Darby Johnston
// All rights reserved.

#include <djvViewApp/Media.h>

#include <djvViewApp/Annotate.h>
#include <djvViewApp/EditSystem.h>

#include <djvAV/AVSystem.h>
#include <djvAV/IOSystem.h>
#include <djvAV/TimeFunc.h>

#include <djvAudio/AudioSystem.h>
#include <djvAudio/DataFunc.h>

#include <djvSystem/Context.h>
#include <djvSystem/FileInfoFunc.h>
#include <djvSystem/LogSystem.h>
#include <djvSystem/TextSystem.h>
#include <djvSystem/TimerFunc.h>

#include <djvMath/FrameNumberFunc.h>

#include <djvCore/StringFormat.h>
#include <djvCore/StringFunc.h>
#include <djvCore/UndoStack.h>

using namespace djv::Core;

namespace djv
{
    namespace ViewApp
    {
        namespace
        {
            //! \todo Should this be configurable?
            const size_t audioBufferFrameCount = 256;
            const size_t videoQueueSize        = 10;
            const size_t realSpeedFrameCount   = 30;
            
        } // namespace

        struct Media::Private
        {
            std::weak_ptr<System::Context> context;

            bool valid = false;
            System::File::Info fileInfo;
            std::shared_ptr<Observer::ValueSubject<AV::IO::Info> > info;
            Audio::Info audioInfo;
            std::shared_ptr<Observer::ValueSubject<bool> > reload;
            std::shared_ptr<Observer::ValueSubject<std::pair<std::vector<Image::Info>, int> > > layers;
            std::shared_ptr<Observer::ValueSubject<Math::IntRational> > speed;
            std::shared_ptr<Observer::ValueSubject<PlaybackSpeed> > playbackSpeed;
            std::shared_ptr<Observer::ValueSubject<Math::IntRational> > defaultSpeed;
            std::shared_ptr<Observer::ValueSubject<Math::IntRational> > customSpeed;
            float realSpeed = 0.F;
            std::shared_ptr<Observer::ValueSubject<float> > realSpeedSubject;
            std::shared_ptr<Observer::ValueSubject<bool> > playEveryFrame;
            std::shared_ptr<Observer::ValueSubject<Math::Frame::Sequence> > sequence;
            std::shared_ptr<Observer::ValueSubject<Math::Frame::Index> > currentFrame;
            std::shared_ptr<Observer::ValueSubject<std::shared_ptr<Image::Data> > > currentImage;
            std::shared_ptr<Observer::ValueSubject<Playback> > playback;
            std::shared_ptr<Observer::ValueSubject<PlaybackMode> > playbackMode;
            std::shared_ptr<Observer::ValueSubject<AV::IO::InOutPoints> > inOutPoints;
            std::shared_ptr<Observer::ValueSubject<bool> > audioEnabled;
            std::shared_ptr<Observer::ValueSubject<float> > volume;
            std::shared_ptr<Observer::ValueSubject<bool> > mute;
            std::shared_ptr<Observer::ValueSubject<size_t> > threadCount;
            std::shared_ptr<Observer::ValueSubject<Math::Frame::Sequence> > cacheSequence;
            std::shared_ptr<Observer::ValueSubject<Math::Frame::Sequence> > cachedFrames;
            bool cacheEnabled = false;
            size_t cacheMaxByteCount = 0;
            std::shared_ptr<Observer::ListSubject<std::shared_ptr<AnnotatePrimitive> > > annotations;
            std::shared_ptr<Command::UndoStack> undoStack;

            std::shared_ptr<Observer::ValueSubject<size_t> > videoQueueMax;
            std::shared_ptr<Observer::ValueSubject<size_t> > videoQueueCount;
            std::shared_ptr<Observer::ValueSubject<size_t> > audioQueueMax;
            std::shared_ptr<Observer::ValueSubject<size_t> > audioQueueCount;
            std::shared_ptr<AV::IO::IRead> read;

            AV::IO::Direction ioDirection = AV::IO::Direction::Forward;
            std::unique_ptr<RtAudio> rtAudio;
            std::shared_ptr<Audio::Data> audioData;
            size_t audioDataSamplesOffset = 0;
            size_t audioDataSamplesCount = 0;
            Math::Frame::Index frameOffset = 0;
            Time::Duration currentTime = Time::Duration::zero();
            std::chrono::steady_clock::time_point playbackTime;
            std::chrono::steady_clock::time_point realSpeedTime;
            size_t realSpeedFrameCount = 0;
            Time::Duration playEveryFrameTime = Time::Duration::zero();
            std::shared_ptr<System::Timer> playbackTimer;
            std::shared_ptr<System::Timer> queueTimer;
            std::shared_ptr<System::Timer> realSpeedTimer;
            std::shared_ptr<System::Timer> cacheTimer;
            std::shared_ptr<System::Timer> debugTimer;
        };

        void Media::_init(
            const System::File::Info& fileInfo,
            const std::shared_ptr<System::Context>& context)
        {
            DJV_PRIVATE_PTR();
            p.context = context;

            p.fileInfo = fileInfo;
            p.info = Observer::ValueSubject<AV::IO::Info>::create();
            p.reload = Observer::ValueSubject<bool>::create(false);
            p.layers = Observer::ValueSubject<std::pair<std::vector<Image::Info>, int> >::create(
                std::make_pair(std::vector<Image::Info>(), 0));
            p.speed = Observer::ValueSubject<Math::IntRational>::create();
            p.playbackSpeed = Observer::ValueSubject<PlaybackSpeed>::create();
            p.defaultSpeed = Observer::ValueSubject<Math::IntRational>::create();
            p.customSpeed = Observer::ValueSubject<Math::IntRational>::create();
            p.realSpeedSubject = Observer::ValueSubject<float>::create(p.realSpeed);
            p.playEveryFrame = Observer::ValueSubject<bool>::create(false);
            p.sequence = Observer::ValueSubject<Math::Frame::Sequence>::create();
            p.currentFrame = Observer::ValueSubject<Math::Frame::Index>::create(Math::Frame::invalid);
            p.currentImage = Observer::ValueSubject<std::shared_ptr<Image::Data> >::create();
            p.playback = Observer::ValueSubject<Playback>::create(Playback::First);
            p.playbackMode = Observer::ValueSubject<PlaybackMode>::create(PlaybackMode::First);
            p.inOutPoints = Observer::ValueSubject<AV::IO::InOutPoints>::create();
            p.volume = Observer::ValueSubject<float>::create(1.F);
            p.audioEnabled = Observer::ValueSubject<bool>::create(false);
            p.mute = Observer::ValueSubject<bool>::create(false);
            p.threadCount = Observer::ValueSubject<size_t>::create(4);
            p.cacheSequence = Observer::ValueSubject<Math::Frame::Sequence>::create();
            p.cachedFrames = Observer::ValueSubject<Math::Frame::Sequence>::create();
            p.annotations = Observer::ListSubject<std::shared_ptr<AnnotatePrimitive> >::create();
            p.undoStack = Command::UndoStack::create();
            
            p.videoQueueMax = Observer::ValueSubject<size_t>::create();
            p.audioQueueMax = Observer::ValueSubject<size_t>::create();
            p.videoQueueCount = Observer::ValueSubject<size_t>::create();
            p.audioQueueCount = Observer::ValueSubject<size_t>::create();

            p.playbackTimer = System::Timer::create(context);
            p.playbackTimer->setRepeating(true);
            p.queueTimer = System::Timer::create(context);
            p.queueTimer->setRepeating(true);
            p.realSpeedTimer = System::Timer::create(context);
            p.realSpeedTimer->setRepeating(true);
            auto weak = std::weak_ptr<Media>(std::dynamic_pointer_cast<Media>(shared_from_this()));
            p.realSpeedTimer->start(
                System::getTimerDuration(System::TimerValue::Slow),
                [weak](const std::chrono::steady_clock::time_point&, const Time::Duration&)
            {
                if (auto media = weak.lock())
                {
                    media->_p->realSpeedSubject->setIfChanged(media->_p->realSpeed);
                }
            });
            p.cacheTimer = System::Timer::create(context);
            p.cacheTimer->setRepeating(true);
            p.debugTimer = System::Timer::create(context);
            p.debugTimer->setRepeating(true);

            try
            {
                p.rtAudio.reset(new RtAudio);
            }
            catch (const std::exception& e)
            {
                std::vector<std::string> messages;
                auto textSystem = context->getSystemT<System::TextSystem>();
                messages.push_back(String::Format("{0}: {1}").
                    arg(p.fileInfo.getFileName()).
                    arg(textSystem->getText(DJV_TEXT("error_audio_cannot_be_initialized"))));
                messages.push_back(e.what());
                auto logSystem = context->getSystemT<System::LogSystem>();
                logSystem->log("djv::ViewApp::Media", String::join(messages, ' '), System::LogLevel::Error);
            }

            _open();

            p.queueTimer->start(
                System::getTimerDuration(System::TimerValue::VeryFast),
                [weak](const std::chrono::steady_clock::time_point&, const Time::Duration&)
                {
                    if (auto media = weak.lock())
                    {
                        media->_queueUpdate();
                    }
                });
        }

        Media::Media() :
            _p(new Private)
        {}

        Media::~Media()
        {
            DJV_PRIVATE_PTR();
            p.rtAudio.reset();
        }

        std::shared_ptr<Media> Media::create(
            const System::File::Info& fileInfo,
            const std::shared_ptr<System::Context>& context)
        {
            auto out = std::shared_ptr<Media>(new Media);
            out->_init(fileInfo, context);
            return out;
        }

        bool Media::isValid() const
        {
            return _p->valid;
        }

        const System::File::Info& Media::getFileInfo() const
        {
            return _p->fileInfo;
        }

        std::shared_ptr<Observer::IValueSubject<AV::IO::Info> > Media::observeInfo() const
        {
            return _p->info;
        }

        std::shared_ptr<Core::Observer::IValueSubject<bool> > Media::observeReload() const
        {
            return _p->reload;
        }

        void Media::reload()
        {
            _open();
        }

        std::shared_ptr<Observer::IValueSubject<std::pair<std::vector<Image::Info>, int> > > Media::observeLayers() const
        {
            return _p->layers;
        }

        void Media::setLayer(int value)
        {
            DJV_PRIVATE_PTR();
            if (p.layers->setIfChanged(std::make_pair(p.info->get().video, value)))
            {
                _open();
            }
        }

        void Media::nextLayer()
        {
            size_t layer = _p->layers->get().second;
            ++layer;
            if (layer >= _p->info->get().video.size())
            {
                layer = 0;
            }
            setLayer(layer);
        }

        void Media::prevLayer()
        {
            size_t layer = _p->layers->get().second;
            const size_t size = _p->info->get().video.size();
            if (layer > 0)
            {
                --layer;
            }
            else if (size > 0)
            {
                layer = size - 1;
            }
            else
            {
                layer = 0;
            }
            setLayer(layer);
        }

        std::shared_ptr<Observer::IValueSubject<std::shared_ptr<Image::Data> > > Media::observeCurrentImage() const
        {
            return _p->currentImage;
        }

        std::shared_ptr<Observer::IValueSubject<Math::IntRational> > Media::observeSpeed() const
        {
            return _p->speed;
        }

        std::shared_ptr<Observer::IValueSubject<PlaybackSpeed> > Media::observePlaybackSpeed() const
        {
            return _p->playbackSpeed;
        }

        std::shared_ptr<Observer::IValueSubject<Math::IntRational> > Media::observeCustomSpeed() const
        {
            return _p->customSpeed;
        }

        std::shared_ptr<Observer::IValueSubject<Math::IntRational> > Media::observeDefaultSpeed() const
        {
            return _p->defaultSpeed;
        }

        std::shared_ptr<Observer::IValueSubject<float> > Media::observeRealSpeed() const
        {
            return _p->realSpeedSubject;
        }

        std::shared_ptr<Observer::IValueSubject<bool> > Media::observePlayEveryFrame() const
        {
            return _p->playEveryFrame;
        }

        std::shared_ptr<Observer::IValueSubject<Math::Frame::Sequence> > Media::observeSequence() const
        {
            return _p->sequence;
        }

        std::shared_ptr<Observer::IValueSubject<Math::Frame::Index> > Media::observeCurrentFrame() const
        {
            return _p->currentFrame;
        }

        std::shared_ptr<Observer::IValueSubject<Playback> > Media::observePlayback() const
        {
            return _p->playback;
        }

        std::shared_ptr<Observer::IValueSubject<PlaybackMode> > Media::observePlaybackMode() const
        {
            return _p->playbackMode;
        }

        std::shared_ptr<Observer::IValueSubject<AV::IO::InOutPoints> > Media::observeInOutPoints() const
        {
            return _p->inOutPoints;
        }

        void Media::setPlaybackSpeed(PlaybackSpeed value)
        {
            DJV_PRIVATE_PTR();
            if (p.playbackSpeed->setIfChanged(value))
            {
                Math::IntRational speed;
                switch (value)
                {
                case PlaybackSpeed::Default: speed = p.defaultSpeed->get(); break;
                case PlaybackSpeed::Custom:  speed = p.customSpeed->get(); break;
                default: break;
                }
                _setSpeed(speed);
            }
        }

        void Media::setCustomSpeed(const Math::IntRational& value)
        {
            DJV_PRIVATE_PTR();
            if (p.customSpeed->setIfChanged(value))
            {
                if (PlaybackSpeed::Custom == p.playbackSpeed->get())
                {
                    _setSpeed(value);
                }
            }
        }

        void Media::setPlayEveryFrame(bool value)
        {
            DJV_PRIVATE_PTR();
            if (_p->playEveryFrame->setIfChanged(value))
            {
                _seek(p.currentFrame->get());
                p.audioEnabled->setIfChanged(_isAudioEnabled());
                if (_hasAudioSyncPlayback())
                {
                    _startAudioStream();
                }
            }
        }

        void Media::setCurrentFrame(Math::Frame::Index value, bool inOutPoints)
        {
            DJV_PRIVATE_PTR();
            Math::Range<Math::Frame::Index> range;
            if (inOutPoints)
            {
                range = p.inOutPoints->get().getRange(p.sequence->get().getFrameCount());
            }
            else
            {
                range = Math::Frame::Range(0, p.sequence->get().getLastIndex());
            }
            Math::Frame::Index tmp = value;
            const size_t size = range.getMax() - range.getMin() + 1;
            if (size > 1)
            {
                while (tmp > range.getMax())
                {
                    tmp -= size;
                }
                while (tmp < range.getMin())
                {
                    tmp += size;
                }
            }
            if (p.currentFrame->setIfChanged(tmp))
            {
                setPlayback(Playback::Stop);
                _seek(p.currentFrame->get());
            }
        }

        void Media::inPoint()
        {
            DJV_PRIVATE_PTR();
            const auto& inOutPoints = p.inOutPoints->get();
            setCurrentFrame(inOutPoints.isEnabled() ? inOutPoints.getIn() : 0);
        }

        void Media::outPoint()
        {
            DJV_PRIVATE_PTR();
            const auto& inOutPoints = p.inOutPoints->get();
            const size_t size = p.sequence->get().getFrameCount();
            const Math::Frame::Index end = size > 0 ? (size - 1) : 0;
            setCurrentFrame(inOutPoints.isEnabled() ? inOutPoints.getOut() : end, false);
        }

        void Media::start()
        {
            setCurrentFrame(0, false);
        }

        void Media::end()
        {
            DJV_PRIVATE_PTR();
            const size_t size = p.sequence->get().getFrameCount();
            const Math::Frame::Index frame = size > 0 ? (size - 1) : 0;
            setCurrentFrame(frame, false);
        }

        void Media::nextFrame(size_t value)
        {
            DJV_PRIVATE_PTR();
            const Math::Frame::Index frame = p.currentFrame->get();
            setCurrentFrame(frame + static_cast<Math::Frame::Index>(value));
        }

        void Media::prevFrame(size_t value)
        {
            DJV_PRIVATE_PTR();
            const Math::Frame::Index frame = p.currentFrame->get();
            setCurrentFrame(frame - static_cast<Math::Frame::Index>(value));
        }

        void Media::setPlayback(Playback value)
        {
            DJV_PRIVATE_PTR();
            const auto& sequence = p.sequence->get();
            const size_t sequenceSize = sequence.getFrameCount();
            const auto& range = p.inOutPoints->get().getRange(sequenceSize);
            const auto currentFrame = p.currentFrame->get();
            switch (value)
            {
            case Playback::Forward:
                switch (p.playbackMode->get())
                {
                case PlaybackMode::Once:
                    if (currentFrame >= range.getMax() ||
                        currentFrame < range.getMin())
                    {
                        setCurrentFrame(range.getMin());
                    }
                    break;
                case PlaybackMode::Loop:
                    if (currentFrame > range.getMax() ||
                        currentFrame < range.getMin())
                    {
                        setCurrentFrame(range.getMin());
                    }
                    break;
                case PlaybackMode::PingPong:
                    if (currentFrame > range.getMax())
                    {
                        setCurrentFrame(range.getMax());
                    }
                    else if (currentFrame < range.getMin())
                    {
                        setCurrentFrame(range.getMin());
                    }
                    break;
                default: break;
                }
                break;
            case Playback::Reverse:
                switch (p.playbackMode->get())
                {
                case PlaybackMode::Once:
                    if (currentFrame <= range.getMin() ||
                        currentFrame > range.getMax())
                    {
                        setCurrentFrame(range.getMax());
                    }
                    break;
                case PlaybackMode::Loop:
                    if (currentFrame < range.getMin() ||
                        currentFrame > range.getMax())
                    {
                        setCurrentFrame(range.getMax());
                    }
                    break;
                case PlaybackMode::PingPong:
                    if (currentFrame > range.getMax())
                    {
                        setCurrentFrame(range.getMax());
                    }
                    else if (currentFrame < range.getMin())
                    {
                        setCurrentFrame(range.getMin());
                    }
                    break;
                default: break;
                }
                break;
            default: break;
            }
            if (p.playback->setIfChanged(value))
            {
                _playbackUpdate();
            }
        }

        void Media::setPlaybackMode(PlaybackMode value)
        {
            _p->playbackMode->setIfChanged(value);
        }

        void Media::setInOutPoints(const AV::IO::InOutPoints& value)
        {
            DJV_PRIVATE_PTR();
            if (p.inOutPoints->setIfChanged(value))
            {
                if (p.read)
                {
                    p.read->setInOutPoints(value);
                }
                _seek(p.currentFrame->get());
                if (_hasAudioSyncPlayback())
                {
                    _startAudioStream();
                }
            }
        }

        void Media::setInPoint()
        {
            DJV_PRIVATE_PTR();
            setInOutPoints(AV::IO::InOutPoints(true, p.currentFrame->get(), p.inOutPoints->get().getOut()));
        }

        void Media::setOutPoint()
        {
            DJV_PRIVATE_PTR();
            setInOutPoints(AV::IO::InOutPoints(true, p.inOutPoints->get().getIn(), p.currentFrame->get()));
        }

        void Media::resetInPoint()
        {
            DJV_PRIVATE_PTR();
            const AV::IO::InOutPoints& value = p.inOutPoints->get();
            bool enabled = true;
            const size_t size = p.sequence->get().getFrameCount();
            if ((static_cast<Math::Frame::Index>(size) - 1) == value.getOut())
            {
                enabled = false;
            }
            setInOutPoints(AV::IO::InOutPoints(enabled, 0, value.getOut()));
        }

        void Media::resetOutPoint()
        {
            DJV_PRIVATE_PTR();
            const AV::IO::InOutPoints& value = p.inOutPoints->get();
            bool enabled = true;
            if (0 == value.getIn())
            {
                enabled = false;
            }
            const size_t size = p.sequence->get().getFrameCount();
            setInOutPoints(AV::IO::InOutPoints(enabled, value.getIn(), size > 0 ? (static_cast<Math::Frame::Index>(size) - 1) : 0));
        }

        std::shared_ptr<Observer::IValueSubject<bool> > Media::observeAudioEnabled() const
        {
            return _p->audioEnabled;
        }

        std::shared_ptr<Observer::IValueSubject<float> > Media::observeVolume() const
        {
            return _p->volume;
        }

        std::shared_ptr<Observer::IValueSubject<bool> > Media::observeMute() const
        {
            return _p->mute;
        }

        void Media::setVolume(float value)
        {
            _p->volume->setIfChanged(Math::clamp(value, 0.F, 1.F));
        }

        void Media::setMute(bool value)
        {
            _p->mute->setIfChanged(value);
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > Media::observeThreadCount() const
        {
            return _p->threadCount;
        }

        void Media::setThreadCount(size_t value)
        {
            DJV_PRIVATE_PTR();
            if (p.threadCount->setIfChanged(value))
            {
                if (p.read)
                {
                    p.read->setThreadCount(value);
                }
            }
        }

        bool Media::hasCache() const
        {
            DJV_PRIVATE_PTR();
            return p.read ? p.read->hasCache() : false;
        }

        size_t Media::getCacheMaxByteCount() const
        {
            return _p->cacheMaxByteCount;
        }

        size_t Media::getCacheByteCount() const
        {
            DJV_PRIVATE_PTR();
            return p.read ? p.read->getCacheByteCount() : 0;
        }

        std::shared_ptr<Core::Observer::IValueSubject<Math::Frame::Sequence> > Media::observeCacheSequence() const
        {
            return _p->cacheSequence;
        }

        std::shared_ptr<Core::Observer::IValueSubject<Math::Frame::Sequence> > Media::observeCachedFrames() const
        {
            return _p->cachedFrames;
        }

        void Media::setCacheEnabled(bool value)
        {
            DJV_PRIVATE_PTR();
            p.cacheEnabled = value;
            if (p.read)
            {
                p.read->setCacheEnabled(p.cacheEnabled);
            }
        }

        void Media::setCacheMaxByteCount(size_t value)
        {
            DJV_PRIVATE_PTR();
            p.cacheMaxByteCount = value;
            if (p.read)
            {
                p.read->setCacheMaxByteCount(p.cacheMaxByteCount);
            }
        }
            
        std::shared_ptr<Core::Observer::IListSubject<std::shared_ptr<AnnotatePrimitive> > > Media::observeAnnotations() const
        {
            return _p->annotations;
        }

        void Media::addAnnotation(const std::shared_ptr<AnnotatePrimitive>& value)
        {
            _p->annotations->pushBack(value);
        }

        void Media::removeAnnotation(const std::shared_ptr<AnnotatePrimitive>& value)
        {
            const size_t i = _p->annotations->indexOf(value);
            if (i != Observer::invalidListIndex)
            {
                _p->annotations->removeItem(i);
            }
        }
        
        void Media::clearAnnotations()
        {
            _p->annotations->clear();
        }

        void Media::pushCommand(const std::shared_ptr<Core::Command::ICommand>& value)
        {
            _p->undoStack->push(value);
        }

        std::shared_ptr<Core::Observer::IValueSubject<bool> > Media::observeHasUndo() const
        {
            return _p->undoStack->observeHasUndo();
        }

        std::shared_ptr<Core::Observer::IValueSubject<bool> > Media::observeHasRedo() const
        {
            return _p->undoStack->observeHasRedo();
        }

        void Media::undo()
        {
            _p->undoStack->undo();
        }

        void Media::redo()
        {
            _p->undoStack->redo();
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > Media::observeVideoQueueMax() const
        {
            return _p->videoQueueMax;
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > Media::observeAudioQueueMax() const
        {
            return _p->audioQueueMax;
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > Media::observeVideoQueueCount() const
        {
            return _p->videoQueueCount;
        }

        std::shared_ptr<Observer::IValueSubject<size_t> > Media::observeAudioQueueCount() const
        {
            return _p->audioQueueCount;
        }

        bool Media::_hasAudio() const
        {
            DJV_PRIVATE_PTR();
            return p.audioInfo.isValid() && p.rtAudio;
        }

        bool Media::_isAudioEnabled() const
        {
            DJV_PRIVATE_PTR();
            return _hasAudio() &&
                p.speed->get() == p.defaultSpeed->get() &&
                !p.playEveryFrame->get();
        }

        bool Media::_hasAudioSyncPlayback() const
        {
            DJV_PRIVATE_PTR();
            return _isAudioEnabled() &&
                Playback::Forward == p.playback->get();
        }

        void Media::_open()
        {
            DJV_PRIVATE_PTR();
            if (auto context = p.context.lock())
            {
                try
                {
                    p.valid = false;

                    AV::IO::ReadOptions options;
                    options.layer = p.layers->get().second;
                    options.videoQueueSize = videoQueueSize;
                    auto io = context->getSystemT<AV::IO::IOSystem>();
                    p.read = io->read(p.fileInfo, options);
                    p.read->setThreadCount(p.threadCount->get());
                    p.read->setLoop(true);
                    p.read->setCacheEnabled(p.cacheEnabled);
                    p.read->setCacheMaxByteCount(p.cacheMaxByteCount);

                    const auto info = p.read->getInfo().get();
                    p.info->setIfChanged(info);
                    const int currentLayer = Math::clamp(p.layers->get().second, 0, static_cast<int>(info.video.size()) - 1);
                    p.layers->setIfChanged(std::make_pair(info.video, currentLayer));
                    Math::IntRational speed = info.videoSpeed;
                    Math::Frame::Sequence sequence = info.videoSequence;
                    p.audioInfo = info.audio;
                    {
                        std::stringstream ss;
                        ss << "Open: " << p.fileInfo << ", sequence: " << sequence;
                        auto logSystem = context->getSystemT<System::LogSystem>();
                        logSystem->log("djv::ViewApp::Media", ss.str());
                    }
                    p.speed->setIfChanged(speed);
                    p.defaultSpeed->setIfChanged(speed);
                    p.sequence->setIfChanged(sequence);
                    const Math::Frame::Index end = sequence.getLastIndex();
                    p.inOutPoints->setIfChanged(AV::IO::InOutPoints(false, 0, end));
                    const Math::Frame::Index currentFrame = p.currentFrame->get();
                    Math::Frame::Index frame = Math::Frame::invalid;
                    if (Math::Frame::invalid == currentFrame)
                    {
                        frame = p.sequence->get().getFrameCount() > 1 ? 0 : Math::Frame::invalid;
                    }
                    else
                    {
                        frame = Math::clamp(currentFrame, static_cast<Math::Frame::Index>(0), end);
                    }
                    p.currentFrame->setIfChanged(frame);
                    if (_hasAudio())
                    {
                        if (p.rtAudio->isStreamOpen())
                        {
                            p.rtAudio->closeStream();
                        }
                        RtAudio::StreamParameters rtParameters;
                        auto audioSystem = context->getSystemT<Audio::AudioSystem>();
                        rtParameters.deviceId = audioSystem->getDefaultOutputDevice();
                        rtParameters.nChannels = p.audioInfo.channelCount;
                        unsigned int rtBufferFrames = audioBufferFrameCount;
                        try
                        {
                            p.rtAudio->openStream(
                                &rtParameters,
                                nullptr,
                                Audio::toRtAudio(p.audioInfo.type),
                                p.audioInfo.sampleRate,
                                &rtBufferFrames,
                                _rtAudioCallback,
                                this,
                                nullptr,
                                _rtAudioErrorCallback);
                        }
                        catch (const std::exception& e)
                        {
                            std::vector<std::string> messages;
                            auto textSystem = context->getSystemT<System::TextSystem>();
                            messages.push_back(String::Format("{0}: {1}").
                                arg(p.fileInfo.getFileName()).
                                arg(textSystem->getText(DJV_TEXT("error_the_audio_stream_cannot_be_opened"))));
                            messages.push_back(e.what());
                            auto logSystem = context->getSystemT<System::LogSystem>();
                            logSystem->log("djv::ViewApp::Media", String::join(messages, ' '), System::LogLevel::Error);
                        }
                    }
                    p.audioEnabled->setIfChanged(_isAudioEnabled());

                    auto weak = std::weak_ptr<Media>(std::dynamic_pointer_cast<Media>(shared_from_this()));
                    p.cacheTimer->start(
                        System::getTimerDuration(System::TimerValue::Fast),
                        [weak](const std::chrono::steady_clock::time_point&, const Time::Duration&)
                        {
                            if (auto media = weak.lock())
                            {
                                if (media->_p->read)
                                {
                                    const auto& sequence = media->_p->read->getCacheSequence();
                                    const auto& frames = media->_p->read->getCachedFrames();
                                    media->_p->cacheSequence->setIfChanged(sequence);
                                    media->_p->cachedFrames->setIfChanged(frames);
                                }
                            }
                        });

                    p.debugTimer->start(
                        System::getTimerDuration(System::TimerValue::Medium),
                        [weak](const std::chrono::steady_clock::time_point&, const Time::Duration&)
                        {
                            if (auto media = weak.lock())
                            {
                                if (media->_p->read)
                                {
                                    bool valid = false;
                                    size_t videoQueueMax   = 0;
                                    size_t videoQueueCount = 0;
                                    size_t audioQueueMax   = 0;
                                    size_t audioQueueCount = 0;
                                    {
                                        std::unique_lock<std::mutex> lock(media->_p->read->getMutex());
                                        if (lock.owns_lock())
                                        {
                                            valid = true;
                                            const auto& videoQueue = media->_p->read->getVideoQueue();
                                            const auto& audioQueue = media->_p->read->getAudioQueue();
                                            videoQueueMax   = videoQueue.getMax();
                                            videoQueueCount = videoQueue.getCount();
                                            audioQueueMax   = audioQueue.getMax();
                                            audioQueueCount = audioQueue.getCount();
                                        }
                                    }
                                    if (valid)
                                    {
                                        media->_p->videoQueueMax->setAlways(videoQueueMax);
                                        media->_p->videoQueueCount->setAlways(videoQueueCount);
                                        media->_p->audioQueueMax->setAlways(audioQueueMax);
                                        media->_p->audioQueueCount->setAlways(audioQueueCount);
                                    }
                                }
                            }
                        });

                    p.valid = true;
                }
                catch (const std::exception& e)
                {
                    auto logSystem = context->getSystemT<System::LogSystem>();
                    logSystem->log(
                        "djv::ViewApp::Media",
                        e.what(),
                        System::LogLevel::Error);
                }
                
                _seek(p.currentFrame->get());

                p.reload->setAlways(true);
            }
        }

        void Media::_setSpeed(const Math::IntRational& value)
        {
            DJV_PRIVATE_PTR();
            if (p.speed->setIfChanged(value))
            {
                _seek(p.currentFrame->get());
                p.audioEnabled->setIfChanged(_isAudioEnabled());
                if (_hasAudioSyncPlayback())
                {
                    _startAudioStream();
                }
            }
        }

        void Media::_setCurrentFrame(Math::Frame::Index value)
        {
            DJV_PRIVATE_PTR();
            const Math::Frame::Sequence& sequence = p.sequence->get();
            const size_t sequenceSize = sequence.getFrameCount();
            const auto& range = p.inOutPoints->get().getRange(sequenceSize);
            if (p.currentFrame->setIfChanged(value))
            {
                switch (p.playback->get())
                {
                case Playback::Forward:
                    if (value >= range.getMax())
                    {
                        switch (p.playbackMode->get())
                        {
                        case PlaybackMode::Once:
                            setPlayback(Playback::Stop);
                            break;
                        case PlaybackMode::Loop:
                        {
                            setPlayback(Playback::Stop);
                            setPlayback(Playback::Forward);
                            break;
                        }
                        case PlaybackMode::PingPong:
                        {
                            setPlayback(Playback::Stop);
                            setPlayback(Playback::Reverse);
                            break;
                        }
                        default: break;
                        }
                    }
                    break;
                case Playback::Reverse:
                    if (value <= range.getMin())
                    {
                        switch (p.playbackMode->get())
                        {
                        case PlaybackMode::Once:
                            setPlayback(Playback::Stop);
                            break;
                        case PlaybackMode::Loop:
                        {
                            setPlayback(Playback::Stop);
                            setPlayback(Playback::Reverse);
                            break;
                        }
                        case PlaybackMode::PingPong:
                        {
                            setPlayback(Playback::Stop);
                            setPlayback(Playback::Forward);
                            break;
                        }
                        default: break;
                        }
                    }
                    break;
                default: break;
                }
            }
        }

        void Media::_seek(Math::Frame::Index value)
        {
            DJV_PRIVATE_PTR();
            if (auto context = p.context.lock())
            {
                if (p.read)
                {
                    p.read->seek(value, p.ioDirection);
                }
                p.audioData.reset();
                p.audioDataSamplesOffset = 0;
                p.audioDataSamplesCount = 0;
                p.frameOffset = p.currentFrame->get();
                p.currentTime = Time::Duration::zero();
                p.realSpeedTime = std::chrono::steady_clock::now();
                p.realSpeedFrameCount = 0;
                p.playEveryFrameTime = Time::Duration::zero();
                _stopAudioStream();
            }
        }

        void Media::_playbackUpdate()
        {
            DJV_PRIVATE_PTR();
            if (auto context = p.context.lock())
            {
                bool forward = false;
                switch (p.playback->get())
                {
                case Playback::Stop:
                    if (p.read)
                    {
                        p.read->setPlayback(false);
                    }
                    _stopAudioStream();
                    p.playbackTimer->stop();
                    _seek(p.currentFrame->get());
                    break;
                case Playback::Forward:
                    forward = true;
                case Playback::Reverse: // Forward or reverse.
                {
                    if (p.read)
                    {
                        p.read->setPlayback(true);
                    }
                    p.ioDirection = forward ? AV::IO::Direction::Forward : AV::IO::Direction::Reverse;
                    _seek(p.currentFrame->get());
                    p.audioData.reset();
                    p.audioDataSamplesOffset = 0;
                    p.audioDataSamplesCount = 0;
                    p.frameOffset = p.currentFrame->get();
                    p.currentTime = Time::Duration::zero();
                    p.playbackTime = std::chrono::steady_clock::now();
                    p.realSpeedTime = p.playbackTime;
                    p.realSpeedFrameCount = 0;
                    p.playEveryFrameTime = Time::Duration::zero();
                    if (_hasAudioSyncPlayback())
                    {
                        _startAudioStream();
                    }
                    auto weak = std::weak_ptr<Media>(std::dynamic_pointer_cast<Media>(shared_from_this()));
                    p.playbackTimer->start(
                        System::getTimerDuration(System::TimerValue::VeryFast),
                        [weak](const std::chrono::steady_clock::time_point&, const Time::Duration&)
                    {
                        if (auto media = weak.lock())
                        {
                            auto now = std::chrono::steady_clock::now();
                            auto delta = std::chrono::duration_cast<Time::Duration>(now - media->_p->playbackTime);
                            media->_p->playbackTime = now;
                            media->_p->currentTime += delta;
                            media->_p->playEveryFrameTime += delta;
                            media->_playbackTick();
                        }
                    });
                    break;
                }
                default: break;
                }
            }
        }

        void Media::_playbackTick()
        {
            DJV_PRIVATE_PTR();
            const Playback playback = p.playback->get();
            switch (playback)
            {
            case Playback::Forward:
            case Playback::Reverse:
            {
                const auto& speed = p.speed->get();
                if (_hasAudioSyncPlayback())
                {
                    if (p.audioDataSamplesCount)
                    {
                        Math::Frame::Index frame = p.frameOffset +
                            AV::Time::scale(
                                p.audioDataSamplesCount,
                                Math::IntRational(1, static_cast<int>(p.audioInfo.sampleRate)),
                                speed.swap());
                        _setCurrentFrame(frame);
                    }
                }
                else if (p.playEveryFrame->get())
                {
                }
                else
                {
                    Math::Frame::Index elapsed = std::chrono::duration<float>(p.currentTime).count() * speed.toFloat();
                    Math::Frame::Index frame = Math::Frame::invalid;
                    switch (playback)
                    {
                    case Playback::Forward: frame = p.frameOffset + elapsed; break;
                    case Playback::Reverse: frame = p.frameOffset - elapsed; break;
                    default: break;
                    }
                    _setCurrentFrame(frame);
                }
                break;
            }
            default: break;
            }
        }

        void Media::_startAudioStream()
        {
            DJV_PRIVATE_PTR();
            if (auto context = p.context.lock())
            {
                try
                {
                    p.rtAudio->startStream();
                }
                catch (const std::exception& e)
                {
                    std::vector<std::string> messages;
                    auto textSystem = context->getSystemT<System::TextSystem>();
                    messages.push_back(String::Format("{0}: {1}").
                        arg(p.fileInfo.getFileName()).
                        arg(textSystem->getText(DJV_TEXT("error_cannot_start_audio_stream"))));
                    messages.push_back(e.what());
                    auto logSystem = context->getSystemT<System::LogSystem>();
                    logSystem->log("djv::ViewApp::Media", String::join(messages, ' '), System::LogLevel::Error);
                }
            }
        }

        void Media::_stopAudioStream()
        {
            DJV_PRIVATE_PTR();
            if (auto context = p.context.lock())
            {
                if (_hasAudio() && p.rtAudio->isStreamRunning())
                {
                    try
                    {
                        p.rtAudio->abortStream();
                        p.rtAudio->setStreamTime(0.0);
                    }
                    catch (const std::exception& e)
                    {
                        std::vector<std::string> messages;
                        auto textSystem = context->getSystemT<System::TextSystem>();
                        messages.push_back(String::Format("{0}: {1}").
                            arg(p.fileInfo.getFileName()).
                            arg(textSystem->getText(DJV_TEXT("error_cannot_stop_audio_stream"))));
                        messages.push_back(e.what());
                        auto logSystem = context->getSystemT<System::LogSystem>();
                        logSystem->log("djv::ViewApp::Media", String::join(messages, ' '), System::LogLevel::Error);
                    }
                }
            }
        }

        void Media::_queueUpdate()
        {
            DJV_PRIVATE_PTR();
            if (p.read)
            {
                // Update the video queue.
                const Playback playback = p.playback->get();
                const auto frameTime = std::chrono::duration<float>(1.F / p.speed->get().toFloat());
                const bool playEveryFrameAdvance = p.playEveryFrameTime >= frameTime;
                //std::cout << "every frame: " << playEveryFrameAdvance << ", " <<
                //    p.playEveryFrameTime.count() << "/" << frameTime.count() << std::endl;
                const Math::Frame::Index currentFrame = p.currentFrame->get();
                AV::IO::VideoFrame frame;
                bool gotFrame = false;
                {
                    std::lock_guard<std::mutex> lock(p.read->getMutex());
                    auto& queue = p.read->getVideoQueue();
                    if (p.playEveryFrame->get())
                    {
                        if (playback != Playback::Stop && !queue.isEmpty() && playEveryFrameAdvance)
                        {
                            frame = queue.popFrame();
                            gotFrame = true;
                            p.realSpeedFrameCount = p.realSpeedFrameCount + 1;
                            p.playEveryFrameTime = p.playEveryFrameTime - std::chrono::duration_cast<Time::Duration>(frameTime);
                        }
                    }
                    else
                    {
                        while (!queue.isEmpty() &&
                            (AV::IO::Direction::Forward == p.ioDirection ?
                                (queue.getFrame().frame < currentFrame) :
                                (queue.getFrame().frame > currentFrame)))
                        {
                            frame = queue.popFrame();
                            gotFrame = true;
                            p.realSpeedFrameCount = p.realSpeedFrameCount + 1;
                        }
                    }
                    if (!gotFrame && !queue.isEmpty())
                    {
                        frame = queue.getFrame();
                        gotFrame = true;
                    }
                }
                if (gotFrame)
                {
                    if (p.realSpeedFrameCount >= realSpeedFrameCount)
                    {
                        auto now = std::chrono::steady_clock::now();
                        auto delta = std::chrono::duration<float>(now - p.realSpeedTime);
                        p.realSpeed = p.realSpeedFrameCount / delta.count();
                        p.realSpeedTime = now;
                        p.realSpeedFrameCount = 0;
                    }
                    p.currentImage->setIfChanged(frame.data);
                    if (p.playEveryFrame->get())
                    {
                        _setCurrentFrame(frame.frame);
                    }
                }

                // Update the audio queue.
                if (_hasAudio() && !_hasAudioSyncPlayback())
                {
                    std::lock_guard<std::mutex> lock(p.read->getMutex());
                    auto& queue = p.read->getAudioQueue();
                    while (queue.getCount() > queue.getMax())
                    {
                        queue.popFrame();
                    }
                }
            }
        }
        
        int Media::_rtAudioCallback(
            void* outputBuffer,
            void* inputBuffer,
            unsigned int nFrames,
            double streamTime,
            RtAudioStreamStatus status,
            void* userData)
        {
            Media* media = reinterpret_cast<Media*>(userData);
            const auto& info = media->_p->audioInfo;

            size_t outputSampleCount = static_cast<size_t>(nFrames);
            size_t sampleCount = 0;
            const size_t sampleByteCount = info.channelCount * Audio::getByteCount(info.type);
            const float volume = !media->_p->mute->get() ? media->_p->volume->get() : 0.F;

            if (media->_p->audioData)
            {
                sampleCount += media->_p->audioData->getSampleCount() - media->_p->audioDataSamplesOffset;
            }

            // Get audio frames from the read queue.
            std::vector<AV::IO::AudioFrame> frames;
            {
                std::lock_guard<std::mutex> lock(media->_p->read->getMutex());
                auto& queue = media->_p->read->getAudioQueue();
                while (!queue.isEmpty() && sampleCount < outputSampleCount)
                {
                    auto frame = queue.getFrame();
                    frames.push_back(frame);
                    queue.popFrame();
                    sampleCount += frame.data->getSampleCount();
                }
            }

            // Use the remaining data from the frame.
            uint8_t* p = reinterpret_cast<uint8_t*>(outputBuffer);
            if (media->_p->audioData)
            {
                const size_t size = std::min(
                    media->_p->audioData->getSampleCount() - media->_p->audioDataSamplesOffset,
                    outputSampleCount);
                //memcpy(
                //    p,
                //    media->_p->audioData->getData() + media->_p->audioDataSamplesOffset * sampleByteCount,
                //    size * sampleByteCount);
                Audio::volume(
                    media->_p->audioData->getData() + media->_p->audioDataSamplesOffset * sampleByteCount,
                    p,
                    volume,
                    size,
                    info.channelCount,
                    info.type);
                p += size * sampleByteCount;
                media->_p->audioDataSamplesOffset += size;
                media->_p->audioDataSamplesCount += size;
                outputSampleCount -= size;
                if (media->_p->audioDataSamplesOffset >= media->_p->audioData->getSampleCount())
                {
                    media->_p->audioData.reset();
                    media->_p->audioDataSamplesOffset = 0;
                }
            }

            // Process the frames.
            for (const auto& i : frames)
            {
                media->_p->audioData = i.data;
                size_t size = std::min(i.data->getSampleCount(), outputSampleCount);
                //memcpy(
                //    p,
                //    i.audio->getData(),
                //    size * sampleByteCount);
                Audio::volume(
                    i.data->getData(),
                    p,
                    volume,
                    size,
                    info.channelCount,
                    info.type);
                p += size * sampleByteCount;
                media->_p->audioDataSamplesOffset = size;
                media->_p->audioDataSamplesCount += size;
                outputSampleCount -= size;
            }

            const size_t zero = (nFrames * sampleByteCount) - (p - reinterpret_cast<uint8_t*>(outputBuffer));
            if (zero)
            {
                //! \todo Is this the correct way to clear the audio data?
                memset(p, 0, zero);
            }

            return 0;
        }

        void Media::_rtAudioErrorCallback(
            RtAudioError::Type type,
            const std::string& errorText)
        {}

    } // namespace ViewApp
} // namespace djv

