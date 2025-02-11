/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Nest Labs, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Provides an implementation of the PlatformManager object
 *          for Darwin platforms.
 */

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/Darwin/DiagnosticDataProviderImpl.h>
#include <platform/PlatformManager.h>

// Include the non-inline definitions for the GenericPlatformManagerImpl<> template,
#include <platform/internal/GenericPlatformManagerImpl.ipp>

#include <CoreFoundation/CoreFoundation.h>

namespace chip {
namespace DeviceLayer {

PlatformManagerImpl PlatformManagerImpl::sInstance;

CHIP_ERROR PlatformManagerImpl::_InitChipStack()
{
    CHIP_ERROR err;

    // Initialize the configuration system.
    err = Internal::PosixConfig::Init();
    SuccessOrExit(err);
    SetConfigurationMgr(&ConfigurationManagerImpl::GetDefaultInstance());
    SetDiagnosticDataProvider(&DiagnosticDataProviderImpl::GetDefaultInstance());

    mRunLoopSem = dispatch_semaphore_create(0);

    // Ensure there is a dispatch queue available
    GetWorkQueue();

    // Call _InitChipStack() on the generic implementation base class
    // to finish the initialization process.
    err = Internal::GenericPlatformManagerImpl<PlatformManagerImpl>::_InitChipStack();
    SuccessOrExit(err);

    mStartTime = System::SystemClock().GetMonotonicTimestamp();

    static_cast<System::LayerSocketsLoop &>(DeviceLayer::SystemLayer()).SetDispatchQueue(GetWorkQueue());

exit:
    return err;
}

CHIP_ERROR PlatformManagerImpl::_StartEventLoopTask()
{
    if (mIsWorkQueueRunning == false)
    {
        mIsWorkQueueRunning = true;
        dispatch_resume(mWorkQueue);
    }

    return CHIP_NO_ERROR;
};

CHIP_ERROR PlatformManagerImpl::_StopEventLoopTask()
{
    if (mIsWorkQueueRunning == true)
    {
        mIsWorkQueueRunning = false;
        if (dispatch_get_current_queue() != mWorkQueue)
        {
            // dispatch_sync is used in order to guarantee serialization of the caller with
            // respect to any tasks that might already be on the queue, or running.
            dispatch_sync(mWorkQueue, ^{
                dispatch_suspend(mWorkQueue);
            });
        }
        else
        {
            // We are called from a task running on our work queue.  Dispatch async,
            // so we don't deadlock ourselves.  Note that we do have to dispatch to
            // guarantee that we don't signal the semaphore until we have ensured
            // that no more tasks will run on the queue.
            dispatch_async(mWorkQueue, ^{
                dispatch_suspend(mWorkQueue);
                dispatch_semaphore_signal(mRunLoopSem);
            });
        }
    }

    return CHIP_NO_ERROR;
}

void PlatformManagerImpl::_RunEventLoop()
{
    _StartEventLoopTask();

    //
    // Block on the semaphore till we're signalled to stop by
    // _StopEventLoopTask()
    //
    dispatch_semaphore_wait(mRunLoopSem, DISPATCH_TIME_FOREVER);
}

CHIP_ERROR PlatformManagerImpl::_Shutdown()
{
    // Call up to the base class _Shutdown() to perform the bulk of the shutdown.
    return GenericPlatformManagerImpl<ImplClass>::_Shutdown();
}

CHIP_ERROR PlatformManagerImpl::_PostEvent(const ChipDeviceEvent * event)
{
    if (mWorkQueue == nullptr)
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }

    const ChipDeviceEvent eventCopy = *event;
    dispatch_async(mWorkQueue, ^{
        Impl()->DispatchEvent(&eventCopy);
    });
    return CHIP_NO_ERROR;
}

CHIP_ERROR
PlatformManagerImpl::_SetUserLabelList(
    EndpointId endpoint, AttributeList<app::Clusters::UserLabel::Structs::LabelStruct::Type, kMaxUserLabels> & labelList)
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR
PlatformManagerImpl::_GetSupportedLocales(AttributeList<chip::CharSpan, kMaxLanguageTags> & supportedLocales)
{
    // In Darwin simulation, return following hardcoded list of Strings that are valid values for the ActiveLocale.
    supportedLocales.add(CharSpan::fromCharString("Test"));
    supportedLocales.add(CharSpan::fromCharString("en-US"));
    supportedLocales.add(CharSpan::fromCharString("de-DE"));
    supportedLocales.add(CharSpan::fromCharString("fr-FR"));
    supportedLocales.add(CharSpan::fromCharString("en-GB"));
    supportedLocales.add(CharSpan::fromCharString("es-ES"));
    supportedLocales.add(CharSpan::fromCharString("zh-CN"));
    supportedLocales.add(CharSpan::fromCharString("it-IT"));
    supportedLocales.add(CharSpan::fromCharString("ja-JP"));

    return CHIP_NO_ERROR;
}

CHIP_ERROR
PlatformManagerImpl::_GetSupportedCalendarTypes(
    AttributeList<app::Clusters::TimeFormatLocalization::CalendarType, kMaxCalendarTypes> & supportedCalendarTypes)
{
    // In Darwin simulation, return following supported Calendar Types
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kBuddhist);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kChinese);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kCoptic);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kEthiopian);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kGregorian);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kHebrew);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kIndian);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kIslamic);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kJapanese);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kKorean);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kPersian);
    supportedCalendarTypes.add(app::Clusters::TimeFormatLocalization::CalendarType::kTaiwanese);

    return CHIP_NO_ERROR;
}

} // namespace DeviceLayer
} // namespace chip
