/*
 * main.c
 *
 * Main source file
 *
 *  Created on: 2012-11-19
 *      Author: Anonymous Meerkat
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysymdef.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#define len(x) (sizeof(x)/sizeof(*x))

#define WIDTH 640
#define HEIGHT 480
#define TITLE "BZFPS"
#define MAX_OBJECTS 64
#define FPS_MAX 120
#define PLAYER_HEIGHT 0.2
#define FIRE_MAX 30.0
#define FIRE_SECS 0.05
#define FIRE_PITCH_SHIFT 2
#define FIRE_YAW_SHIFT .5
#define FIRE_DAMAGE .2
// Millis per unit
#define EXPLOSION_TIME 500
#define EXPLOSION_LINES 10
#define MAX_EXPLOSIONS 100

GLint glAttrs[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

typedef enum {
	CUBE, PYRAMID, PLAYER, TANK, KAMIKAZE, TURRET
} OBJTYPE;

typedef struct {
	GLfloat r, g, b;
} color;

typedef struct {
	GLfloat x, z;
} pos;

typedef struct {
	GLfloat x, y, z;
} pos3f;

typedef struct {
	OBJTYPE type;
	pos pos;
	pos size;
	pos rot;
	color color;
	short visible, exists;
	float hp;
} object;

typedef struct {
	long long starttime;
	long long lasttime;
	float size;
	short active;
	pos3f lines[EXPLOSION_LINES];
	pos3f angles[EXPLOSION_LINES];
	object* obj;
	color color;
} explosion;

char* citoa(int i) {
	char s[80];
	sprintf(s, "%i", i);
	char* r = s;
	return r;
}

void glColor(color c) {
	glColor3f(c.r, c.g, c.b);
}

void setColor(color *c, GLfloat r, GLfloat g, GLfloat b) {
	c->r = r;
	c->g = g;
	c->b = b;
}

float toRadians(float degrees) {
	return degrees * 0.0174532925;
}

float toDegrees(float radians) {
	return radians / 0.0174532925;
}

void horizon(color c) {
	// Mountains
	glBegin(GL_LINES);
	glColor(c);
	// Back
	glVertex3f(-1, .1, -1);
	glVertex3f(-.3, .3, -1);
	glVertex3f(-.3, .3, -1);
	glVertex3f(.2, .05, -1);
	glVertex3f(.2, .05, -1);
	glVertex3f(.7, .3, -1);
	glVertex3f(.7, .3, -1);
	glVertex3f(1, .07, -1);
	// Right
	glVertex3f(1, .07, -1);
	glVertex3f(1, .2, -.3);
	glVertex3f(1, .2, -.3);
	glVertex3f(1, .02, .2);
	glVertex3f(1, .02, .2);
	glVertex3f(1, .3, .7);
	glVertex3f(1, .3, .7);
	glVertex3f(1, .2, 1);
	// Front
	glVertex3f(1, .2, 1);
	glVertex3f(.7, .3, 1);
	glVertex3f(.7, .3, 1);
	glVertex3f(.2, .04, 1);
	glVertex3f(.2, .04, 1);
	glVertex3f(-.3, .2, 1);
	glVertex3f(-.3, .2, 1);
	glVertex3f(-1, .1, 1);
	// Left
	glVertex3f(-1, .1, 1);
	glVertex3f(-1, .2, .7);
	glVertex3f(-1, .2, .7);
	glVertex3f(-1, .1, .2);
	glVertex3f(-1, .1, .2);
	glVertex3f(-1, .3, -.3);
	glVertex3f(-1, .3, -.3);
	glVertex3f(-1, .1, -1);
	glEnd();
	// Horizon line
	glBegin(GL_LINES);
	glColor(c);
	glVertex3f(-1, 0, -1);
	glVertex3f(1, 0, -1);
	glVertex3f(1, 0, -1);
	glVertex3f(1, 0, 1);
	glVertex3f(1, 0, 1);
	glVertex3f(-1, 0, 1);
	glVertex3f(-1, 0, 1);
	glVertex3f(-1, 0, -1);
	glEnd();
}

void renderhorizon(pos rot, color c) {
	glPushMatrix();
	glLoadIdentity();
	glRotatef(rot.z, 1.0f, 0.0f, 0.0f);
	glRotatef(rot.x, 0.0f, 1.0f, 0.0f);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	horizon(c);
	glPopAttrib();
	glPopMatrix();
}

void crosshair(short s_, color c, int w, int h) {
	int x = w / 2;
	int y = h / 2;
	short s = s_ / 2;
	glBegin(GL_LINES);
	glColor(c);
	glVertex2f(x - s, y);
	glVertex2f(x + s, y);
	glVertex2f(x, y - s);
	glVertex2f(x, y + s);
	glEnd();
}

void heart(short s_, color c, pos p, short half) {
	short s = s_ / 2, s1 = s / 2;
	int x = p.x, y = p.z;
	glBegin(GL_POLYGON);
	glColor(c);
	glVertex2f(x, y + s);
	glVertex2f(x - s, y - s1);
	glVertex2f(x - s1, y - s);
	glVertex2f(x, y - s1);
	if (half) {
		glVertex2f(x, y + s);
	} else {
		glVertex2f(x + s1, y - s);
		glVertex2f(x + s, y - s1);
		glVertex2f(x, y + s);
	}
	glEnd();
}

void showhp(short hp, color c) {
	pos p;
	p.x = 20;
	p.z = 20;
	int i;
	for (i = 0; i < (hp / 2); i++) {
		heart(20, c, p, 0);
		p.x += 25;
	}
	if (hp % 2 == 1) {
		heart(20, c, p, 1);
	}
}

void cube(GLfloat size, color c) {
	GLfloat s = -size / 2;
	GLfloat e = size / 2;
	// Top
	glBegin(GL_QUADS);
	glColor(c);
	glVertex3f(s, e, s);
	glVertex3f(s, e, e);
	glVertex3f(e, e, e);
	glVertex3f(e, e, s);
	glEnd();
	// Bottom
	glBegin(GL_QUADS);
	glColor(c);
	glVertex3f(s, s, s);
	glVertex3f(e, s, s);
	glVertex3f(e, s, e);
	glVertex3f(s, s, e);
	glEnd();
	// Front
	glBegin(GL_QUADS);
	glColor(c);
	glVertex3f(s, s, e);
	glVertex3f(e, s, e);
	glVertex3f(e, e, e);
	glVertex3f(s, e, e);
	glEnd();
	// Back
	glBegin(GL_QUADS);
	glColor(c);
	glVertex3f(s, s, s);
	glVertex3f(s, e, s);
	glVertex3f(e, e, s);
	glVertex3f(e, s, s);
	glEnd();
	// Left
	glBegin(GL_QUADS);
	glColor(c);
	glVertex3f(s, s, s);
	glVertex3f(s, s, e);
	glVertex3f(s, e, e);
	glVertex3f(s, e, s);
	glEnd();
	// Right
	glBegin(GL_QUADS);
	glColor(c);
	glVertex3f(e, s, s);
	glVertex3f(e, e, s);
	glVertex3f(e, e, e);
	glVertex3f(e, s, e);
	glEnd();
}

void pyramid(GLfloat size, color c) {
	GLfloat s = -size / 2;
	GLfloat e = size / 2;
	GLfloat m = 0;
	glBegin(GL_TRIANGLES);
	glColor(c);
	// first triangle
	glVertex3f(e, s, e);
	glVertex3f(m, e, m);
	glVertex3f(s, s, e);
	// second triangle
	glVertex3f(s, s, e);
	glVertex3f(m, e, m);
	glVertex3f(s, s, s);
	// third triangle
	glVertex3f(s, s, s);
	glVertex3f(m, e, m);
	glVertex3f(e, s, s);
	// last triangle
	glVertex3f(e, s, s);
	glVertex3f(m, e, m);
	glVertex3f(e, s, e);
	glEnd();
	/*glColor(c);
	 glBegin(GL_TRIANGLES);
	 glVertex3f(s, s, e);
	 glVertex3f(s, s, s);
	 glVertex3f(e, s, e);
	 glVertex3f(e, s, e);
	 glVertex3f(s, s, s);
	 glVertex3f(e, s, s);
	 glEnd();*/
}

void init3D(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, ((float) width) / ((float) height), 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void init2DGL(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, width, height, 0.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);
	glDisable(GL_DEPTH_TEST);
}

void hideCursor(Display *disp, Window win) {
	Cursor invisibleCursor;
	Pixmap bitmapNoData;
	XColor black;
	static char noData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	black.red = black.green = black.blue = 0;

	bitmapNoData = XCreateBitmapFromData(disp, win, noData, 8, 8);
	invisibleCursor = XCreatePixmapCursor(disp, bitmapNoData, bitmapNoData,
			&black, &black, 0, 0);
	XDefineCursor(disp, win, invisibleCursor);
	XFreeCursor(disp, invisibleCursor);
}

void showCursor(Display *disp, Window win) {
	XUndefineCursor(disp, win);
}

uint64_t getRawTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL );
	return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;
}

uint64_t getTime(uint64_t* prev_time) {
	uint64_t r = getRawTime();
	uint64_t t = (r - *prev_time);
	*prev_time = r;
	return t;
}

short _colDetect(pos p, pos s, pos p1, pos s1) {
	short r = 0;
	if ((p.x + s.x / 2) > (p1.x - s.x / 2)
			&& (p.x - s.x / 2) < (p1.x + s1.x / 2)) {
		r += 1;
	}
	if ((p.z + s.z / 2) > (p1.z - s.x / 2)
			&& (p.z - s.x / 2) < (p1.z + s1.z / 2)) {
		r += 2;
	}
	return r;
}

object* _colDetectGetObjects(pos p, pos s, object objs[MAX_OBJECTS]) {
	int i;
	object r[MAX_OBJECTS];
	object emptyobject;
	emptyobject.exists = 0;
	emptyobject.visible = 0;
	pos ps;
	ps.x = 0;
	ps.z = 0;
	emptyobject.size = p;
	for (i = 0; i < MAX_OBJECTS; i++) {
		if (i == 0 || !objs[i].exists) {
			r[i] = emptyobject;
			continue;
		}
		short c = _colDetect(p, s, objs[i].pos, objs[i].size);
		if (c < 3) {
			r[i] = emptyobject;
			continue;
		}
		r[i] = objs[i];
	}
	return r;
}

short colDetect(pos *p, pos pp, pos s, object objs[MAX_OBJECTS]) {
	int i;
	for (i = 0; i < MAX_OBJECTS; i++) {
		if (!objs[i].exists || i == 0) {
			continue;
		}
		short c = _colDetect(*p, s, objs[i].pos, objs[i].size);
		if (c < 3) {
			continue;
		}
		short c1 = _colDetect(pp, s, objs[i].pos, objs[i].size);
		short xon = c1 % 2;
		short zon = c1 >= 2;
		if (!xon) {
			p->x = objs[i].pos.x - objs[i].size.x / 2 - s.x / 2;
			if (p->x < pp.x) {
				p->x += objs[i].size.x + s.x;
			}
		}
		if (!zon) {
			p->z = objs[i].pos.z - objs[i].size.z / 2 - s.z / 2;
			if (p->z < pp.z) {
				p->z += objs[i].size.z + s.z;
			}
		}
	}
	return 0;
}

float getAngleFromPoints(pos p, pos p1) {
	float dx = p.x - p1.x;
	float dz = p.z - p1.z;
	return toDegrees(atan2(dx, dz));
}

void normangle(float* angle) {
	while (*angle >= 360) {
		*angle -= 360;
	}
	while (*angle < 0) {
		*angle += 360;
	}
}

pos getRandomPos(pos* p) {
	int i;
	short f = 1;
	pos p1;
	while (1) {
		pos ps;
		ps.x = rand() % MAX_OBJECTS;
		ps.z = rand() % MAX_OBJECTS;
		for (i = 0; i < len(p); i++) {
			if (ps.x == p[i].x && ps.z == p[i].z) {
				f = 0;
			}
		}
		if (f) {
			return ps;
		}
	}
	return p1;
}

char* getTitle(short fps) {
	char inittitle[80];
	memset(inittitle, 0, 80);
	sprintf(inittitle, "%s (%i FPS)", TITLE, fps);
	char* returnme = inittitle;
	return returnme;
}

void getExplosionLines(explosion *e) {
	int i;
	pos3f p;
	pos3f p1;
	p1.x = e->obj->pos.x;
	p1.y = 0;
	p1.z = e->obj->pos.z;
	for (i = 0; i < EXPLOSION_LINES; i++) {
		p.x = rand() % 180;
		p.y = rand() % 180;
		p.z = rand() % 180;
		e->angles[i] = p;
		e->lines[i] = p1;
	}
}

void renderExplosion(explosion *e, long long time) {
	if (!e->active) {
		return;
	}
	if ((time - e->starttime) / 1000 >= e->size * EXPLOSION_TIME) {
		e->active = 0;
	}
	if (time == e->starttime) {
		getExplosionLines(e);
	}
	int i;
	GLfloat calc = (float) ((time - e->starttime) / 1000)
			/ (float) EXPLOSION_TIME;
	glBegin(GL_LINES);
	glColor(e->color);
	for (i = 0; i < EXPLOSION_LINES; i++) {
		glVertex3f(e->lines[i].x, e->lines[i].y, e->lines[i].z);
		e->lines[i].x = e->obj->pos.x + calc * sin(toRadians(e->angles[i].x));
		e->lines[i].y = calc * cos(toRadians(e->angles[i].y));
		e->lines[i].z = e->obj->pos.z + calc * cos(toRadians(e->angles[i].x));
		glVertex3f(e->lines[i].x, e->lines[i].y, e->lines[i].z);
	}
	glEnd();
}

void addExplosion(explosion expls[MAX_EXPLOSIONS], object* o, long long time,
		float s) {
	explosion e;
	e.active = 1;
	e.obj = o;
	e.size = s;
	e.starttime = time;
	e.color = o->color;
	int i;
	for (i = 0; i < MAX_EXPLOSIONS; i++) {
		if (!expls[i].active) {
			expls[i] = e;
			return;
		}
	}
}

void hit(object* o, float damage, explosion expls[MAX_EXPLOSIONS],
		long long time) {
	o->hp -= damage;
	if (o->hp <= 0.0) {
		o->hp = 0.0;
		o->visible = 0;
		o->exists = 0;
		addExplosion(expls, o, time, 5 * o->size.x);
	} else {
		addExplosion(expls, o, time, o->size.x);
	}
}

int main(int argc, char** argv) {
	Display *disp = XOpenDisplay(NULL );
	if (disp == NULL ) {
		printf("Cannot connect to the X server!\n");
		return 1;
	}
	Window root = DefaultRootWindow(disp) ;
	XVisualInfo *vi = glXChooseVisual(disp, 0, glAttrs);
	if (vi == NULL ) {
		printf("No appropriate visual found!\n");
		return 1;
	}
	Colormap cmap = XCreateColormap(disp, root, vi->visual, AllocNone);
	XSetWindowAttributes swinattrs;
	swinattrs.colormap = cmap;
	swinattrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask
			| ButtonMotionMask | PointerMotionMask | ButtonPressMask
			| ButtonPressMask | ButtonReleaseMask;
	Window win = XCreateWindow(disp, root, 0, 0, WIDTH, HEIGHT, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swinattrs);
	int width = WIDTH;
	int height = HEIGHT;
	XMapWindow(disp, win);
	XStoreName(disp, win, TITLE);
	GLXContext ctx = glXCreateContext(disp, vi, NULL, GL_TRUE);
	glXMakeCurrent(disp, win, ctx);
	glEnable(GL_DEPTH_TEST);
	XWindowAttributes winattrs;
	XEvent xev;
	uint64_t currtime = 0;
	getTime(&currtime);
	double delta = 0.0, dist = 0.0, kdist = 0.0, mdist = 0.0;
	float lx = 0.0;
	char* s;
	char c;
	int i, x;
	object objects[MAX_OBJECTS];
	color yellow, green, blue, red, aqua, magenta, white;
	setColor(&yellow, 1.0, 1.0, 0.0);
	setColor(&green, 0.0, 1.0, 0.0);
	setColor(&blue, 0.0, 0.0, 1.0);
	setColor(&red, 1.0, 0.0, 0.0);
	setColor(&aqua, 0.0, 1.0, 1.0);
	setColor(&magenta, 1.0, 0.0, 1.0);
	setColor(&white, 1.0, 1.0, 1.0);
	pos poss[MAX_OBJECTS];
	for (i = 0; i < MAX_OBJECTS; i++) {
		object x;
		pos p = getRandomPos(poss);
		poss[i] = p;
		pos s;
		s.x = 1;
		s.z = 1;
		pos r;
		r.x = 0;
		r.z = 0;
		color c;
		short is = i % 2;
		if (is == 0) {
			c = yellow;
		} else if (is == 1) {
			c = green;
		}/* else if (is == 2) {
		 c = blue;
		 } else if (is == 3) {
		 c = red;
		 } else if (is == 4) {
		 c = aqua;
		 } else if (is == 5) {
		 c = magenta;
		 }*/
		x.pos = p;
		x.size = s;
		x.rot = r;
		x.exists = 1;
		if (i == 0) {
			x.type = PLAYER;
			x.visible = 0;
			x.hp = 20;
		} else {
			if (i % 2 == 0) {
				x.type = CUBE;
				x.hp = 2;
			} else if (i % 2 == 1) {
				x.type = PYRAMID;
				x.hp = 5;
			} else {
				x.hp = 1;
			}
			x.visible = 1;
		}
		x.color = c;
		objects[i] = x;
	}
	object* player = &objects[0];
	XWarpPointer(disp, None, win, 0, 0, 0, 0, width / 2, height / 2);
	short warped = 0;
	hideCursor(disp, win);
	//XGrabPointer(disp, win, True,
	//		ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
	//		GrabModeAsync, GrabModeAsync, win, None, CurrentTime);
	const double sensitivity = 0.0001;
	const float msensitivity = 0.03;
	const float ksensitivity = 0.04;
	const float fps = 1000 / FPS_MAX;
	short fd = 0, bd = 0, ld = 0, rd = 0;
	short loop = 1;
	short fire_pressed = 0, is_shooting = 0;
	long fire_time = 0;
	short fpsc = 0;
	long last_second = currtime;
	explosion explosions[MAX_EXPLOSIONS];
	short fire_yaw_dir = 1;
	while (loop) {
		delta = getTime(&currtime);
		dist = delta * sensitivity;
		kdist = dist * ksensitivity;
		if ((currtime - last_second) >= 1000000) {
			XTextProperty name;
			name.value = (unsigned char*) getTitle(fpsc);
			name.encoding = XA_STRING;
			name.format = 8;
			name.nitems = strlen((char*) name.value);
			XSetWMName(disp, win, &name);
			fpsc = 0;
			last_second = currtime;
		}
		while (XPending(disp) > 0) {
			XNextEvent(disp, &xev);
			switch (xev.type) {
			case Expose:
				XGetWindowAttributes(disp, win, &winattrs);
				glViewport(0, 0, winattrs.width, winattrs.height);
				width = winattrs.width;
				height = winattrs.height;
				break;
			case KeyPress:
				s = XKeysymToString(XLookupKeysym(&xev.xkey, 0));
				if (strcmp(s, "Escape") == 0) {
					loop = 0;
				}
				if (strlen(s) == 1) {
					c = s[0];
				} else {
					c = 0;
				}
				switch (c) {
				case 'w':
					fd = 1;
					break;
				case 's':
					bd = 1;
					break;
				case 'a':
					ld = 1;
					break;
				case 'd':
					rd = 1;
					break;
				default:
					break;
				}
				break;
			case KeyRelease:
				s = XKeysymToString(XLookupKeysym(&xev.xkey, 0));
				if (strcmp(s, "Escape") == 0) {
					loop = 0;
				}
				if (strlen(s) == 1) {
					c = s[0];
				} else {
					c = 0;
				}
				switch (c) {
				case 'w':
					fd = 0;
					break;
				case 's':
					bd = 0;
					break;
				case 'a':
					ld = 0;
					break;
				case 'd':
					rd = 0;
					break;
				default:
					break;
				}
				break;
			case MotionNotify:
				if (warped) {
					warped = 0;
					break;
				}
				player->rot.x = lx;
				player->rot.x += (xev.xmotion.x_root - width / 2 - 1) * dist
						* msensitivity;
				normangle(&player->rot.x);
				lx = player->rot.x;
				XWarpPointer(disp, None, win, 0, 0, 0, 0, width / 2,
						height / 2);
				warped = 1;
				break;
			case ButtonPress:
				fire_pressed = 1;
				is_shooting = 1;
				break;
			case ButtonRelease:
				fire_pressed = 0;
				is_shooting = 0;
				break;
			}
		}
		pos pp = player->pos;
		if (fd) {
			player->pos.x += kdist * sin(toRadians(player->rot.x));
			player->pos.z -= kdist * cos(toRadians(player->rot.x));
		}
		if (bd) {
			player->pos.x -= kdist * sin(toRadians(player->rot.x));
			player->pos.z += kdist * cos(toRadians(player->rot.x));
		}
		if (ld) {
			player->pos.x += kdist * sin(toRadians(player->rot.x - 90));
			player->pos.z -= kdist * cos(toRadians(player->rot.x - 90));
		}
		if (rd) {
			player->pos.x += kdist * sin(toRadians(player->rot.x + 90));
			player->pos.z -= kdist * cos(toRadians(player->rot.x + 90));
		}
		colDetect(&player->pos, pp, player->size, objects);
		if (fire_pressed) {
			if ((currtime - fire_time) < FIRE_SECS * 1000000) {
				goto render;
			}
			player->rot.z = 0;
			player->rot.x = lx;
			if(rand()%2 == 0) {
				fire_yaw_dir = -1;
			} else {
				fire_yaw_dir = 1;
			}
			fire_time = currtime;
			pos size;
			size.x = 1;
			size.z = 1;
			float f;
			float d = FIRE_MAX + 1, d1;
			x = 0;
			for (f = 0.0; f < FIRE_MAX; f += .1) {
				pos p;
				p.x = player->pos.x + f * sin(toRadians(player->rot.x));
				p.z = player->pos.z - f * cos(toRadians(player->rot.x));
				object* objs = _colDetectGetObjects(p, size, objects);
				for (i = 0; i < MAX_OBJECTS; i++) {
					if (!objs[i].exists) {
						continue;
					}
					d1 = sqrt(
							pow((objs[i].pos.x - player->pos.x), 2)
									+ pow((objs[i].pos.z - player->pos.z), 2));
					if (d1 < d) {
						x = i;
						d = d1;
					}
				}
			}
			if (x > 0) {
				hit(&objects[x], FIRE_DAMAGE, explosions, currtime);
			}
		}
		render:
		// Calculate pitch
		if ((currtime - fire_time) < FIRE_SECS * 500000) {
			player->rot.z -= (float) (FIRE_PITCH_SHIFT
					* (float) ((currtime - fire_time) / (FIRE_SECS * 500000)));
			player->rot.x -= fire_yaw_dir * (float) (FIRE_YAW_SHIFT
					* (float) ((currtime - fire_time) / (FIRE_SECS * 500000)));
		} else if ((currtime - fire_time) < FIRE_SECS * 1000000) {
			player->rot.z += (float) (FIRE_PITCH_SHIFT
					* ((float) ((currtime - fire_time) / (FIRE_SECS * 500000))
							- 1));
			player->rot.x += fire_yaw_dir * (float) (FIRE_YAW_SHIFT
					* ((float) ((currtime - fire_time) / (FIRE_SECS * 500000))
							- 1));
		} else {
			player->rot.z = 0;
			player->rot.x = lx;
		}
		// Clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		init3D(width, height);
		// Wireframe
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(2.0);
		// Camera
		glRotatef(player->rot.z, 1.0, 0.0, 0.0);
		glRotatef(player->rot.x, 0.0, 1.0, 0.0);
		glTranslatef(-player->pos.x, 0.0, -player->pos.z);
		// Horizon
		renderhorizon(player->rot, blue);
		// Render
		for (i = 0; i < MAX_OBJECTS; i++) {
			if (!objects[i].exists || !objects[i].visible) {
				continue;
			}
			glTranslatef(objects[i].pos.x, 0.5 - PLAYER_HEIGHT,
					objects[i].pos.z);
			switch (objects[i].type) {
			case CUBE:
				cube(1.0, objects[i].color);
				break;
			case PYRAMID:
				pyramid(1.0, objects[i].color);
				break;
			default:
				break;
			}
			glTranslatef(-objects[i].pos.x, -0.5 + PLAYER_HEIGHT,
					-objects[i].pos.z);
		}
		for (i = 0; i < MAX_EXPLOSIONS; i++) {
			if (!explosions[i].active) {
				continue;
			}
			renderExplosion(&explosions[i], currtime);
		}
		// HUD
		init2DGL(width, height);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0);
		if (is_shooting) {
			crosshair(15, red, width, height);
		} else {
			crosshair(20, white, width, height);
		}
		showhp(player->hp, red);
		// Display the rendering
		glXSwapBuffers(disp, win);
		calcfps: fpsc++;
		int calc = fps - ((getRawTime() - currtime) * 1000);
		/*if (calc > 0 && 0) {
		 usleep(calc * 1000);
		 }*/
	}
	glXMakeCurrent(disp, None, NULL );
	glXDestroyContext(disp, ctx);
	XDestroyWindow(disp, win);
	XCloseDisplay(disp);
	return 0;
}
