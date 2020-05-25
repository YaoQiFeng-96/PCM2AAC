#pragma once

#include <stdio.h>
#include <iostream>

extern "C"
{
#include "libavformat\avformat.h"
#include "libavcodec\avcodec.h"
#include "libavutil\time.h"
#include "libswresample/swresample.h"
}

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")