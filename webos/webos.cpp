#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h> //for mkdir

#include <string>
#ifdef WEBOS
#include <SDL/SDL.h>
#include <PDL.h>
#include "../Core/Config.h" //check landscape for ui screen scale;
#else
#include "SDL.h"
#endif

#include "base/display.h"
#include "base/logging.h"
#include "base/timeutil.h"
#include "gfx_es2/gl_state.h"
#include "gfx_es2/glsl_program.h"
#include "file/zip_read.h"
#include "input/input_state.h"
#include "base/NativeApp.h"
#include "net/resolve.h"



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
#ifdef WEBOS
bool menu = false; //sorry about this.....
#endif
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
#ifdef WEBOS
	if (menu)
	{
		input->pad_buttons |= (1 << 12);
		menu = false;
	}
#endif
}

extern void mixaudio(void *userdata, Uint8 *stream, int len) {
	NativeMix((short *)stream, len / 4);
}

int main(int argc, char *argv[]) {
	std::string app_name;
	std::string app_name_nice;
	bool landscape;

	PDL_Init(0);
	PDL_GesturesEnable(SDL_TRUE);
	PDL_BannerMessagesEnable(SDL_FALSE);

	NativeGetAppInfo(&app_name, &app_name_nice, &landscape);
	
	// Change these to temporarily test other resolutions.


	PDL_ScreenMetrics out;
	PDL_SetOrientation(PDL_ORIENTATION_90);
	PDL_GetScreenMetrics(&out);
	pixel_xres = out.horizontalPixels;;
	pixel_yres = out.verticalPixels;

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

	setenv("HOME", "/media/internal", true);
	strcpy(path, the_path);
	if (path[strlen(path)-1] != '/')
		strcat(path, "/");

	NativeInit(argc, (const char **)argv, path, "/tmp", "BADCOFFEE");

	dp_xres = (float)pixel_xres;
	dp_yres = (float)pixel_yres;
	pixel_in_dps = (float)pixel_xres / dp_xres;

	NativeInitGraphics();
	glstate.viewport.set(0, 0, pixel_xres, pixel_yres);
	glstate.cullFaceMode.set(GL_FRONT_AND_BACK);

	printf("Pixels: %i x %i, v: %d, %d\n", pixel_xres, pixel_yres, dp_xres, dp_yres);

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
	int x, y;
	float mx, my;
	x = dp_xres;
	y = dp_yres;

	while (true) {
		input_state.accelerometer_valid = false;
		input_state.mouse_valid = true;
		int quitRequested = 0;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {

#ifdef WEBOS
			if(g_Config.bLandScape) {
				dp_xres = y;
				dp_yres = x;
				mx = event.motion.y;
				my = x - event.motion.x;
			} else {
				dp_xres = x;
				dp_yres = y;
				mx = event.motion.x;
				my = event.motion.y;
			}
#else
			mx = event.motion.x;
			my = event.motion.y;
#endif
			switch(event.type) {
			case SDL_QUIT:
				quitRequested = 1;
				break;
			case SDL_MOUSEMOTION:
				input_state.pointer_x[0] = mx;
				input_state.pointer_y[0] = my;
				NativeTouch(0, mx, my, 0, TOUCH_MOVE);
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					input_state.pointer_down[0] = true;
					nextFrameMD = true;
					NativeTouch(0, mx, my, 0, TOUCH_DOWN);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (event.button.button == SDL_BUTTON_LEFT) {
					input_state.pointer_down[0] = false;
					nextFrameMD = false;
					NativeTouch(0, mx, my, 0, TOUCH_UP);
				}
				break;
			case SDL_ACTIVEEVENT:
				if(event.active.state == SDL_APPACTIVE)
					if(!event.active.gain)
						menu = true;
				break;
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
