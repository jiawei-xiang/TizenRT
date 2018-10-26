/******************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/
#include <pthread.h>
#include <tinyara/audio/audio.h>
#include <sys/types.h>
#include <debug.h>
#include <unistd.h>

#include "HardwareKeywordDetector.h"
#include "../audio/audio_manager.h"

namespace media {
namespace voice {

HardwareKeywordDetector::HardwareKeywordDetector(int card, int device)
	: mCard(card)
	, mDevice(device)
{

}

bool HardwareKeywordDetector::init(uint32_t samprate, uint8_t channels)
{
	audio_manager_result_t result;

	result = register_stream_in_device_process_type(
		mCard, mDevice,
		AUDIO_DEVICE_PROCESS_TYPE_SPEECH_DETECTOR,
		AUDIO_DEVICE_SPEECH_DETECT_KD);

	if (result != AUDIO_MANAGER_SUCCESS) {
		meddbg("Error: register_stream_in_device_process_type(%d, %d) failed!\n", mCard, mDevice);
		return false;
	}

	result = register_stream_in_device_process_handler(
		mCard, mDevice,
		AUDIO_DEVICE_PROCESS_TYPE_SPEECH_DETECTOR);

	if (result != AUDIO_MANAGER_SUCCESS) {
		meddbg("Error: register_stream_in_device_process_handler(%d, %d) failed!\n", mCard, mDevice);
		return false;
	}

	return true;
}

void HardwareKeywordDetector::deinit()
{
	audio_manager_result_t result;

	result = unregister_stream_in_device_process(mCard, mDevice);

	if (result != AUDIO_MANAGER_SUCCESS) {
		meddbg("Error: unregister_stream_in_device_process(%d, %d) failed!\n", mCard, mDevice);
	}
}

bool HardwareKeywordDetector::startKeywordDetect(uint32_t timeout)
{
	bool ret = false;
	audio_manager_result_t result;

	medvdbg("startKeywordDetect for %d %d\n", mCard, mDevice);

	result = start_stream_in_device_process_type(mCard, mDevice, AUDIO_DEVICE_SPEECH_DETECT_KD);

	if (result != AUDIO_MANAGER_SUCCESS) {
		meddbg("Error: start_stream_in_device_process(%d, %d) failed!\n", mCard, mDevice);
		return ret;
	}

	struct timespec curtime;
	struct timespec waketime;
	clock_gettime(CLOCK_REALTIME, &waketime);
	waketime.tv_sec += timeout;

	do {
		uint16_t msgId;
		result = get_device_process_handler_message(mCard, mDevice, &msgId);

		if (result == AUDIO_MANAGER_SUCCESS) {
			if (msgId == AUDIO_DEVICE_SPEECH_DETECT_KD) {
				medvdbg("#### KD DETECTED!! ####\n");
				ret = true;
				break;
			}
		} else if (result == AUDIO_MANAGER_INVALID_DEVICE) {
			meddbg("Error: device doesn't support it!!!\n");
			break;
		}

		pthread_yield();
		clock_gettime(CLOCK_REALTIME, &curtime);
	} while ((curtime.tv_sec < waketime.tv_sec) ||
			 (curtime.tv_sec == waketime.tv_sec && curtime.tv_nsec <= waketime.tv_nsec));

	stop_stream_in_device_process_type(mCard, mDevice, AUDIO_DEVICE_SPEECH_DETECT_KD);
	return ret;
}

} // namespace voice
} // namespace media
