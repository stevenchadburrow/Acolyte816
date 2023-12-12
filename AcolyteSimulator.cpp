// Acolyte Simulator
// uses updates for '816

// Uses OpenGL GLFW

// To download required libraries, use:

// sudo apt-get install g++ libglfw3-dev libglu1-mesa-dev

// (and maybe others?  Mesa packages for Linux?  Mingw perhaps?)

// Use this to compile:

// g++ -o AcolyteSimulator.o AcolyteSimulator.cpp -lglfw -lGL -lGLU

// Then to execute:

// ./AcolyteSimulator.o <binary_file>

// The <binary_file> is whatever .bin file would be on the ROM chip.

// For Windows, use Code::Blocks and download the GLFW Pre-Compiled Libraries, and just toss those into the folders needed ("include" and "lib").

// Do not use the GLFW project, but use a blank project, and use the linker to put in all kinds of things.

// Also remember you need <windows.h> for any Windows program!



#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "mos6502-Edit816.cpp" // this is actually Agumander's version with my edits

uint8_t bootloader_code[512] = { // this is 512 bytes for 0x00FE00 to 0x00FFFF
0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
0x80,0x01,0x40,0xA9,0x80,0x8F,0x02,0x00,0x01,0xA9,0x02,0x8F,0x0C,0x00,0x01,0x8F,
0x0E,0x00,0x01,0x58,0x20,0x85,0xFE,0xAE,0xE1,0xFF,0xEC,0xE0,0xFF,0xF0,0xF8,0xEE,
0xE1,0xFF,0xBD,0x00,0x02,0xC9,0xF0,0xD0,0x05,0xEE,0xE1,0xFF,0x80,0xE9,0xC9,0x29,
0xD0,0x05,0xEE,0xB1,0xFE,0x80,0xDD,0xC9,0x5A,0xD0,0x0B,0xAD,0xB1,0xFE,0x18,0x69,
0x20,0x8D,0xB1,0xFE,0x80,0xCE,0xC9,0x66,0xD0,0x05,0xCE,0xB1,0xFE,0x80,0xC5,0xC9,
0x76,0xF0,0x1D,0xA2,0x0F,0xDD,0x5B,0xFF,0xF0,0x03,0xCA,0xD0,0xF8,0xDA,0x20,0xB0,
0xFE,0xB5,0x00,0x0A,0x0A,0x0A,0x0A,0x95,0x00,0x68,0x15,0x00,0x95,0x00,0x80,0xA4,
0x20,0x00,0x00,0x80,0x9F,0xA0,0x02,0xA2,0x00,0xA5,0x00,0xDA,0x9C,0x13,0xFF,0x20,
0xB0,0xFE,0xEC,0x8A,0xFE,0xD0,0x03,0xCE,0x13,0xFF,0xFA,0x20,0xB3,0xFE,0xEE,0x8A,
0xFE,0x8A,0x29,0x3F,0xD0,0xE3,0xA2,0x00,0xC8,0xC8,0xAD,0x8A,0xFE,0xD0,0xDA,0x60,
0xA2,0x00,0x60,0x48,0x48,0x29,0xF0,0x18,0x6A,0x6A,0x6A,0x6A,0x20,0xC9,0xFE,0xE8,
0x68,0x29,0x0F,0x20,0xC9,0xFE,0xE8,0x68,0x60,0x48,0xDA,0x5A,0x48,0x8A,0x0A,0x8D,
0x17,0xFF,0x98,0x0A,0x0A,0x0A,0x8D,0x18,0xFF,0x68,0x0A,0x0A,0xAA,0xA0,0x04,0xBD,
0x1B,0xFF,0x20,0xF6,0xFE,0x20,0x01,0xFF,0x20,0xF6,0xFE,0x20,0x01,0xFF,0xE8,0x88,
0xD0,0xED,0x7A,0xFA,0x68,0x60,0x48,0x20,0x12,0xFF,0xEE,0x17,0xFF,0x68,0x0A,0x0A,

0x60,0x48,0x20,0x12,0xFF,0xCE,0x17,0xFF,0xEE,0x18,0xFF,0xEE,0x18,0xFF,0x68,0x0A,
0x0A,0x60,0x49,0x00,0x29,0xC0,0x8F,0x00,0x00,0x02,0x60,0xEA,0xAA,0xE0,0x00,0x22,
0x22,0x20,0x00,0xE2,0xE8,0xE0,0x00,0xE2,0xE2,0xE0,0x00,0xAA,0xE2,0x20,0x00,0xE8,
0xE2,0xE0,0x00,0xE8,0xEA,0xE0,0x00,0xE2,0x22,0x20,0x00,0xEA,0xEA,0xE0,0x00,0xEA,
0xE2,0xE0,0x00,0xEA,0xEA,0xA0,0x00,0x88,0xEA,0xE0,0x00,0xE8,0x88,0xE0,0x00,0x22,
0xEA,0xE0,0x00,0xE8,0xE8,0xE0,0x00,0xE8,0xE8,0x80,0x00,0x45,0x16,0x1E,0x26,0x25,
0x2E,0x36,0x3D,0x3E,0x46,0x1C,0x32,0x21,0x23,0x24,0x2B,0x48,0xA9,0xFF,0x8F,0x0D,
0x00,0x01,0xAF,0x00,0x00,0x01,0x29,0x20,0x0A,0x0A,0x18,0x6E,0xE2,0xFF,0x18,0x6D,
0xE2,0xFF,0x8D,0xE2,0xFF,0xEE,0xE4,0xFF,0xAD,0xE4,0xFF,0xC9,0x09,0xD0,0x12,0xAD,
0xE2,0xFF,0xDA,0xAE,0xE0,0xFF,0x9D,0x00,0x02,0x8A,0x1A,0x8D,0xE0,0xFF,0xFA,0x68,
0x40,0xC9,0x0B,0xF0,0x02,0x68,0x40,0x9C,0xE4,0xFF,0x68,0x40,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x12,0xFE,0x12,0xFE,0x12,0xFE,0x12,0xFE,0x00,0x00,0x12,0xFE,
0x00,0x00,0x00,0x00,0x12,0xFE,0x00,0x00,0x12,0xFE,0x12,0xFE,0x00,0xFE,0x6B,0xFF,
};

bool verbose = false; // change to display reach read/write action

int open_window_x = 640;
int open_window_y = 480;

int open_cursor_new_x = 0;
int open_cursor_new_y = 0;
int open_cursor_old_x = 0;
int open_cursor_old_y = 0;

int open_keyboard_state[512];
int open_button_state[16];

int open_frames_per_second = 0;
int open_frames_per_second_counter = 0;
int open_frames_per_second_timer = 0;

int open_frames_limit = 120; // this is frames per second, cannot be more than 60 though! (yet, it works best this way)
double open_frames_difference = (1.0f / (double)open_frames_limit) * (double)CLOCKS_PER_SEC;
double open_frames_counter = 0.0f;
double open_frames_previous = 0.0f;
double open_frames_current = 0.0f;
bool open_frames_drawing = true;
int open_frames_drawing_counter = 0;
float open_frames_delta = 0.0f;

clock_t open_clock_previous, open_clock_current;

uint8_t MEMORY[16777216]; // 16MB of RAM
uint32_t KEY_ARRAY = 0x00000200; // need the location for keyboard buffer
uint32_t KEY_POS_WRITE = 0x0000FFE0; // need the location for keyboard buffer

mos6502 *CPU;

void WriteFunction(uint32_t address, uint8_t value)
{
	if (verbose) printf("Write %06x %02x\n", address, value);

	MEMORY[address] = value;
};

uint8_t ReadFunction(uint32_t address)
{
	if (verbose) printf("Read %06x %02x\n", address, MEMORY[address]);	

	return MEMORY[address];
};


uint8_t PS2KeyCode(int key)
{
	switch (key)
	{
		case GLFW_KEY_UNKNOWN:
		{
			return 0x00;
		}
 
		case GLFW_KEY_SPACE:
		{
			return 0x29;
		}
 
		case GLFW_KEY_APOSTROPHE:
		{
			return 0x52;
		}
 
		case GLFW_KEY_COMMA:
		{
			return 0x41;
		}
 
		case GLFW_KEY_MINUS:
		{
			return 0x4E;
		}
 
		case GLFW_KEY_PERIOD:
		{
			return 0x49;
		}
 
		case GLFW_KEY_SLASH:
		{
			return 0x4A;
		}

		case GLFW_KEY_0:
		{
			return 0x45;
		}
 
		case GLFW_KEY_1:
		{
			return 0x16;
		}
 
		case GLFW_KEY_2:
		{
			return 0x1E;
		}
 
		case GLFW_KEY_3:
		{
			return 0x26;
		}
 
		case GLFW_KEY_4:
		{
			return 0x25;
		}
 
		case GLFW_KEY_5:
		{
			return 0x2E;
		}
 
		case GLFW_KEY_6:
		{
			return 0x36;
		}
 
		case GLFW_KEY_7:
		{
			return 0x3D;
		}
 
		case GLFW_KEY_8:
		{
			return 0x3E;
		}
 
		case GLFW_KEY_9:
		{
			return 0x46;
		}
 
		case GLFW_KEY_SEMICOLON:
		{
			return 0x4C;
		}
 
		case GLFW_KEY_EQUAL:
		{
			return 0x55;
		}
 
		case GLFW_KEY_A:
		{
			return 0x1C;
		}
 
		case GLFW_KEY_B:
		{
			return 0x32;
		}

		case GLFW_KEY_C:
		{
			return 0x21;
		}

		case GLFW_KEY_D:
		{
			return 0x23;
		}

		case GLFW_KEY_E:
		{
			return 0x24;
		}

		case GLFW_KEY_F:
		{
			return 0x2B;
		}

		case GLFW_KEY_G:
		{
			return 0x34;
		}
	
		case GLFW_KEY_H:
		{
			return 0x33;
		}

		case GLFW_KEY_I:
		{
			return 0x43;
		}

		case GLFW_KEY_J:
		{
			return 0x3B;
		}

		case GLFW_KEY_K:
		{
			return 0x42;
		}

		case GLFW_KEY_L:
		{
			return 0x4B;
		}

		case GLFW_KEY_M:
		{
			return 0x3A;
		}

		case GLFW_KEY_N:
		{
			return 0x31;
		}

		case GLFW_KEY_O:
		{
			return 0x44;
		}

		case GLFW_KEY_P:
		{
			return 0x4D;
		}

		case GLFW_KEY_Q:
		{
			return 0x15;
		}

		case GLFW_KEY_R:
		{
			return 0x2D;
		}

		case GLFW_KEY_S:
		{
			return 0x1B;
		}

		case GLFW_KEY_T:
		{
			return 0x2C;
		}

		case GLFW_KEY_U:
		{
			return 0x3C;
		}
		
		case GLFW_KEY_V:
		{
			return 0x2A;
		}

		case GLFW_KEY_W:
		{
			return 0x1D;
		}

		case GLFW_KEY_X:
		{
			return 0x22;	
		}
		
		case GLFW_KEY_Y:
		{
			return 0x35;
		}

		case GLFW_KEY_Z:
		{
			return 0x1A;
		}

		case GLFW_KEY_LEFT_BRACKET:
		{
			return 0x54;
		}
 
		case GLFW_KEY_BACKSLASH:
		{
			return 0x5D;
		}
 
		case GLFW_KEY_RIGHT_BRACKET:
		{
			return 0x5B;
		}
 
		case GLFW_KEY_GRAVE_ACCENT:
		{
			return 0x0E;
		}
 
		case GLFW_KEY_WORLD_1:
		{
			return 0x00;
		}
 
		case GLFW_KEY_WORLD_2:
		{
			return 0x00;
		}
 
		case GLFW_KEY_ESCAPE:
		{
			return 0x76;
		}
 
		case GLFW_KEY_ENTER:
		{
			return 0x5A;
		}

		case GLFW_KEY_TAB:
		{
			return 0x0D;
		}
 
		case GLFW_KEY_BACKSPACE:
		{
			return 0x66;
		}
 
		case GLFW_KEY_CAPS_LOCK:
		{
			return 0x58;
		}
 
		case GLFW_KEY_SCROLL_LOCK:
		{
			return 0x7E;
		}
 
		case GLFW_KEY_NUM_LOCK:
		{
			return 0x77;
		}
 
		case GLFW_KEY_F1:
		{
			return 0x05;
		}

		case GLFW_KEY_F2:
		{
			return 0x06;
		}

		case GLFW_KEY_F3:
		{
			return 0x04;
		}

		case GLFW_KEY_F4:
		{
			return 0x0C;
		}

		case GLFW_KEY_F5:
		{
			return 0x03;
		}

		case GLFW_KEY_F6:
		{
			return 0x0B;
		}

		case GLFW_KEY_F7:
		{
			return 0x83;
		}

		case GLFW_KEY_F8:
		{
			return 0x0A;
		}

		case GLFW_KEY_F9:
		{
			return 0x01;
		}

		case GLFW_KEY_F10:
		{
			return 0x09;
		}

		case GLFW_KEY_F11:
		{
			return 0x78;
		}

		case GLFW_KEY_F12:
		{
			return 0x07;
		}
 
		case GLFW_KEY_KP_0:
		{
			return 0x70;
		}
 
		case GLFW_KEY_KP_1:
		{
			return 0x69;
		}
 
		case GLFW_KEY_KP_2:
		{
			return 0x72;
		}
 
		case GLFW_KEY_KP_3:
		{
			return 0x7A;
		}
 
		case GLFW_KEY_KP_4:
		{
			return 0x6B;
		}
 
		case GLFW_KEY_KP_5:
		{
			return 0x73;
		}
 
		case GLFW_KEY_KP_6:
		{
			return 0x74;
		}
 
		case GLFW_KEY_KP_7:
		{
			return 0x6C;
		}
 
		case GLFW_KEY_KP_8:
		{
			return 0x75;
		}
 
		case GLFW_KEY_KP_9:
		{
			return 0x7D;
		}
 
		case GLFW_KEY_KP_DECIMAL:
		{
			return 0x71;
		}

		case GLFW_KEY_KP_MULTIPLY:
		{
			return 0x7C;
		}
 
		case GLFW_KEY_KP_SUBTRACT:
		{
			return 0x7B;
		}
 
		case GLFW_KEY_KP_ADD:
		{
			return 0x79;
		}
 
		case GLFW_KEY_LEFT_SHIFT:
		{
			return 0x12;
		}

		case GLFW_KEY_LEFT_CONTROL:
		{
			return 0x14;
		}
 
		case GLFW_KEY_LEFT_ALT:
		{
			return 0x11;
		}
 
		case GLFW_KEY_RIGHT_SHIFT:
		{
			return 0x59;
		}

		case GLFW_KEY_INSERT:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_DELETE:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_RIGHT:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_LEFT:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_DOWN:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_UP:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_PAGE_UP:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_PAGE_DOWN:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_HOME:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_END:
		{
			return 0xE0;
		}

		case GLFW_KEY_KP_DIVIDE:
		{
			return 0xE0;
		}

		case GLFW_KEY_KP_ENTER:
		{
			return 0xE0;
		}

		case GLFW_KEY_RIGHT_CONTROL:
		{
			return 0xE0;
		}
 
		case GLFW_KEY_RIGHT_ALT:
		{
			return 0xE0;
		}

		case GLFW_KEY_PRINT_SCREEN:
		{
			return 0x00;
		}
 
		case GLFW_KEY_PAUSE:
		{
			return 0x00;
		}

		default:
		{
			return 0x00;
		}
	}

	return 0x00;
};

uint8_t PS2ExtendedKeyCode(int key)
{
	switch (key)
	{
		case GLFW_KEY_INSERT:
		{
			return 0x70;
		}
 
		case GLFW_KEY_DELETE:
		{
			return 0x71;
		}
 
		case GLFW_KEY_RIGHT:
		{
			return 0x74;
		}
 
		case GLFW_KEY_LEFT:
		{
			return 0x6B;
		}
 
		case GLFW_KEY_DOWN:
		{
			return 0x72;
		}
 
		case GLFW_KEY_UP:
		{
			return 0x75;
		}
 
		case GLFW_KEY_PAGE_UP:
		{
			return 0x7D;
		}
 
		case GLFW_KEY_PAGE_DOWN:
		{
			return 0x7A;
		}
 
		case GLFW_KEY_HOME:
		{
			return 0x6C;
		}
 
		case GLFW_KEY_END:
		{
			return 0x69;
		}

		case GLFW_KEY_KP_DIVIDE:
		{
			return 0x4A;
		}

		case GLFW_KEY_KP_ENTER:
		{
			return 0x5A;
		}

		case GLFW_KEY_RIGHT_CONTROL:
		{
			return 0x14;
		}
 
		case GLFW_KEY_RIGHT_ALT:
		{
			return 0x11;
		}

		default:
		{
			return 0x00;
		}
	}

	return 0x00;
};


void InitializeOpenGLSettings()
{
	// set up the init settings
	glViewport(0, 0, open_window_x, open_window_y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// glOrtho() uses the orthographic projection.
	// Basically it makes the shapes not distort as you go near the edges of the screen.
	// This is very unnatural, but is useful when your 3D objects should actually be displayed as 2D objects.
	glOrtho(0, open_window_x, 0, open_window_y, -1000.0f, 1000.0f);
	
	// gluPerspective() uses the perspective projection.
	// Basically this makes the shapes distort as they go near the edges of the screen.
	// This is most natural, especially for 3D environments.
	//gluPerspective(45.0f, (GLfloat)open_window_x / (GLfloat)open_window_y, 0.05f, 500.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); 
	//glEnable(GL_TEXTURE_2D);  
	//glShadeModel(GL_SMOOTH);
	glClearColor(0.1f, 0.1f, 0.1f, 0.5f);
	glClearDepth(1.0f);
	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);					
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	
	glAlphaFunc(GL_GREATER, 0.9f);									
	glEnable(GL_ALPHA_TEST);

	//glDisable(GL_ALPHA_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// more stuff for lighting                                  
	glFrontFace(GL_CCW);
	glEnable(GL_NORMALIZE);     
	glCullFace(GL_FRONT);                   

	// allows colors to be still lit up      
	glEnable(GL_COLOR_MATERIAL);

	return;
};

void handleKeys(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		open_keyboard_state[key] = 1;

		MEMORY[KEY_ARRAY+MEMORY[KEY_POS_WRITE]] = (uint8_t)PS2KeyCode(key); 

		MEMORY[KEY_POS_WRITE]++;
	
		//if (RAM[KEY_POS_WRITE] >= 0x80) RAM[KEY_POS_WRITE] = 0x00;

		if (PS2KeyCode(key) == 0xE0) // extended
		{
			MEMORY[KEY_ARRAY+MEMORY[KEY_POS_WRITE]] = (uint8_t)PS2ExtendedKeyCode(key);
		
			MEMORY[KEY_POS_WRITE]++;
	
			//if (RAM[KEY_POS_WRITE] >= 0x80) RAM[KEY_POS_WRITE] = 0x00;
		}	

		if (key == GLFW_KEY_PAUSE)
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		
/*
		switch (key)
		{
			case GLFW_KEY_ESCAPE:
			{
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			
				break;
			}
	
			case GLFW_KEY_F1:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

				glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);

				open_window_x = mode->width;
				open_window_y = mode->height;

				break;
			}

			case GLFW_KEY_F2:
			{
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

				glfwSetWindowMonitor(window, NULL, 0, 0, 640, 480, mode->refreshRate);

				open_window_x = 640;
				open_window_y = 480;

				break;
			}

			default: {}
		}
*/
	}
	else if (action == GLFW_RELEASE)
	{
		open_keyboard_state[key] = 0;

		if (PS2KeyCode(key) == 0xE0) // extended
		{
			MEMORY[KEY_ARRAY+MEMORY[KEY_POS_WRITE]] = (uint8_t)0xE0;
		
			MEMORY[KEY_POS_WRITE]++;

			//if (RAM[KEY_POS_WRITE] >= 0x80) RAM[KEY_POS_WRITE] = 0x00;
		}	

		MEMORY[KEY_ARRAY+MEMORY[KEY_POS_WRITE]] = (uint8_t)0xF0;

		MEMORY[KEY_POS_WRITE]++;

		//if (RAM[KEY_POS_WRITE] >= 0x80) RAM[KEY_POS_WRITE] = 0x00;

		if (PS2KeyCode(key) == 0xE0) // extended
		{
			MEMORY[KEY_ARRAY+MEMORY[KEY_POS_WRITE]] = (uint8_t)PS2ExtendedKeyCode(key);
		}
		else
		{
			MEMORY[KEY_ARRAY+MEMORY[KEY_POS_WRITE]] = (uint8_t)PS2KeyCode(key);
		}

		MEMORY[KEY_POS_WRITE]++;

		//if (RAM[KEY_POS_WRITE] >= 0x80) RAM[KEY_POS_WRITE] = 0x00;
	}

	return;
};

void handleButtons(GLFWwindow *window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		open_button_state[button] = 1;
	}
	else if (action == GLFW_RELEASE)
	{
		open_button_state[button] = 0;
	}

	return;	
};

void handleResize(GLFWwindow *window, int width, int height)
{
	glfwGetWindowSize(window, &width, &height);	

	open_window_x = width;
	open_window_y = height;
	
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// glOrtho() uses the orthographic projection.
	// Basically it makes the shapes not distort as you go near the edges of the screen.
	// This is very unnatural, but is useful when your 3D objects should actually be displayed as 2D objects.
	glOrtho(0, open_window_x, 0, open_window_y, -1000.0f, 1000.0f);
	
	// gluPerspective() uses the perspective projection.
	// Basically this makes the shapes distort as they go near the edges of the screen.
	// This is most natural, especially for 3D environments.
	//gluPerspective(45.0f, (GLfloat)open_window_x / (GLfloat)open_window_y, 0.05f, 500.0f);

	InitializeOpenGLSettings();

	return;
};

void handleCursor(GLFWwindow *window, double xpos, double ypos)
{
	glfwGetCursorPos(window, &xpos, &ypos);

	open_cursor_new_x = xpos;
	open_cursor_new_y = ypos;

	return;
};

void Draw()
{
	float red, green, blue;

	float white_a, white_b;

	float coord_x, coord_y;

	for (unsigned long i=131072; i<262144; i++) // 131072 to 262144, 0x020000 to 0x03FFFF
	{
		red = (float)((MEMORY[i] & 0x30) >> 4);
		green = (float)((MEMORY[i] & 0x0C) >> 2);
		blue = (float)((MEMORY[i] & 0x03));
		
		white_a = (float)((MEMORY[i] & 0x80) >> 7);
		white_b = (float)((MEMORY[i] & 0x40) >> 6);

		coord_x = (float)(i % 512) * 2.0f + 1.0f;
		coord_y = 480.0f - (float)((i / 512) % 256) * 2.0f;

		glBegin(GL_LINES);
	
		if ((MEMORY[i] & 0xC0) == 0x00) // color
		{
			glColor3f(red / 3.0f, green / 3.0f, blue / 3.0f);
			glVertex3f((float)(coord_x),(float)coord_y,0.0f);
			glVertex3f((float)(coord_x),(float)(coord_y-2),0.0f);
			glVertex3f((float)(coord_x+1),(float)coord_y,0.0f);
			glVertex3f((float)(coord_x+1),(float)(coord_y-2),0.0f);
		}
		else // mono
		{
			glColor3f(white_a, white_a, white_a);
			glVertex3f((float)(coord_x),(float)coord_y,0.0f);
			glVertex3f((float)(coord_x),(float)(coord_y-2),0.0f);

			glColor3f(white_b, white_b, white_b);
			glVertex3f((float)(coord_x+1),(float)coord_y,0.0f);
			glVertex3f((float)(coord_x+1),(float)(coord_y-2),0.0f);
		}
	}

	glEnd();	

	return;
};

int Load(const char *filename) // loads 128KB into video area
{
	for (unsigned int i=0; i<256; i++) // randomize zero page
	{
		MEMORY[i] = rand() % 256;
	}

	for (unsigned int i=0; i<512; i++) // bootloader code
	{
		MEMORY[i+0xFE00] = bootloader_code[i];
	}

	FILE *input = NULL;

	input = fopen(filename, "rb");
	if (!input) return 0;

	uint8_t buffer, bytes = 0;

	for (unsigned long i=0; i<131072; i++)
	{
		bytes = fscanf(input, "%c", &buffer);

		if (bytes > 0)
		{
			MEMORY[(unsigned long)(i+131072)] = buffer;
		}
	}

	fclose(input);

	// put jump location at top of bootloader code
	MEMORY[0xFE00] = 0x5C;
	MEMORY[0xFE01] = 0x00;
	MEMORY[0xFE02] = 0xE0;
	MEMORY[0xFE03] = 0x03;

	return 1;
};

void Stop()
{
	printf("Stopped\nA=%02x X=%02x Y=%02x S=%02x P=%06x R=%02x\n", CPU->A, CPU->X, CPU->Y, CPU->sp, CPU->pc, CPU->status);

	return;
};
		
int main(const int argc, const char **argv)
{
	if (argc < 2)
	{
		printf("Acolyte '816 - Simulator\n");
		printf("Argument is a 128KB binary file used for memory\n");
		
		return 0;
	}

	int temp;

	for (int i=0; i<(int)(time(0)%1000); i++)
	{
		temp = rand() % 1000;
	}

	int random_value = 0;

	for (long unsigned random_counter=0; random_counter<(time(0) % 1000); random_counter++)
	{
		random_value = rand() % 1000;
	}

	GLFWwindow* window;

	// Init library
	if (!glfwInit())
	{
		return 0;
	}

	// create windowed mode
	window = glfwCreateWindow(open_window_x, open_window_y, "Acolyte '816", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return 0;
	}

	// make windows context current
	glfwMakeContextCurrent(window);

	// all of the settings needed for typical 3D graphics
	InitializeOpenGLSettings();

	// set keyboard state to 0
	for (int i=0; i<512; i++) open_keyboard_state[i] = 0;

	// set cursor to middle of window
	open_cursor_new_x = (int)(open_window_x/2);
	open_cursor_new_y = (int)(open_window_y/2);
	open_cursor_old_x = (int)(open_window_x/2);
	open_cursor_old_y = (int)(open_window_y/2);
	glfwSetCursorPos(window, (int)(open_window_x/2), (int)(open_window_y/2));

	// modes
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);

	// callbacks
	glfwSetKeyCallback(window, handleKeys);
	glfwSetWindowSizeCallback(window, handleResize);
	glfwSetCursorPosCallback(window, handleCursor);
	glfwSetMouseButtonCallback(window, handleButtons);

	// INITIALIZE HERE!

	if (!Load(argv[1]))
	{
		printf("Error in Load()\n");
		
		//return 0;
	}

	CPU = new mos6502(ReadFunction, WriteFunction, Stop);

	CPU->Reset();

	// loop until closed
	while (!glfwWindowShouldClose(window))
	{
		open_clock_previous = open_clock_current;
		open_clock_current = clock();

		open_frames_counter += (double)(open_clock_current - open_clock_previous);

		if (open_frames_counter >= open_frames_difference)
		{
			open_frames_previous = open_frames_current;
			open_frames_current = clock();

			while (open_frames_counter >= open_frames_difference)
			{
				open_frames_counter -= open_frames_difference;
			}

			open_frames_drawing = true;

			open_frames_per_second_counter++;
		}

		if (open_frames_per_second_timer != time(0))
		{
			open_frames_per_second = open_frames_per_second_counter;
			open_frames_per_second_counter = 0;
			open_frames_per_second_timer = time(0);

			// once per second...
		}

		open_frames_delta = ((double)(open_clock_current - open_clock_previous) / (double)CLOCKS_PER_SEC);

		uint64_t counter = 0;

		CPU->Run(52500*4, counter); // 52500*4 clock cycles per one screen refresh at 60 Hz

		if (open_frames_drawing == true)
		{
			open_frames_drawing = false;

			glViewport(0, 0, open_window_x, open_window_y);
			glClear(GL_COLOR_BUFFER_BIT);

			// DRAW HERE!	

			Draw();

			// make sure V-Sync is on
			glfwSwapInterval(1);

			// swap front and back buffers
			glfwSwapBuffers(window);
		}

		// poll for and process events
		glfwPollEvents();
	}

	printf("Exited\nA=%02x X=%02x Y=%02x S=%02x P=%06x R=%02x\n", CPU->A, CPU->X, CPU->Y, CPU->sp, CPU->pc, CPU->status);

	delete CPU;

	glfwDestroyWindow(window);

	glfwTerminate();

	return 1;
}


