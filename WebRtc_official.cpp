// WebRtc_official.cpp: 定义控制台应用程序的入口点。
//	用于测试 google 官方提供的 webrtc 库中的 noise suppression 模块
//	所用文件均与官方程序库中的保持一致

#include "stdafx.h"
#include <iostream>
#include <vector>
#include "AudioFile.cpp"	

#include "noise_suppression.c"
#include "ns_core.c"
#include "defines.h"
#include "typedefs.h"
#include "checks.cc"

#ifndef nullptr
#define nullptr 0
#endif

#ifndef MIN
#define MIN(A, B)        ((A) < (B) ? (A) : (B))
#endif

#define WEBRTC_WIN
enum nsLevel {
	kLow,
	kModerate,
	kHigh,
	kVeryHigh
};
using namespace std;	
NsHandle* nsInit(int sample_rate,nsLevel);
vector<vector<double>> nsProcess(NsHandle* nsHandle,AudioFile<double> &audio_file);

int main()
{
	AudioFile<double> af;
	af.load("NoisySpeech_after_ns.wav");					
	af.printSummary();
	NsHandle *nsHandle = nsInit(af.getSampleRate(), kHigh);		// 最佳采样率为 16k
	vector<vector<double>> output = nsProcess(nsHandle, af);
	af.setAudioBuffer(output);
	af.save("NoisySpeech_after_ns_01.wav");						// 输出的音频采样率、位数与输入一致
    return 0;
}
// TODO:
//	1. 高采样率下的降噪处理
NsHandle* nsInit(int sample_rate, nsLevel ns_level)
{
	// 以下为初始化
	NsHandle *nsHandle = WebRtcNs_Create();
	int status = WebRtcNs_Init(nsHandle, sample_rate);
	if (status != 0) {
		printf("WebRtcNs_Init fail\n");
		return nullptr;
	}
	status = WebRtcNs_set_policy(nsHandle, ns_level);
	if (status != 0) {
		printf("WebRtcNs_set_policy fail\n");
		return nullptr;
	}
	return nsHandle;
};

vector<vector<double>> nsProcess(NsHandle * nsHandle, AudioFile<double> &audio_file)
{
	bool isMono = true;
	vector<vector<double>> input;
	vector<vector<double>> output(2);				// 默认为双声道
	int sample_rate = audio_file.getSampleRate();
	int total_samples = audio_file.getNumSamplesPerChannel();
	input.push_back(audio_file.samples[0]);

	if (audio_file.getNumChannels() > 1) {					// 提取双声道数据
		isMono = false;
		input.push_back(audio_file.samples[1]);
	}
	// load noise suppression module
	size_t samples = MIN(160, sample_rate / 100);	// 最高支持160个点
	const int maxSamples = 320;
	size_t total_frames = (total_samples / samples);		// 处理的帧数

	// 主处理函数（帧处理)
	for (int i = 0; i < total_frames; i++) {
		float data_in[maxSamples];
		float data_out[maxSamples];
		float data_in2[maxSamples];
		float data_out2[maxSamples];
		//  input the signal to process,input points <= 160 (10ms)
		for (int n = 0; n != samples; ++n) {
			data_in[n] = input[0][samples * i + n];
			data_in2[n] = input[1][samples * i + n];
		}
		float *input_buffer[2] = { data_in ,data_in2 };			//ns input buffer [band][data]   band:1~2
		float *output_buffer[2] = { data_out,data_out2 };		//ns output buffer [band][data] band:1~2
																//声明p是一个指针，它指向一个具有2个元素的数组
		WebRtcNs_Analyze(nsHandle, input_buffer[0]);
		WebRtcNs_Process(nsHandle, (const float *const *)input_buffer, isMono ? 1 : 2, output_buffer);	// num_bands = 1 or 2 
		// output the processed signal
		for (int n = 0; n != samples; ++n) {
			output[0].push_back(output_buffer[0][n]);		// Lift band	
			if (!isMono)
				output[1].push_back(output_buffer[1][n]);	// Right band

		}

	}
	WebRtcNs_Free(nsHandle);

	return output;
	//return vector<double>();
}