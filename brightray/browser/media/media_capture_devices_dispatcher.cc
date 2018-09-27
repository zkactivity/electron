// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brightray/browser/media/media_capture_devices_dispatcher.h"

#include <utility>

#include "base/logging.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "chrome/browser/media/webrtc/native_desktop_media_list.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_capture_devices.h"
#include "content/public/common/media_stream_request.h"

namespace brightray {

using content::BrowserThread;
using content::MediaStreamDevices;

namespace {

// Finds a device in |devices| that has |device_id|, or NULL if not found.
const content::MediaStreamDevice* FindDeviceWithId(
    const content::MediaStreamDevices& devices,
    const std::string& device_id) {
  auto iter = devices.begin();
  for (; iter != devices.end(); ++iter) {
    if (iter->id == device_id) {
      return &(*iter);
    }
  }
  return nullptr;
}

const MediaStreamDevices& EmptyDevices() {
  static MediaStreamDevices* devices = new MediaStreamDevices;
  return *devices;
}

}  // namespace

MediaCaptureDevicesDispatcher* MediaCaptureDevicesDispatcher::GetInstance() {
  return base::Singleton<MediaCaptureDevicesDispatcher>::get();
}

std::vector<std::unique_ptr<DesktopMediaList>>
MediaCaptureDevicesDispatcher::CreateMediaList(
    const std::vector<content::DesktopMediaID::Type>& types) {
  // Keep same order as the input |sources| and avoid duplicates.
  std::vector<std::unique_ptr<DesktopMediaList>> source_lists;
  for (auto source_type : types) {
    switch (source_type) {
      case content::DesktopMediaID::TYPE_NONE:
        break;
      case content::DesktopMediaID::TYPE_SCREEN: {
        std::unique_ptr<DesktopMediaList> screen_list =
            std::make_unique<NativeDesktopMediaList>(
                content::DesktopMediaID::TYPE_SCREEN,
                std::move(screen_capturer_));
        source_lists.push_back(std::move(screen_list));
        break;
      }
      case content::DesktopMediaID::TYPE_WINDOW: {
        std::unique_ptr<DesktopMediaList> window_list =
            std::make_unique<NativeDesktopMediaList>(
                content::DesktopMediaID::TYPE_WINDOW,
                std::move(window_capturer_));
        source_lists.push_back(std::move(window_list));
        break;
      }
      case content::DesktopMediaID::TYPE_WEB_CONTENTS:
        break;
    }
  }
  return source_lists;
}

MediaCaptureDevicesDispatcher::MediaCaptureDevicesDispatcher()
    : screen_capturer_(content::desktop_capture::CreateScreenCapturer()),
      window_capturer_(content::desktop_capture::CreateWindowCapturer()),
      is_device_enumeration_disabled_(false) {
  // MediaCaptureDevicesDispatcher is a singleton. It should be created on
  // UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

MediaCaptureDevicesDispatcher::~MediaCaptureDevicesDispatcher() {}

const MediaStreamDevices&
MediaCaptureDevicesDispatcher::GetAudioCaptureDevices() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (is_device_enumeration_disabled_)
    return EmptyDevices();
  return content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
}

const MediaStreamDevices&
MediaCaptureDevicesDispatcher::GetVideoCaptureDevices() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (is_device_enumeration_disabled_)
    return EmptyDevices();
  return content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
}

void MediaCaptureDevicesDispatcher::GetDefaultDevices(
    bool audio,
    bool video,
    content::MediaStreamDevices* devices) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(audio || video);

  if (audio) {
    const content::MediaStreamDevice* device = GetFirstAvailableAudioDevice();
    if (device)
      devices->push_back(*device);
  }

  if (video) {
    const content::MediaStreamDevice* device = GetFirstAvailableVideoDevice();
    if (device)
      devices->push_back(*device);
  }
}

const content::MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetRequestedAudioDevice(
    const std::string& requested_audio_device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const content::MediaStreamDevices& audio_devices = GetAudioCaptureDevices();
  const content::MediaStreamDevice* const device =
      FindDeviceWithId(audio_devices, requested_audio_device_id);
  return device;
}

const content::MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetFirstAvailableAudioDevice() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const content::MediaStreamDevices& audio_devices = GetAudioCaptureDevices();
  if (audio_devices.empty())
    return nullptr;
  return &(*audio_devices.begin());
}

const content::MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetRequestedVideoDevice(
    const std::string& requested_video_device_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const content::MediaStreamDevices& video_devices = GetVideoCaptureDevices();
  const content::MediaStreamDevice* const device =
      FindDeviceWithId(video_devices, requested_video_device_id);
  return device;
}

const content::MediaStreamDevice*
MediaCaptureDevicesDispatcher::GetFirstAvailableVideoDevice() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  const content::MediaStreamDevices& video_devices = GetVideoCaptureDevices();
  if (video_devices.empty())
    return nullptr;
  return &(*video_devices.begin());
}

void MediaCaptureDevicesDispatcher::DisableDeviceEnumerationForTesting() {
  is_device_enumeration_disabled_ = true;
}

void MediaCaptureDevicesDispatcher::OnAudioCaptureDevicesChanged() {}

void MediaCaptureDevicesDispatcher::OnVideoCaptureDevicesChanged() {}

void MediaCaptureDevicesDispatcher::OnMediaRequestStateChanged(
    int render_process_id,
    int render_view_id,
    int page_request_id,
    const GURL& security_origin,
    content::MediaStreamType stream_type,
    content::MediaRequestState state) {}

void MediaCaptureDevicesDispatcher::OnCreatingAudioStream(int render_process_id,
                                                          int render_view_id) {}

void MediaCaptureDevicesDispatcher::OnSetCapturingLinkSecured(
    int render_process_id,
    int render_frame_id,
    int page_request_id,
    content::MediaStreamType stream_type,
    bool is_secure) {}

}  // namespace brightray
