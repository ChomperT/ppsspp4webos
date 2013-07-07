
CC 	= arm-linux-gnueabi-gcc
CXX 	= arm-linux-gnueabi-g++
STRIP 	= arm-linux-gnueabi-strip

APP_NAME = ppsspp
SRC_PATH = ..

CFLAGS = -D_NDEBUG -DWEBOS -DARM -DARMV7 -DARMEABI_V7A -DUSE_FFMPEG -DUSE_PROFILER -DGL_GLEXT_PROTOTYPES -DUSING_GLES2 -Ofast -fsigned-char -Wall -Wno-multichar -Wno-psabi -Wno-unused-variable -fno-strict-aliasing -ffast-math
CPPFLAGS = -std=gnu++11 -fno-rtti -fpermissive $(CFLAGS)

C_INCLUDES = \
	-I./ffmpeg \
	-I$(SRC_PATH)/Common \
	-I$(SRC_PATH)/native/base \
	-I$(SRC_PATH)/native/ext/libzip \
	-I$(SRC_PATH)/native \
	-I$(SRC_PATH)

ARCH_FILES = \
	$(SRC_PATH)/Common/ArmEmitter.cpp \
	$(SRC_PATH)/Common/ArmCPUDetect.cpp \
	$(SRC_PATH)/Common/ArmThunk.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmCompALU.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmCompBranch.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmCompFPU.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmCompLoadStore.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmCompVFPU.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmAsm.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmJit.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmRegCache.cpp \
	$(SRC_PATH)/Core/MIPS/ARM/ArmRegCacheFPU.cpp 

SRCS = \
	$(ARCH_FILES) \
	$(SRC_PATH)/android/jni/ArmEmitterTest.cpp \
	$(SRC_PATH)/android/jni/TestRunner.cpp \
	$(SRC_PATH)/UI/ui_atlas.cpp \
	$(SRC_PATH)/UI/NativeApp.cpp \
	$(SRC_PATH)/UI/EmuScreen.cpp \
	$(SRC_PATH)/UI/MenuScreens.cpp \
	$(SRC_PATH)/UI/MainScreen.cpp \
	$(SRC_PATH)/UI/UIShader.cpp \
	$(SRC_PATH)/UI/GamepadEmu.cpp \
	$(SRC_PATH)/UI/GameInfoCache.cpp \
	$(SRC_PATH)/UI/OnScreenDisplay.cpp \
	$(SRC_PATH)/UI/PluginScreen.cpp \
	$(SRC_PATH)/UI/GameScreen.cpp \
	$(SRC_PATH)/UI/GameSettingsScreen.cpp \
	$(SRC_PATH)/ext/disarm.cpp \
	$(SRC_PATH)/ext/libkirk/AES.c \
	$(SRC_PATH)/ext/libkirk/amctrl.c \
	$(SRC_PATH)/ext/libkirk/SHA1.c \
	$(SRC_PATH)/ext/libkirk/bn.c \
	$(SRC_PATH)/ext/libkirk/ec.c \
	$(SRC_PATH)/ext/libkirk/kirk_engine.c \
	$(SRC_PATH)/ext/snappy/snappy-c.cpp \
	$(SRC_PATH)/ext/snappy/snappy.cpp \
	$(SRC_PATH)/ext/xbrz/xbrz.cpp \
	$(SRC_PATH)/Common/Crypto/md5.cpp \
	$(SRC_PATH)/Common/KeyMap.cpp \
	$(SRC_PATH)/Common/LogManager.cpp \
	$(SRC_PATH)/Common/MemArena.cpp \
	$(SRC_PATH)/Common/MemoryUtil.cpp \
	$(SRC_PATH)/Common/MsgHandler.cpp \
	$(SRC_PATH)/Common/FileUtil.cpp \
	$(SRC_PATH)/Common/StringUtils.cpp \
	$(SRC_PATH)/Common/Thread.cpp \
	$(SRC_PATH)/Common/ThreadPools.cpp \
	$(SRC_PATH)/Common/Timer.cpp \
	$(SRC_PATH)/Common/Misc.cpp \
	$(SRC_PATH)/Common/MathUtil.cpp \
	$(SRC_PATH)/GPU/Math3D.cpp \
	$(SRC_PATH)/GPU/GPUCommon.cpp \
	$(SRC_PATH)/GPU/GPUState.cpp \
	$(SRC_PATH)/GPU/GeDisasm.cpp \
	$(SRC_PATH)/GPU/GLES/Framebuffer.cpp \
	$(SRC_PATH)/GPU/GLES/DisplayListInterpreter.cpp \
	$(SRC_PATH)/GPU/GLES/TextureCache.cpp \
	$(SRC_PATH)/GPU/GLES/IndexGenerator.cpp \
	$(SRC_PATH)/GPU/GLES/TransformPipeline.cpp \
	$(SRC_PATH)/GPU/GLES/StateMapping.cpp \
	$(SRC_PATH)/GPU/GLES/VertexDecoder.cpp \
	$(SRC_PATH)/GPU/GLES/ShaderManager.cpp \
	$(SRC_PATH)/GPU/GLES/VertexShaderGenerator.cpp \
	$(SRC_PATH)/GPU/GLES/FragmentShaderGenerator.cpp \
	$(SRC_PATH)/GPU/GLES/TextureScaler.cpp \
	$(SRC_PATH)/GPU/Null/NullGpu.cpp \
	$(SRC_PATH)/Core/ELF/ElfReader.cpp \
	$(SRC_PATH)/Core/ELF/PBPReader.cpp \
	$(SRC_PATH)/Core/ELF/PrxDecrypter.cpp \
	$(SRC_PATH)/Core/ELF/ParamSFO.cpp \
	$(SRC_PATH)/Core/HW/atrac3plus.cpp \
	$(SRC_PATH)/Core/HW/MemoryStick.cpp \
	$(SRC_PATH)/Core/HW/MpegDemux.cpp \
	$(SRC_PATH)/Core/HW/OMAConvert.cpp \
	$(SRC_PATH)/Core/HW/MediaEngine.cpp \
	$(SRC_PATH)/Core/HW/SasAudio.cpp \
	$(SRC_PATH)/Core/Core.cpp \
	$(SRC_PATH)/Core/Config.cpp \
	$(SRC_PATH)/Core/CoreTiming.cpp \
	$(SRC_PATH)/Core/CPU.cpp \
	$(SRC_PATH)/Core/CwCheat.cpp \
	$(SRC_PATH)/Core/HDRemaster.cpp \
	$(SRC_PATH)/Core/Host.cpp \
	$(SRC_PATH)/Core/Loaders.cpp \
	$(SRC_PATH)/Core/PSPLoaders.cpp \
	$(SRC_PATH)/Core/MemMap.cpp \
	$(SRC_PATH)/Core/MemMapFunctions.cpp \
	$(SRC_PATH)/Core/Reporting.cpp \
	$(SRC_PATH)/Core/SaveState.cpp \
	$(SRC_PATH)/Core/System.cpp \
	$(SRC_PATH)/Core/PSPMixer.cpp \
	$(SRC_PATH)/Core/Debugger/Breakpoints.cpp \
	$(SRC_PATH)/Core/Debugger/SymbolMap.cpp \
	$(SRC_PATH)/Core/Dialog/PSPDialog.cpp \
	$(SRC_PATH)/Core/Dialog/PSPMsgDialog.cpp \
	$(SRC_PATH)/Core/Dialog/PSPOskDialog.cpp \
	$(SRC_PATH)/Core/Dialog/PSPPlaceholderDialog.cpp \
	$(SRC_PATH)/Core/Dialog/PSPSaveDialog.cpp \
	$(SRC_PATH)/Core/Dialog/SavedataParam.cpp \
	$(SRC_PATH)/Core/Font/PGF.cpp \
	$(SRC_PATH)/Core/HLE/HLETables.cpp \
	$(SRC_PATH)/Core/HLE/HLE.cpp \
	$(SRC_PATH)/Core/HLE/sceAtrac.cpp \
	$(SRC_PATH)/Core/HLE/__sceAudio.cpp \
	$(SRC_PATH)/Core/HLE/sceAudio.cpp \
	$(SRC_PATH)/Core/HLE/sceAudiocodec.cpp \
	$(SRC_PATH)/Core/HLE/sceChnnlsv.cpp \
	$(SRC_PATH)/Core/HLE/sceCcc.cpp \
	$(SRC_PATH)/Core/HLE/sceCtrl.cpp \
	$(SRC_PATH)/Core/HLE/sceDeflt.cpp \
	$(SRC_PATH)/Core/HLE/sceDisplay.cpp \
	$(SRC_PATH)/Core/HLE/sceDmac.cpp \
	$(SRC_PATH)/Core/HLE/sceGe.cpp \
	$(SRC_PATH)/Core/HLE/sceFont.cpp \
	$(SRC_PATH)/Core/HLE/sceHprm.cpp \
	$(SRC_PATH)/Core/HLE/sceHttp.cpp \
	$(SRC_PATH)/Core/HLE/sceImpose.cpp \
	$(SRC_PATH)/Core/HLE/sceIo.cpp \
	$(SRC_PATH)/Core/HLE/sceJpeg.cpp \
	$(SRC_PATH)/Core/HLE/sceKernel.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelAlarm.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelEventFlag.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelInterrupt.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelMemory.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelModule.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelMutex.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelMbx.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelMsgPipe.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelSemaphore.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelThread.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelTime.cpp \
	$(SRC_PATH)/Core/HLE/sceKernelVTimer.cpp \
	$(SRC_PATH)/Core/HLE/sceMpeg.cpp \
	$(SRC_PATH)/Core/HLE/sceMd5.cpp \
	$(SRC_PATH)/Core/HLE/sceMp4.cpp \
	$(SRC_PATH)/Core/HLE/sceMp3.cpp \
	$(SRC_PATH)/Core/HLE/sceNet.cpp \
	$(SRC_PATH)/Core/HLE/sceOpenPSID.cpp \
	$(SRC_PATH)/Core/HLE/sceP3da.cpp \
	$(SRC_PATH)/Core/HLE/sceParseHttp.cpp \
	$(SRC_PATH)/Core/HLE/sceParseUri.cpp \
	$(SRC_PATH)/Core/HLE/scePower.cpp \
	$(SRC_PATH)/Core/HLE/sceRtc.cpp \
	$(SRC_PATH)/Core/HLE/scePsmf.cpp \
	$(SRC_PATH)/Core/HLE/sceSas.cpp \
	$(SRC_PATH)/Core/HLE/sceSsl.cpp \
	$(SRC_PATH)/Core/HLE/sceUmd.cpp \
	$(SRC_PATH)/Core/HLE/sceUsb.cpp \
	$(SRC_PATH)/Core/HLE/sceUtility.cpp \
	$(SRC_PATH)/Core/HLE/sceVaudio.cpp \
	$(SRC_PATH)/Core/HLE/scePspNpDrm_user.cpp \
	$(SRC_PATH)/Core/HLE/sceGameUpdate.cpp \
	$(SRC_PATH)/Core/HLE/sceNp.cpp \
	$(SRC_PATH)/Core/HLE/scePauth.cpp \
	$(SRC_PATH)/Core/FileSystems/BlockDevices.cpp \
	$(SRC_PATH)/Core/FileSystems/ISOFileSystem.cpp \
	$(SRC_PATH)/Core/FileSystems/MetaFileSystem.cpp \
	$(SRC_PATH)/Core/FileSystems/DirectoryFileSystem.cpp \
	$(SRC_PATH)/Core/FileSystems/tlzrc.cpp \
	$(SRC_PATH)/Core/MIPS/MIPS.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSAnalyst.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSDis.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSDisVFPU.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSInt.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSIntVFPU.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSTables.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSVFPUUtils.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSCodeUtils.cpp \
	$(SRC_PATH)/Core/MIPS/MIPSDebugInterface.cpp \
	$(SRC_PATH)/Core/MIPS/JitCommon/JitCommon.cpp \
	$(SRC_PATH)/Core/MIPS/JitCommon/JitBlockCache.cpp \
	$(SRC_PATH)/Core/Util/BlockAllocator.cpp \
	$(SRC_PATH)/Core/Util/ppge_atlas.cpp \
	$(SRC_PATH)/Core/Util/PPGeDraw.cpp 

NATIVE_SRCS =\
	android/native_audio.cpp \
	audio/wav_read.cpp \
	audio/mixer.cpp \
	base/buffer.cpp \
	base/display.cpp \
	base/timeutil.cpp \
	base/colorutil.cpp \
	base/error_context.cpp \
	base/stringutil.cpp \
	data/compression.cpp \
	ext/rg_etc1/rg_etc1.cpp \
	ext/cityhash/city.cpp \
	ext/jpge/jpgd.cpp \
	ext/jpge/jpge.cpp \
	ext/sha1/sha1.cpp \
	ext/stb_image/stb_image.c \
	ext/stb_vorbis/stb_vorbis.cpp \
	ext/vjson/json.cpp \
	ext/vjson/block_allocator.cpp \
	file/dialog.cpp \
	file/fd_util.cpp \
	file/easy_file.cpp \
	file/chunk_file.cpp \
	file/file_util.cpp \
	file/ini_file.cpp \
	file/zip_read.cpp \
	json/json_writer.cpp \
	i18n/i18n.cpp \
	input/gesture_detector.cpp \
	input/input_state.cpp \
	math/math_util.cpp \
	math/curves.cpp \
	math/expression_parser.cpp \
	math/lin/aabb.cpp \
	math/lin/plane.cpp \
	math/lin/quat.cpp \
	math/lin/vec3.cpp \
	math/lin/matrix4x4.cpp \
	midi/midi_input.cpp \
	net/http_client.cpp \
	net/resolve.cpp \
	net/url.cpp \
	profiler/profiler.cpp \
	thread/threadutil.cpp \
	thread/prioritizedworkqueue.cpp \
	thread/threadpool.cpp \
	gfx_es2/glsl_program.cpp \
	gfx_es2/gl_state.cpp \
	gfx_es2/draw_buffer.cpp \
	gfx_es2/vertex_format.cpp \
	gfx_es2/fbo.cpp \
	gfx/gl_debug_log.cpp \
	gfx/gl_lost_manager.cpp \
	gfx/texture.cpp \
	gfx/texture_atlas.cpp \
	gfx/texture_gen.cpp \
	image/zim_load.cpp \
	image/zim_save.cpp \
	image/png_load.cpp \
	ui/view.cpp \
	ui/viewgroup.cpp \
	ui/ui.cpp \
	ui/ui_screen.cpp \
	ui/ui_context.cpp \
	ui/screen.cpp \
	ui/virtual_input.cpp \
	util/random/perlin.cpp \
	util/text/utf8.cpp

SRCS += $(addprefix $(SRC_PATH)/native/,$(NATIVE_SRCS))
SRCS += webos.cpp
OBJS = $(addsuffix .o, $(basename $(SRCS)))

LIBS += ffmpeg/libavformat/*.a
LIBS += ffmpeg/libavcodec/*.a
LIBS += ffmpeg/libswresample/*.a
LIBS += ffmpeg/libswscale/*.a
LIBS += ffmpeg/libavutil/*.a
LIBS += -lGLESv2 -ldl -lpthread -lz -lSDL -lpdl  -Wl,--allow-shlib-undefined -static-libstdc++ -static-libgcc


%.o:%.c
	@echo CC $*.o 
	@$(CC) $(CFLAGS) $(C_INCLUDES) $< -o $(addsuffix .o, $(basename $<)) 2>>build.log -c

%.o:%.cpp
	@echo CXX $*.o
	@$(CXX) $(CPPFLAGS) $(C_INCLUDES) $< -o $(addsuffix .o, $(basename $<)) 2>>build.log -c

all: program

$(OBJS): ffmpeg_libs

#program: $(OBJS)
program:  ffmpeg_libs webos.o
	@echo LINK $(APP_NAME)
	$(CXX) $(CPPFLAGS) $(OBJS) $(LIBS) -o $(APP_NAME)
	$(STRIP) $(APP_NAME)

ffmpeg: 
	git clone git://source.ffmpeg.org/ffmpeg.git ffmpeg 

ffmpeg_libs: ffmpeg
	@echo Compiling ffmpeg
	$(shell ./build_ffmpeg.sh $(CC) $(CXX))
	@echo Done.

clean:
	@make -C ffmpeg distclean || true
	@rm $(OBJS) $(APP_NAME) ffmpeg/.ffmpeg_done build.log 2>/dev/null || true 
