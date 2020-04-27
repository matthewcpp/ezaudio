#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "ezaudio/ez_audio.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <array>

#define STR(x) #x
#define TO_STR(x) STR(x)

uint32_t audio_callback(void* buffer, uint32_t buffer_size, uint32_t sample_count, void* user_data)
{
	tsf* soundfont = reinterpret_cast<tsf*>(user_data);
	tsf_render_float(soundfont, reinterpret_cast<float*>(buffer), sample_count, 0);
    return buffer_size;
}

int main(int argc, char** argv)
{
	std::string soundfontPath = TO_STR(SOUNDFONT_FILE);

	auto soundfont = tsf_load_filename(soundfontPath.c_str());
	if (!soundfont) {
		std::cout << "Failed to load soundfont.  Tried: " << soundfontPath << std::endl;
		return 1;
	}

	ez_audio_params params;
	ez_audio_init_params(&params);

	params.frequency = 44100;
	params.render_callback = audio_callback;
	params.user_data = soundfont;

	tsf_set_output(soundfont, TSF_STEREO_INTERLEAVED, params.frequency, 0.0f);

	auto ez_audio = ez_audio_create(&params);
	if (!ez_audio) {
		std::cout << "Failed to create ez_audio session." << std::endl;
		tsf_close(soundfont);
		return 1;
	}
	
	int error = ez_audio_start(ez_audio);
	if (error) {
		std::cout << "failed to start ez_audio session." << std::endl;
		ez_audio_destroy(ez_audio);
		tsf_close(soundfont);

		return 1;
	}

	std::array<int, 3> c_major = { 60, 64, 67 };
	for (const auto note : c_major)
		tsf_note_on(soundfont, 0, note, 1.0f);

	std::this_thread::sleep_for(std::chrono::seconds(2));

	ez_audio_destroy(ez_audio);
	tsf_close(soundfont);

	return 0;
}
