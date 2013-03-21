CXX		= arm-linux-gnueabi-g++ 
CC		= arm-linux-gnueabi-gcc

APPNAME		= ppsspp4webos

SRC_ROOT	= ..

HEADERS		= -I$(SRC_ROOT) -I$(SRC_ROOT)/native -I$(SRC_ROOT)/Common 
CXXFLAGS 	= -Ofast -D_NDEBUG -DARM -DUSING_GLES2 -Wno-multichar -fno-strict-aliasing -ffast-math -Wno-psabi -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -std=gnu++0x -fbuiltin
CFLAGS 	= -Ofast -D_NDEBUG -DARM -DUSING_GLES2 -Wno-multichar -fno-strict-aliasing -ffast-math -Wno-psabi -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 

C_SRC 		= \
		  ${SRC_ROOT}/native/ext/stb_image/stb_image.c \
		  ${SRC_ROOT}/native/ext/stb_vorbis/stb_vorbis.c \
		  ${SRC_ROOT}/ext/libkirk/AES.c \
		  ${SRC_ROOT}/ext/libkirk/amctrl.c \
		  ${SRC_ROOT}/ext/libkirk/SHA1.c \
		  ${SRC_ROOT}/ext/libkirk/bn.c \
		  ${SRC_ROOT}/ext/libkirk/ec.c \
		  ${SRC_ROOT}/ext/libkirk/kirk_engine.c 

 CPP_SRC		= \
		  ${SRC_ROOT}/Common/ArmCPUDetect.cpp \
		  ${SRC_ROOT}/Common/ArmEmitter.cpp \
		  ${SRC_ROOT}/Common/ArmThunk.cpp \
		  ${SRC_ROOT}/Common/ColorUtil.cpp \
		  ${SRC_ROOT}/Common/ConsoleListener.cpp \
		  ${SRC_ROOT}/Common/Crypto/aes_cbc.cpp \
		  ${SRC_ROOT}/Common/Crypto/aes_core.cpp \
		  ${SRC_ROOT}/Common/Crypto/bn.cpp \
		  ${SRC_ROOT}/Common/Crypto/ec.cpp \
		  ${SRC_ROOT}/Common/Crypto/md5.cpp \
		  ${SRC_ROOT}/Common/Crypto/sha1.cpp \
		  ${SRC_ROOT}/Common/ExtendedTrace.cpp \
		  ${SRC_ROOT}/Common/FPURoundModeGeneric.cpp \
		  ${SRC_ROOT}/Common/FileSearch.cpp \
		  ${SRC_ROOT}/Common/FileUtil.cpp \
		  ${SRC_ROOT}/Common/Hash.cpp \
		  ${SRC_ROOT}/Common/IniFile.cpp \
		  ${SRC_ROOT}/Common/LogManager.cpp \
		  ${SRC_ROOT}/Common/MathUtil.cpp \
		  ${SRC_ROOT}/Common/MemArena.cpp \
		  ${SRC_ROOT}/Common/MemoryUtil.cpp \
		  ${SRC_ROOT}/Common/Misc.cpp \
		  ${SRC_ROOT}/Common/MsgHandler.cpp \
		  ${SRC_ROOT}/Common/StringUtil.cpp \
		  ${SRC_ROOT}/Common/Thread.cpp \
		  ${SRC_ROOT}/Common/Timer.cpp \
		  ${SRC_ROOT}/Common/Version.cpp \
		  ${SRC_ROOT}/ext/snappy/snappy-c.cpp \
		  ${SRC_ROOT}/ext/snappy/snappy.cpp \
		  ${SRC_ROOT}/native/ext/etcpack/etcdec.cpp \
		  ${SRC_ROOT}/native/ext/etcpack/etcpack.cpp \
		  ${SRC_ROOT}/native/ext/etcpack/image.cpp \
		  ${SRC_ROOT}/native/ext/cityhash/city.cpp \
		  ${SRC_ROOT}/native/ext/sha1/sha1.cpp \
		  ${SRC_ROOT}/native/audio/mixer.cpp \
		  ${SRC_ROOT}/native/audio/wav_read.cpp \
		  ${SRC_ROOT}/native/base/buffer.cpp \
		  ${SRC_ROOT}/native/base/colorutil.cpp \
		  ${SRC_ROOT}/native/base/display.cpp \
		  ${SRC_ROOT}/native/base/error_context.cpp \
		  ${SRC_ROOT}/native/base/fastlist_test.cpp \
		  ${SRC_ROOT}/native/base/stringutil.cpp \
		  ${SRC_ROOT}/native/base/threadutil.cpp \
		  ${SRC_ROOT}/native/base/timeutil.cpp \
		  ${SRC_ROOT}/native/file/chunk_file.cpp \
		  ${SRC_ROOT}/native/file/dialog.cpp \
		  ${SRC_ROOT}/native/file/easy_file.cpp \
		  ${SRC_ROOT}/native/file/fd_util.cpp \
		  ${SRC_ROOT}/native/file/file_util.cpp \
		  ${SRC_ROOT}/native/file/zip_read.cpp \
		  ${SRC_ROOT}/native/gfx/gl_debug_log.cpp \
		  ${SRC_ROOT}/native/gfx/gl_lost_manager.cpp \
		  ${SRC_ROOT}/native/gfx/texture.cpp \
		  ${SRC_ROOT}/native/gfx/texture_atlas.cpp \
		  ${SRC_ROOT}/native/gfx/texture_gen.cpp \
		  ${SRC_ROOT}/native/gfx_es2/draw_buffer.cpp \
		  ${SRC_ROOT}/native/gfx_es2/fbo.cpp \
		  ${SRC_ROOT}/native/gfx_es2/gl_state.cpp \
		  ${SRC_ROOT}/native/gfx_es2/glsl_program.cpp \
		  ${SRC_ROOT}/native/gfx_es2/vertex_format.cpp \
		  ${SRC_ROOT}/native/image/png_load.cpp \
		  ${SRC_ROOT}/native/image/zim_load.cpp \
		  ${SRC_ROOT}/native/image/zim_save.cpp \
		  ${SRC_ROOT}/native/input/gesture_detector.cpp \
		  ${SRC_ROOT}/native/json/json_writer.cpp \
		  ${SRC_ROOT}/native/math/curves.cpp \
		  ${SRC_ROOT}/native/math/lin/aabb.cpp \
		  ${SRC_ROOT}/native/math/lin/matrix4x4.cpp \
		  ${SRC_ROOT}/native/math/lin/plane.cpp \
		  ${SRC_ROOT}/native/math/lin/quat.cpp \
		  ${SRC_ROOT}/native/math/lin/vec3.cpp \
		  ${SRC_ROOT}/native/math/math_util.cpp \
		  ${SRC_ROOT}/native/midi/midi_input.cpp \
		  ${SRC_ROOT}/native/net/http_client.cpp \
		  ${SRC_ROOT}/native/net/resolve.cpp \
		  ${SRC_ROOT}/native/profiler/profiler.cpp \
		  ${SRC_ROOT}/native/ui/screen.cpp \
		  ${SRC_ROOT}/native/ui/ui.cpp \
		  ${SRC_ROOT}/native/ui/virtual_input.cpp \
		  ${SRC_ROOT}/native/util/bits/bits.cpp \
		  ${SRC_ROOT}/native/util/bits/varint.cpp \
		  ${SRC_ROOT}/native/util/hash/hash.cpp \
		  ${SRC_ROOT}/native/util/random/perlin.cpp \
		  ${SRC_ROOT}/native/util/text/utf8.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmAsm.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmCompALU.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmCompBranch.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmCompFPU.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmCompLoadStore.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmCompVFPU.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmJit.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmJitCache.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmRegCache.cpp \
		  ${SRC_ROOT}/Core/MIPS/ARM/ArmRegCacheFPU.cpp \
		  ${SRC_ROOT}/ext/disarm.cpp \
		  ${SRC_ROOT}/Core/CPU.cpp \
		  ${SRC_ROOT}/Core/Config.cpp \
		  ${SRC_ROOT}/Core/Core.cpp \
		  ${SRC_ROOT}/Core/CoreTiming.cpp \
		  ${SRC_ROOT}/Core/Debugger/Breakpoints.cpp \
		  ${SRC_ROOT}/Core/Debugger/SymbolMap.cpp \
		  ${SRC_ROOT}/Core/Dialog/PSPDialog.cpp \
		  ${SRC_ROOT}/Core/Dialog/PSPMsgDialog.cpp \
		  ${SRC_ROOT}/Core/Dialog/PSPOskDialog.cpp \
		  ${SRC_ROOT}/Core/Dialog/PSPPlaceholderDialog.cpp \
		  ${SRC_ROOT}/Core/Dialog/PSPSaveDialog.cpp \
		  ${SRC_ROOT}/Core/Dialog/SavedataParam.cpp \
		  ${SRC_ROOT}/Core/ELF/ElfReader.cpp \
		  ${SRC_ROOT}/Core/ELF/PrxDecrypter.cpp \
		  ${SRC_ROOT}/Core/ELF/ParamSFO.cpp \
		  ${SRC_ROOT}/Core/FileSystems/BlockDevices.cpp \
		  ${SRC_ROOT}/Core/FileSystems/DirectoryFileSystem.cpp \
		  ${SRC_ROOT}/Core/FileSystems/ISOFileSystem.cpp \
		  ${SRC_ROOT}/Core/FileSystems/MetaFileSystem.cpp \
		  ${SRC_ROOT}/Core/Font/PGF.cpp \
		  ${SRC_ROOT}/Core/HLE/HLE.cpp \
		  ${SRC_ROOT}/Core/HLE/HLETables.cpp \
		  ${SRC_ROOT}/Core/HLE/__sceAudio.cpp \
		  ${SRC_ROOT}/Core/HLE/sceAtrac.cpp \
		  ${SRC_ROOT}/Core/HLE/sceAudio.cpp \
		  ${SRC_ROOT}/Core/HLE/sceChnnlsv.cpp \
		  ${SRC_ROOT}/Core/HLE/sceCtrl.cpp \
		  ${SRC_ROOT}/Core/HLE/sceDisplay.cpp \
		  ${SRC_ROOT}/Core/HLE/sceDmac.cpp \
		  ${SRC_ROOT}/Core/HLE/sceGe.cpp \
		  ${SRC_ROOT}/Core/HLE/sceFont.cpp \
		  ${SRC_ROOT}/Core/HLE/sceHprm.cpp \
		  ${SRC_ROOT}/Core/HLE/sceHttp.cpp \
		  ${SRC_ROOT}/Core/HLE/sceImpose.cpp \
		  ${SRC_ROOT}/Core/HLE/sceIo.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernel.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelAlarm.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelEventFlag.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelInterrupt.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelMbx.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelMemory.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelModule.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelMsgPipe.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelMutex.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelSemaphore.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelThread.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelTime.cpp \
		  ${SRC_ROOT}/Core/HLE/sceKernelVTimer.cpp \
		  ${SRC_ROOT}/Core/HLE/sceMpeg.cpp \
		  ${SRC_ROOT}/Core/HLE/sceNet.cpp \
		  ${SRC_ROOT}/Core/HLE/sceOpenPSID.cpp \
		  ${SRC_ROOT}/Core/HLE/sceP3da.cpp \
		  ${SRC_ROOT}/Core/HLE/sceParseHttp.cpp \
		  ${SRC_ROOT}/Core/HLE/sceParseUri.cpp \
		  ${SRC_ROOT}/Core/HLE/scePower.cpp \
		  ${SRC_ROOT}/Core/HLE/scePsmf.cpp \
		  ${SRC_ROOT}/Core/HLE/sceRtc.cpp \
		  ${SRC_ROOT}/Core/HLE/sceSas.cpp \
		  ${SRC_ROOT}/Core/HLE/sceSsl.cpp \
		  ${SRC_ROOT}/Core/HLE/sceUmd.cpp \
		  ${SRC_ROOT}/Core/HLE/sceUsb.cpp \
		  ${SRC_ROOT}/Core/HLE/sceUtility.cpp \
		  ${SRC_ROOT}/Core/HLE/sceVaudio.cpp \
		  ${SRC_ROOT}/Core/HLE/scePspNpDrm_user.cpp \
		  ${SRC_ROOT}/Core/HW/MediaEngine.cpp \
		  ${SRC_ROOT}/Core/HW/MemoryStick.cpp \
		  ${SRC_ROOT}/Core/HW/SasAudio.cpp \
		  ${SRC_ROOT}/Core/Host.cpp \
		  ${SRC_ROOT}/Core/Loaders.cpp \
		  ${SRC_ROOT}/Core/MIPS/JitCommon/JitCommon.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPS.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSAnalyst.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSCodeUtils.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSDebugInterface.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSDis.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSDisVFPU.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSInt.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSIntVFPU.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSTables.cpp \
		  ${SRC_ROOT}/Core/MIPS/MIPSVFPUUtils.cpp \
		  ${SRC_ROOT}/Core/MemMap.cpp \
		  ${SRC_ROOT}/Core/MemMapFunctions.cpp \
		  ${SRC_ROOT}/Core/PSPLoaders.cpp \
		  ${SRC_ROOT}/Core/PSPMixer.cpp \
		  ${SRC_ROOT}/Core/Reporting.cpp \
		  ${SRC_ROOT}/Core/SaveState.cpp \
		  ${SRC_ROOT}/Core/System.cpp \
		  ${SRC_ROOT}/Core/Util/BlockAllocator.cpp \
		  ${SRC_ROOT}/Core/Util/PPGeDraw.cpp \
		  ${SRC_ROOT}/Core/Util/ppge_atlas.cpp \
		  ${SRC_ROOT}/git-version.cpp \
		  ${SRC_ROOT}/GPU/GLES/DisplayListInterpreter.cpp \
		  ${SRC_ROOT}/GPU/GLES/FragmentShaderGenerator.cpp \
		  ${SRC_ROOT}/GPU/GLES/Framebuffer.cpp \
		  ${SRC_ROOT}/GPU/GLES/IndexGenerator.cpp \
		  ${SRC_ROOT}/GPU/GLES/ShaderManager.cpp \
		  ${SRC_ROOT}/GPU/GLES/StateMapping.cpp \
		  ${SRC_ROOT}/GPU/GLES/TextureCache.cpp \
		  ${SRC_ROOT}/GPU/GLES/TransformPipeline.cpp \
		  ${SRC_ROOT}/GPU/GLES/VertexDecoder.cpp \
		  ${SRC_ROOT}/GPU/GLES/VertexShaderGenerator.cpp \
		  ${SRC_ROOT}/GPU/GeDisasm.cpp \
		  ${SRC_ROOT}/GPU/GPUCommon.cpp \
		  ${SRC_ROOT}/GPU/GPUState.cpp \
		  ${SRC_ROOT}/GPU/Math3D.cpp \
		  ${SRC_ROOT}/GPU/Null/NullGpu.cpp \
		  ${SRC_ROOT}/android/jni/NativeApp.cpp \
		  ${SRC_ROOT}/android/jni/EmuScreen.cpp \
		  ${SRC_ROOT}/android/jni/TestRunner.cpp \
		  ${SRC_ROOT}/android/jni/MenuScreens.cpp \
		  ${SRC_ROOT}/android/jni/GamepadEmu.cpp \
		  ${SRC_ROOT}/android/jni/UIShader.cpp  \
		  ${SRC_ROOT}/android/jni/ui_atlas.cpp \
		  webos.cpp

OBJS_C		= $(addsuffix .o, $(basename $(C_SRC)))
OBJS_CXX	= $(addsuffix .o, $(basename $(CPP_SRC)))

%.o:%.cpp 
	$(CXX) $(CXXFLAGS) $(HEADERS) $< -o $(addsuffix .o, $(basename $<)) -c 
%.o:%.c
	$(CC) $(CFLAGS) $(HEADERS) $< -o $(addsuffix .o, $(basename $<)) -c 


all: $(OBJS_C) $(OBJS_CXX)
	$(CXX) $(OBJS_C) $(OBJS_CXX) -o $(APPNAME) -lz -lm -lrt -lpdl -lGLESv2 -lSDL -lpthread -static-libstdc++ -static-libgcc -Wl,-static -Wl,--allow-multiple-definition -Wl,--allow-shlib-undefined 

clean:
	rm -r $(OBJS_C) $(OBJS_CXX) 2>/dev/null || true


