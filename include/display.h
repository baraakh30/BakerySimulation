#ifndef DISPLAY_H
#define DISPLAY_H

#include <GL/glut.h>
#include "shared.h"
#include "config.h"

// Display function prototypes
void init_display(int argc, char **argv);
void display_function(void);
void reshape_function(int width, int height);
void timer_function(int value);
void keyboard_function(unsigned char key, int x, int y);
void update_display(void);
void draw_bakery_state(void);
void draw_inventory(void);
void draw_supplies(void);
void draw_statistics(void);
void draw_text(float x, float y, const char *string);

#endif