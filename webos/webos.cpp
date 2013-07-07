// SDL/EGL implementation of the framework.
// This is quite messy due to platform-specific implementations and #ifdef's.
// It is suggested to use the Qt implementation instead.

#include <pwd.h>
#include <unistd.h>

#include <SDL/SDL.h>
#include <SDL/SDL_timer.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL_video.h>
#include <PDL.h>

#include "base/display.h"
#include "base/logging.h"
#include "base/timeutil.h"
#include "gfx_es2/gl_state.h"
#include "gfx_es2/glsl_program.h"
#include "file/zip_read.h"
#include "input/input_state.h"
#include "input/keycodes.h"
#include "base/KeyCodeTranslationFromSDL.h"
#include "base/NativeApp.h"
#include "net/resolve.h"
#include "util/const_map.h"


const char *PPSSPP_GIT_VERSION="UNKnown";

void SystemToast(const char *text) {
	puts(text);
}

void ShowAd(int x, int y, bool center_x) {
	// Ignore ads on PC
}

void ShowKeyboard() {
	// Irrelevant on PC
}

void Vibrate(int length_ms) {
	// Ignore on PC
}

void System_InputBox(const char *title, const char *defaultValue) {
}

void LaunchBrowser(const char *url)
{
	ILOG("Would have gone to %s but LaunchBrowser is not implemented on this platform", url);
}

void LaunchMarket(const char *url)
{
	ILOG("Would have gone to %s but LaunchMarket is not implemented on this platform", url);
}

void LaunchEmail(const char *email_address)
{
	ILOG("Would have opened your email client for %s but LaunchEmail is not implemented on this platform", email_address);
}



InputState input_state;


const int buttonMappings[14] = {
	SDLK_z,         //A
	SDLK_x,         //B
	SDLK_a,         //X
	SDLK_s,	        //Y
	SDLK_q,         //LBUMPER
	SDLK_w,         //RBUMPER
	SDLK_SPACE,     //START
	SDLK_v,	        //SELECT
	SDLK_UP,        //UP
	SDLK_DOWN,      //DOWN
	SDLK_LEFT,      //LEFT
	SDLK_RIGHT,     //RIGHT
	SDLK_m,         //MENU
	SDLK_BACKSPACE,	//BACK
};

void SimulateGamepad(const uint8 *keys, InputState *input) {
	input->pad_buttons = 0;
	input->pad_lstick_x = 0;
	input->pad_lstick_y = 0;
	input->pad_rstick_x = 0;
	input->pad_rstick_y = 0;

	if (keys[SDLK_i])
		input->pad_lstick_y=1;
	else if (keys[SDLK_k])
		input->pad_lstick_y=-1;
	if (keys[SDLK_j])
		input->pad_lstick_x=-1;
	else if (keys[SDLK_l])
		input->pad_lstick_x=1; if (keys[SDLK_KP8])
		input->pad_rstick_y=1;
	else if (keys[SDLK_KP2])
		input->pad_rstick_y=-1;
	if (keys[SDLK_KP4])
		input->pad_rstick_x=-1;
	else if (keys[SDLK_KP6])
		input->pad_rstick_x=1;
}

extern void mixaudio(void *userdata, Uint8 *stream, int len) {
	NativeMix((short *)stream, len / 4);
}

int main(int argc, char *argv[]) {
	std::string app_name;
	std::string app_name_nice;

	float zoom = 1.0f;
	bool tablet = false;
	bool aspect43 = false;
	const char *zoomenv = getenv("ZOOM");
	const char *tabletenv = getenv("TABLET");
	const char *ipad = getenv("IPAD");

	if (zoomenv) {
		zoom = atof(zoomenv);
	}
	if (tabletenv) {
		tablet = atoi(tabletenv) ? true : false;
	}
	if (ipad) aspect43 = true;

	bool landscape;
	NativeGetAppInfo(&app_name, &app_name_nice, &landscape);

	// Change these to temporarily test other resolutions.
	aspect43 = false;
	tablet = false;
	float density = 1.0f;
	//zoom = 1.5f;

	if (landscape) {
		if (tablet) {
			pixel_xres = 1280 * zoom;
			pixel_yres = 800 * zoom;
		} else if (aspect43) {
			pixel_xres = 1024 * zoom;
			pixel_yres = 768 * zoom;
		} else {
			pixel_xres = 960 * zoom;
			pixel_yres = 544 * zoom;
		}
	} else {
		// PC development hack for more space
		//pixel_xres = 1580 * zoom;
		//pixel_yres = 1000 * zoom;
		if (tablet) {
			pixel_xres = 800 * zoom;
			pixel_yres = 1280 * zoom;
		} else if (aspect43) {
			pixel_xres = 768 * zoom;
			pixel_yres = 1024 * zoom;
		} else {
			pixel_xres = 480 * zoom;
			pixel_yres = 800 * zoom;
		}
	}


	PDL_Init(0);

	net::Init();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

	if (SDL_SetVideoMode(pixel_xres, pixel_yres, 0, 
				SDL_OPENGL
			    ) == NULL) {
		fprintf(stderr, "SDL SetVideoMode failed: Unable to create OpenGL screen: %s\n", SDL_GetError());
		SDL_Quit();
		return(2);
	}

	SDL_WM_SetCaption(app_name_nice.c_str(), NULL);

	char path[512];
	const char *the_path = getenv("HOME");
	if (!the_path) {
		struct passwd* pwd = getpwuid(getuid());
		if (pwd)
			the_path = pwd->pw_dir;
	}
	strcpy(path, the_path);
	if (path[strlen(path)-1] != '/')
		strcat(path, "/");

	NativeInit(argc, (const char **)argv, path, "/tmp", "BADCOFFEE");

	dp_xres = (float)pixel_xres * density / zoom;
	dp_yres = (float)pixel_yres * density / zoom;
	pixel_in_dps = (float)pixel_xres / dp_xres;

	NativeInitGraphics();
	glstate.viewport.set(0, 0, pixel_xres, pixel_yres);

	float dp_xscale = (float)dp_xres / pixel_xres;
	float dp_yscale = (float)dp_yres / pixel_yres;

	g_dpi_scale = pixel_xres / dp_xres;


	printf("Pixels: %i x %i\n", pixel_xres, pixel_yres);
	printf("Virtual pixels: %i x %i\n", dp_xres, dp_yres);

	SDL_AudioSpec fmt;
	fmt.freq = 44100;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 1024;
	fmt.callback = &mixaudio;
	fmt.userdata = (void *)0;

	if (SDL_OpenAudio(&fmt, NULL) < 0) {
		ELOG("Failed to open audio: %s", SDL_GetError());
		return 1;
	}

	// Audio must be unpaused _after_ NativeInit()
	SDL_PauseAudio(0);

	int framecount = 0, k;
	float t = 0, lastT = 0;
	int quitRequested = 0;
	float mx, my;
	KeyInput key;
	SDL_Event event;
	while (true) {
		input_state.accelerometer_valid = false;
		input_state.mouse_valid = true;
		while (SDL_PollEvent(&event)) {
			mx = event.motion.x * dp_xscale;
			my = event.motion.y * dp_yscale;
			k = event.key.keysym.sym;

			switch(event.type) {
				case SDL_QUIT:
					quitRequested = 1;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						quitRequested = 1;
					}
					key.flags = KEY_DOWN;
					key.keyCode = KeyMapRawSDLtoNative.find(k)->second;
					key.deviceId = DEVICE_ID_KEYBOARD;
					NativeKey(key);
					break;
				case SDL_KEYUP:
					key.flags = KEY_UP;
					key.keyCode = KeyMapRawSDLtoNative.find(k)->second;
					key.deviceId = DEVICE_ID_KEYBOARD;
					NativeKey(key);
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						input_state.pointer_x[0] = mx;
						input_state.pointer_y[0] = my;
						input_state.pointer_down[0] = true;
						TouchInput input;
						input.x = mx;
						input.y = my;
						input.flags = TOUCH_DOWN;
						input.id = 0;
						NativeTouch(input);
					}
					break;
				case SDL_MOUSEMOTION:
					if (input_state.pointer_down[0]) {
						input_state.pointer_x[0] = mx;
						input_state.pointer_y[0] = my;
						TouchInput input;
						input.x = mx;
						input.y = my;
						input.flags = TOUCH_MOVE;
						input.id = 0;
						NativeTouch(input);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (event.button.button == SDL_BUTTON_LEFT) {
						input_state.pointer_x[0] = mx;
						input_state.pointer_y[0] = my;
						input_state.pointer_down[0] = false;
						TouchInput input;
						input.x = mx;
						input.y = my;
						input.flags = TOUCH_UP;
						input.id = 0;
						NativeTouch(input);
					}
					break;
				case SDL_ACTIVEEVENT:
					if(event.active.state == SDL_APPACTIVE)
					{
						key.flags = event.active.gain ? KEY_DOWN : KEY_UP;
						key.keyCode = KeyMapRawSDLtoNative.find(SDLK_m)->second;
						key.deviceId = DEVICE_ID_KEYBOARD;
						NativeKey(key);

					}
					break;
				default:
					break;
			}
		}

			if (quitRequested)
				break;

			const uint8 *keys = (const uint8 *)SDL_GetKeyState(NULL);
			if (keys[SDLK_ESCAPE])
				break;
			SimulateGamepad(keys, &input_state);
			UpdateInputState(&input_state);
			NativeUpdate(input_state);
			NativeRender();

			EndInputState(&input_state);

			if (framecount % 60 == 0) {
			}

			if (!keys[SDLK_TAB] || t - lastT >= 1.0/60.0)
			{
				SDL_GL_SwapBuffers();
				lastT = t;
			}

			time_update();
			t = time_now();
			framecount++;
		}
		NativeShutdownGraphics();
		SDL_PauseAudio(1);
		SDL_CloseAudio();
		NativeShutdown();
		SDL_Quit();
		PDL_Quit();
		net::Shutdown();
		exit(0);
		return 0;
	}
