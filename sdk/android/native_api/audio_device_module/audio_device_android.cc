/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "sdk/android/native_api/audio_device_module/audio_device_android.h"

#include <stdlib.h>
#include <utility>

#include "rtc_base/logging.h"
#include "rtc_base/ptr_util.h"
#include "rtc_base/refcount.h"
#include "rtc_base/refcountedobject.h"

#if defined(AUDIO_DEVICE_INCLUDE_ANDROID_AAUDIO)
#include "sdk/android/src/jni/audio_device/aaudio_player.h"
#include "sdk/android/src/jni/audio_device/aaudio_recorder.h"
#endif

#include "sdk/android/src/jni/audio_device/audio_record_jni.h"
#include "sdk/android/src/jni/audio_device/audio_track_jni.h"
#include "sdk/android/src/jni/audio_device/opensles_player.h"
#include "sdk/android/src/jni/audio_device/opensles_recorder.h"
#include "system_wrappers/include/metrics.h"

namespace webrtc {

namespace {

void GetDefaultAudioParameters(JNIEnv* env,
                               jobject application_context,
                               AudioParameters* input_parameters,
                               AudioParameters* output_parameters) {
  const JavaParamRef<jobject> j_context(application_context);
  const ScopedJavaLocalRef<jobject> j_audio_manager =
      jni::GetAudioManager(env, j_context);
  const int sample_rate = jni::GetDefaultSampleRate(env, j_audio_manager);
  jni::GetAudioParameters(env, j_context, j_audio_manager, sample_rate,
                          false /* use_stereo_input */,
                          false /* use_stereo_output */, input_parameters,
                          output_parameters);
}

}  // namespace

#if defined(AUDIO_DEVICE_INCLUDE_ANDROID_AAUDIO)
rtc::scoped_refptr<AudioDeviceModule> CreateAAudioAudioDeviceModule(
    JNIEnv* env,
    jobject application_context) {
  RTC_LOG(INFO) << __FUNCTION__;
  // Get default audio input/output parameters.
  AudioParameters input_parameters;
  AudioParameters output_parameters;
  GetDefaultAudioParameters(env, application_context, &input_parameters,
                            &output_parameters);
  // Create ADM from AAudioRecorder and AAudioPlayer.
  return CreateAudioDeviceModuleFromInputAndOutput(
      AudioDeviceModule::kAndroidAAudioAudio, false /* use_stereo_input */,
      false /* use_stereo_output */,
      jni::kLowLatencyModeDelayEstimateInMilliseconds,
      rtc::MakeUnique<jni::AAudioRecorder>(input_parameters),
      rtc::MakeUnique<jni::AAudioPlayer>(output_parameters));
}
#endif

rtc::scoped_refptr<AudioDeviceModule> CreateJavaAudioDeviceModule(
    JNIEnv* env,
    jobject application_context) {
  RTC_LOG(INFO) << __FUNCTION__;
  // Get default audio input/output parameters.
  const JavaParamRef<jobject> j_context(application_context);
  const ScopedJavaLocalRef<jobject> j_audio_manager =
      jni::GetAudioManager(env, j_context);
  AudioParameters input_parameters;
  AudioParameters output_parameters;
  GetDefaultAudioParameters(env, application_context, &input_parameters,
                            &output_parameters);
  // Create ADM from AudioRecord and AudioTrack.
  auto audio_input = rtc::MakeUnique<jni::AudioRecordJni>(
      env, input_parameters, jni::kHighLatencyModeDelayEstimateInMilliseconds,
      jni::AudioRecordJni::CreateJavaWebRtcAudioRecord(env, j_context,
                                                       j_audio_manager));
  auto audio_output = rtc::MakeUnique<jni::AudioTrackJni>(
      env, output_parameters,
      jni::AudioTrackJni::CreateJavaWebRtcAudioTrack(env, j_context,
                                                     j_audio_manager));
  return CreateAudioDeviceModuleFromInputAndOutput(
      AudioDeviceModule::kAndroidJavaAudio, false /* use_stereo_input */,
      false /* use_stereo_output */,
      jni::kHighLatencyModeDelayEstimateInMilliseconds, std::move(audio_input),
      std::move(audio_output));
}

rtc::scoped_refptr<AudioDeviceModule> CreateOpenSLESAudioDeviceModule(
    JNIEnv* env,
    jobject application_context) {
  RTC_LOG(INFO) << __FUNCTION__;
  // Get default audio input/output parameters.
  AudioParameters input_parameters;
  AudioParameters output_parameters;
  GetDefaultAudioParameters(env, application_context, &input_parameters,
                            &output_parameters);
  // Create ADM from OpenSLESRecorder and OpenSLESPlayer.
  auto engine_manager = rtc::MakeUnique<jni::OpenSLEngineManager>();
  auto audio_input = rtc::MakeUnique<jni::OpenSLESRecorder>(
      input_parameters, engine_manager.get());
  auto audio_output = rtc::MakeUnique<jni::OpenSLESPlayer>(
      output_parameters, std::move(engine_manager));
  return CreateAudioDeviceModuleFromInputAndOutput(
      AudioDeviceModule::kAndroidOpenSLESAudio, false /* use_stereo_input */,
      false /* use_stereo_output */,
      jni::kLowLatencyModeDelayEstimateInMilliseconds, std::move(audio_input),
      std::move(audio_output));
}

rtc::scoped_refptr<AudioDeviceModule>
CreateJavaInputAndOpenSLESOutputAudioDeviceModule(JNIEnv* env,
                                                  jobject application_context) {
  RTC_LOG(INFO) << __FUNCTION__;
  // Get default audio input/output parameters.
  const JavaParamRef<jobject> j_context(application_context);
  const ScopedJavaLocalRef<jobject> j_audio_manager =
      jni::GetAudioManager(env, j_context);
  AudioParameters input_parameters;
  AudioParameters output_parameters;
  GetDefaultAudioParameters(env, application_context, &input_parameters,
                            &output_parameters);
  // Create ADM from AudioRecord and OpenSLESPlayer.
  auto audio_input = rtc::MakeUnique<jni::AudioRecordJni>(
      env, input_parameters, jni::kLowLatencyModeDelayEstimateInMilliseconds,
      jni::AudioRecordJni::CreateJavaWebRtcAudioRecord(env, j_context,
                                                       j_audio_manager));
  auto audio_output = rtc::MakeUnique<jni::OpenSLESPlayer>(
      output_parameters, rtc::MakeUnique<jni::OpenSLEngineManager>());
  return CreateAudioDeviceModuleFromInputAndOutput(
      AudioDeviceModule::kAndroidJavaInputAndOpenSLESOutputAudio,
      false /* use_stereo_input */, false /* use_stereo_output */,
      jni::kLowLatencyModeDelayEstimateInMilliseconds, std::move(audio_input),
      std::move(audio_output));
}

}  // namespace webrtc
