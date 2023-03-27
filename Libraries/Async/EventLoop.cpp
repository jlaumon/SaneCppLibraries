// Copyright (c) 2022-2023, Stefano Cristiano
//
// All Rights Reserved. Reproduction is not allowed.
#include "EventLoop.h"
#include "../Foundation/Optional.h"

#if SC_PLATFORM_WINDOWS
#include "EventLoopInternalWindows.inl"
#elif SC_PLATFORM_EMSCRIPTEN
#include "EventLoopInternalEmscripten.inl"
#elif SC_PLATFORM_APPLE
#include "EventLoopInternalApple.inl"
#endif

template <>
void SC::OpaqueFunctions<SC::EventLoop::Internal, SC::EventLoop::InternalSize,
                         SC::EventLoop::InternalAlignment>::construct(Handle& buffer)
{
    new (&buffer.reinterpret_as<Object>(), PlacementNew()) Object();
}

template <>
void SC::OpaqueFunctions<SC::EventLoop::Internal, SC::EventLoop::InternalSize,
                         SC::EventLoop::InternalAlignment>::destruct(Object& obj)
{
    obj.~Internal();
}

void SC::EventLoop::addTimeout(IntegerMilliseconds expiration, Async& async, Function<void(AsyncResult&)>&& callback)
{
    Async::Timeout timeout;
    updateTime();
    timeout.expirationTime = loopTime.offsetBy(expiration);
    timeout.timeout        = expiration;
    async.operation.assignValue(move(timeout));
    async.callback = move(callback);
    submitAsync(async);
}

void SC::EventLoop::addRead(FileDescriptorNative fd, Async& async)
{
    Async::Read read;
    read.fileDescriptor = fd;
    async.operation.assignValue(move(read));
    submitAsync(async);
}

void SC::EventLoop::submitAsync(Async& async)
{
    SC_RELEASE_ASSERT(async.state == Async::State::Free);
    async.state = Async::State::Submitting;
    submission.queueBack(async);
}

bool SC::EventLoop::shouldQuit() { return submission.isEmpty(); }

SC::ReturnCode SC::EventLoop::run()
{
    while (!shouldQuit())
    {
        SC_TRY_IF(runOnce());
    }
    return true;
}

const SC::TimeCounter* SC::EventLoop::findEarliestTimer() const
{
    const TimeCounter* earliestTime = nullptr;
    for (Async* async = activeTimers.front; async != nullptr; async = async->next)
    {
        SC_RELEASE_ASSERT(async->operation.type == Async::Operation::Type::Timeout);
        const auto& expirationTime = async->operation.fields.timeout.expirationTime;
        if (earliestTime == nullptr or earliestTime->isLaterThanOrEqualTo(expirationTime))
        {
            earliestTime = &expirationTime;
        }
    };
    return earliestTime;
}

void SC::EventLoop::invokeExpiredTimers()
{
    for (Async* async = activeTimers.front; async != nullptr;)
    {
        SC_RELEASE_ASSERT(async->operation.type == Async::Operation::Type::Timeout);
        const auto& expirationTime = async->operation.fields.timeout.expirationTime;
        if (loopTime.isLaterThanOrEqualTo(expirationTime))
        {
            Async* currentAsync = async;
            async               = async->next;
            activeTimers.remove(*currentAsync);
            currentAsync->state = Async::State::Free;
            AsyncResult res{*this, *currentAsync};
            currentAsync->callback(res);
        }
        else
        {
            async = async->next;
        }
    }
}

[[nodiscard]] SC::ReturnCode SC::EventLoop::create()
{
    Internal& self = internal.get();
    SC_TRY_IF(self.createEventLoop());
    SC_TRY_IF(self.createWakeup(*this));
    return true;
}

SC::ReturnCode SC::EventLoop::stageSubmissions(SC::EventLoop::KernelQueue& queue)
{
    while (Async* async = submission.dequeueFront())
    {
        switch (async->state)
        {
        case Async::State::Submitting: {
            SC_TRY_IF(queue.pushAsync(*this, async));
            async->state = Async::State::Active;
            if (queue.isFull())
            {
                SC_TRY_IF(queue.commitQueue(*this));
                stagedHandles.clear();
            }
        }
        break;
        case Async::State::Free: {
            // TODO: Stop the completion, it has been cancelled before being submitted
            SC_RELEASE_ASSERT(false);
        }
        break;
        case Async::State::Cancelling: {
            // TODO: issue an actual kevent for removal
            SC_RELEASE_ASSERT(false);
        }
        break;
        case Async::State::Active: {
            SC_DEBUG_ASSERT(false);
            return "EventLoop::processSubmissions() got Active handle"_a8;
        }
        break;
        }
    }
    return true;
}

SC::ReturnCode SC::EventLoop::runOnce()
{
    KernelQueue queue;

    auto defer = MakeDeferred(
        [this]
        {
            if (not stagedHandles.isEmpty())
            {
                // TODO: we should re-append stagedHandles to submissions and transition them to Submitting state
                SC_RELEASE_ASSERT(false);
            }
        });
    SC_TRY_IF(stageSubmissions(queue));

    SC_RELEASE_ASSERT(submission.isEmpty());

    const TimeCounter* nextTimer = findEarliestTimer();
    SC_TRY_IF(queue.poll(*this, nextTimer));
    stagedHandles.clear();

    if (nextTimer)
    {
        const bool timeoutOccurredWithoutIO = queue.newEvents == 0;
        const bool timeoutWasAlreadyExpired = loopTime.isLaterThanOrEqualTo(*nextTimer);
        if (timeoutOccurredWithoutIO or timeoutWasAlreadyExpired)
        {
            if (timeoutWasAlreadyExpired)
            {
                // Not sure if this is really possible.
                updateTime();
            }
            else
            {
                loopTime = *nextTimer;
            }
            invokeExpiredTimers();
        }
    }

    for (decltype(KernelQueue::newEvents) idx = 0; idx < queue.newEvents; ++idx)
    {
        if (internal.get().isWakeUp(queue.events[idx]))
        {
            runCompletionForNotifiers();
        }
    }

    return true;
}

SC::ReturnCode SC::EventLoop::initNotifier(ExternalThreadNotifier& notifier, Function<void(EventLoop&)>&& callback)
{
    SC_TRY_MSG(notifier.loop == nullptr and notifier.pending.load() == false or notifier.next != nullptr or
                   notifier.prev != nullptr,
               "EventLoop::registerNotifier re-using already in use notifier"_a8);
    notifier.callback = forward<Function<void(EventLoop&)>>(callback);
    notifier.loop     = this;
    notifiers.queueBack(notifier);
    return true;
}

void SC::EventLoop::removeNotifier(ExternalThreadNotifier& notifier)
{
    notifiers.remove(notifier);
    notifier.loop     = nullptr;
    notifier.callback = Function<void(EventLoop&)>();
}

SC::ReturnCode SC::EventLoop::notifyFromExternalThread(ExternalThreadNotifier& notifier)
{
    if (not notifier.pending.exchange(true))
    {
        // This executes if current thread is lucky enough to atomically exchange pending from false to true.
        // This effectively allows coalescing calls from different threads into a single notification.
        SC_TRY_IF(notifier.loop->wakeUpFromExternalThread());
    }
    return true;
}

void SC::EventLoop::runCompletionForNotifiers()
{
    for (ExternalThreadNotifier* first = notifiers.front; first != nullptr; first = first->next)
    {
        if (first->pending.load() == true)
        {
            first->callback(*this);
            first->pending.exchange(false); // allow executing the notification again
        }
    }
}
