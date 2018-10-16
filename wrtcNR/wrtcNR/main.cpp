//
//  main.cpp
//  wrtcNR
//
//  Created by tangyufeng on 10/15/18.
//  Copyright © 2018 tangyufeng. All rights reserved.
//

#include <iostream>
#include <iostream>
#include <vector>

#include "AudioFile.cpp"
#include "noise_suppression.c"
#include "ns_core.c"
#include "defines.h"
#include "typedefs.h"
#include "checks.cc"

#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

using namespace std;
vector<vector<double>> nsProcess(NsHandle* nsHandle,AudioFile<double> &audio_file);

int main(int argc, const char * argv[]) {
    // insert code here...
    AudioFile<double> audio;
    audio.load("/Users/tangyufeng/lgit/WebRtc_noise_suppression/WebRtc_official/noisegatetest.wav");
    audio.printSummary();
    
    NsHandle *nsHandle = WebRtcNs_Create();
    int status = WebRtcNs_Init(nsHandle, audio.getSampleRate());
    status = WebRtcNs_set_policy(nsHandle, 2);
    
    
    vector<vector<double>> output = nsProcess(nsHandle, audio);
    audio.setAudioBuffer(output);
    audio.save("/Users/tangyufeng/lgit/WebRtc_noise_suppression/WebRtc_official/output.wav");                        // 输出的音频采样率、位数与输入一致
    return 0;
    
}


vector<vector<double>> nsProcess(NsHandle * nsHandle, AudioFile<double> &audio_file)
{
    bool isMono = true;
    vector<vector<double>> input;
    vector<vector<double>> output;                // 默认为双声道
    int sample_rate = audio_file.getSampleRate();
    int total_samples = audio_file.getNumSamplesPerChannel();
    input.push_back(audio_file.samples[0]);
    
    if (audio_file.getNumChannels() > 1) {                    // 提取双声道数据
        isMono = false;
        input.push_back(audio_file.samples[1]);
    }
    // load noise suppression module
    size_t samples = MIN(160, sample_rate / 100);    // 最高支持160个点
    const int maxSamples = 320;
    size_t total_frames = (total_samples / samples);        // 处理的帧数
    
    // 主处理函数（帧处理)
    for (int i = 0; i < total_frames; i++) {
        float data_in[maxSamples];
        float data_out[maxSamples];

        //  input the signal to process,input points <= 160 (10ms)
        for (int n = 0; n != samples; ++n) {
            data_in[n] = input[0][samples * i + n];
           // data_in2[n] = input[1][samples * i + n];
        }
        float *input_buffer[1] = { data_in  };            //ns input buffer [band][data]   band:1~2
        float *output_buffer[1] = { data_out};        //ns output buffer [band][data] band:1~2
        //声明p是一个指针，它指向一个具有2个元素的数组
        WebRtcNs_Analyze(nsHandle, input_buffer[0]);
        WebRtcNs_Process(nsHandle, (const float *const *)input_buffer, 1, output_buffer);    // num_bands = 1 or 2
        // output the processed signal
        for (int n = 0; n != samples; ++n) {
            output[0].push_back(output_buffer[0][n]);        // Lift band
          
            
        }
        
    }
    WebRtcNs_Free(nsHandle);
    
    return output;
    //return vector<double>();
}
