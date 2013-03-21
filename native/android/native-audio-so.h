#pragma once

// Header for dynamic loading

typedef void (*AndroidAudioCallback)(short *buffer, int num_samples);

typedef bool (*OpenSLWrap_Init_T)(AndroidAudioCallback cb);
typedef void (*OpenSLWrap_Shutdown_T)();
