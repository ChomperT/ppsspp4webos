#include <pwd.h>
#include <unistd.h>

#include <string>
#include <SDL/SDL.h>
#include "base/display.h"
#include "base/logging.h"
#include "base/timeutil.h"
#include "gfx_es2/gl_state.h"
#include "gfx_es2/glsl_program.h"
#include "file/zip_read.h"
#include "input/input_state.h"
#include "base/NativeApp.h"
#include "net/resolve.h"
#include <PDL.h>


// Simple implementations of System functions


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
	// Stub
	NativeMessageReceived((std::string("INPUTBOX:") + title).c_str(), "TestFile");
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



const int buttonMappings[14] = {
	SDLK_z,         //A
	SDLK_x,         //B
	SDLK_a,         //X
	SDLK_s,	        //Y
	SDLK_w,         //LBUMPER
	SDLK_q,         //RBUMPER
	SDLK_SPACE,     //START
	SDLK_v,	        //SELECT
	SDLK_UP,        //UP
	SDLK_DOWN,      //DOWN
	SDLK_LEFT,      //LEFT
	SDLK_RIGHT,     //RIGHT
	SDLK_m,         //MENU
	SDLK_ESCAPE,	//BACK
};

void SimulateGamepad(const uint8 *keys, InputState *input) {
	input->pad_buttons = 0;
	input->pad_lstick_x = 0;
	input->pad_lstick_y = 0;
	input->pad_rstick_x = 0;
	input->pad_rstick_y = 0;
	for (int b = 0; b < 14; b++) {
		if (keys[buttonMappings[b]])
			input->pad_buttons |= (1<<b);
	}

	if (keys[SDLK_i])
		input->pad_lstick_y=1;
	else if (keys[SDLK_k])
		input->pad_lstick_y=-1;
	if (keys[SDLK_j])
		input->pad_lstick_x=-1;
	else if (keys[SDLK_l])
		input->pad_lstick_x=1;
	if (keys[SDLK_KP8])
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

	PDL_Init(0);
	if (zoomenv) {
		zoom = atof(zoomenv);
	}
	
	bool landscape;
	NativeGetAppInfo(&app_name, &app_name_nice, &landscape);
	
	// Change these to temporarily test other resolutions.
	aspect43 = false;
	tablet = false;
	float density = 1.0f;
	//zoom = 1.5f;


	PDL_ScreenMetrics out;
	PDL_SetOrientation(PDL_ORIENTATION_90);
	PDL_GetScreenMetrics(&out);
	int x, y;
	x = out.horizontalPixels;
	y = out.verticalPixels;
	pixel_xres = x > y ? x : y;
	pixel_yres = x > y ? y : x;

	net::Init();

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
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
		SDL_OPENGLES
		) == NULL) {
		fprintf(stderr, "SDL SetVideoMode failed: Unable to create OpenGL screen: %s\n", SDL_GetError());
		SDL_Quit();
		return(2);
	}

	// Mac / Linux
	char path[512];
	const char *the_path = "/media/internal/psp";

	mkdir(the_path, 0744);

	strcpy(path, the_path);
	if (path[strlen(path)-1] != '/')
		strcat(path, "/");

	NativeInit(argc, (const char **)argv, path, "/tmp", "BADCOFFEE");

	dp_xres = (float)pixel_xres * density / zoom;
	dp_yres = (float)pixel_yres * density / zoom;
	pixel_in_dps = (float)pixel_xres / dp_xres;

	NativeInitGraphics();
	glstate.viewport.set(0, 0, pixel_xres, pixel_yres);
	glstate.cullFaceMode.set(GL_FRONT_AND_BACK);

	float dp_xscale = (float)dp_xres / pixel_xres;
	float dp_yscale = (float)dp_yres / pixel_yres;


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

	InputState input_state;
	int framecount = 0;
	bool nextFrameMD = 0;
	float t = 0, lastT = 0;
	while (true) {
		input_state.accelerometer_valid = false;
		input_state.mouse_valid = true;
		int quitRequested = 0;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			float mx = event.motion.x * dp_xscale;
			float my = event.motion.y * dp_yscale;
			fflush(stdout);

			if (event.type == SDL_QUIT) {
				quitRequested = 1;
			} else if (event.type == SDL_KEYDOWN) {
/*				if (event.key.keysym.sym == SDLK_ESCAPE) {
					quitRequested = 1;
				}*/
			} else if (event.type == SDL_MOUSEMOTION) {
				input_state.pointer_x[0] = mx;
				input_state.pointer_y[0] = my;
				NativeTouch(0, mx, my, 0, TOUCH_MOVE);
			} else if (event.type == SDL_MOUSEBUTTONDOWN) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					//input_state.mouse_buttons_down = 1;
					input_state.pointer_down[0] = true;
					nextFrameMD = true;
					NativeTouch(0, mx, my, 0, TOUCH_DOWN);
				}
			} else if (event.type == SDL_MOUSEBUTTONUP) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					input_state.pointer_down[0] = false;
					nextFrameMD = false;
					//input_state.mouse_buttons_up = 1;
					NativeTouch(0, mx, my, 0, TOUCH_UP);
				}
			}
		}

		if (quitRequested)
			break;

		const uint8 *keys = (const uint8 *)SDL_GetKeyState(NULL);

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
	net::Shutdown();
	exit(0);
	return 0;
}
