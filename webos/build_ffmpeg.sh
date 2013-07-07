#!/bin/sh
cd ffmpeg 
test -e .ffmpeg_done &&  exit 0
test -e config.mak || ./configure --enable-cross-compile --target-os=linux --arch=arm --cc=$1 --cxx=$2 --enable-neon --disable-programs  --disable-encoders --disable-outdevs --disable-indevs --disable-muxers --enable-decoder=aac,mp3,pcm_u16le,mpeg4,mpeg1video,mpeg2video,g729,h263,h264 >/dev/null 2>/dev/null && touch .ffmpeg_done
make -j 4 >/dev/null 2>/dev/null 
exit 0
