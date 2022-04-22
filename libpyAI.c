//Xpilot-AI Team 2012
#include <Python.h>
//from xpilot.c -EGG
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#ifndef _WINDOWS
# include <unistd.h>
# ifndef __hpux
#  include <sys/time.h>
# endif
# include <sys/param.h>
# include <netdb.h>
#endif
#ifdef _WINDOWS
# include "NT/winNet.h"
# include "NT/winClient.h"
#endif
#include "version.h"
#include "config.h"
#include "const.h"
#include "types.h"
#include "pack.h"
#include "bit.h"
#include "error.h"
#include "socklib.h"
#include "net.h"
#include "connectparam.h"
#include "protoclient.h"
#include "portability.h"
#include "checknames.h"
#include "commonproto.h"
//end from xpilot.c -EGG
//from xpclient_x11.h -EGG
#include "clientAI.h" //originally xpclient.h -EGG
#ifdef HAVE_X11_X_H
#  include <X11/X.h>
#endif
#ifdef HAVE_X11_XLIB_H
#  include <X11/Xlib.h>
#endif
#ifdef HAVE_X11_XOS_H
#  include <X11/Xos.h>
#endif
#ifdef HAVE_X11_XUTIL_H
#  include <X11/Xutil.h>
#endif
#ifdef HAVE_X11_KEYSYM_H
#  include <X11/keysym.h>
#endif
#ifdef HAVE_X11_XATOM_H
#  include <X11/Xatom.h>
#endif
#ifdef HAVE_X11_XMD_H
#  include <X11/Xmd.h>
#endif
/* X client specific headers */
#ifndef _WINDOWS
#include <X11/Xlib.h> //for X graphics -EGG
#include <X11/keysym.h> //for X keys -EGG
#endif
#include "blockbitmaps.h" //originally bitmaps.h -EGG
#include "dbuff.h"
#include "paint.h" //originally xpaint.h, moved from below xinit.h -EGG
#include "paintdata.h"
#include "record.h"
#include "widget.h"
#include "../common/keys.h"
#include "xevent.h"
//#include "xeventhandlers.h"
#include "xinit.h"
//end from xpclient_x11.h -EGG
//#include <sys/ipc.h>
//#include <sys/shm.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include "xpclient_x11.h"
#include <math.h>
#include "../common/rules.h"
#include "../common/setup.h"
#include "configure.h"
#include "netclient.h"
#ifdef _WINDOWS
# include "../common/NT/winXKey.h" //added for XK_keys
#endif
#define BASE_X(i)	(((i % x_areas) << 8) + ext_view_x_offset)
#define BASE_Y(i)	((ext_view_height - 1 - (((i / x_areas) % y_areas) << 8)) - ext_view_y_offset)
#define PI_AI 3.1415926536
#define sgn(x) ((x<0)?-1:((x>0)?1:0)) //Returns the sign of a number
#define AI_MSGLEN   256 //Max length of a message 
#define AI_MSGMAX   16 //Size of (incoming) message buffer - default maxMessage is 8
#ifdef _WINDOWS
# define XK_Shift_L 0xFFE1
# define XK_Control_L 0xFFE3
#endif
static PyObject* py_loop = NULL;
//Added for headless -EGG
int headless =0;
//Defined some stuff to avoid undefined warnings -EGG
extern int mainAI(int argc, char* argv[]);
message_t *TalkMsg[MAX_MSGS], *GameMsg[MAX_MSGS];
score_object_t	score_objects[MAX_SCORE_OBJECTS];
//Defined selfTrackingDeg & selfHeadingDeg to avoid needing pyAI.h -EGG
double selfTrackingDeg();
double selfHeadingDeg();
struct AI_msg_struct {
    char body[AI_MSGLEN];
    char from[32];
    char to[32];
} AI_msg[AI_MSGMAX];
//Ship stuff -JNE
//Stores all the information that gets updated in the ship buffer -JNE
typedef struct {
    double vel;
    double xVel;
    double yVel;
    double trackingDeg;
    double trackingRad;
    double d;
    int reload;
    ship_t ship;
} shipData_t;
shipData_t allShips[128][3];
int prevFrameShips = 0;
//Shot tracking stuff -EGG
//Can currently track up to 100 shots. -EGG
#define AISHOT_MAX		100	/*max number of shots */
struct AIshot_struct {
    int x;
    int y;
    int dist;
    int xdir;
    int vel;
    int veldir;
    int imaginary;		/* 1 if just a predicted shot and 0 if a real shot */
    int fresh_shot;		/* -1 if not, and index pointer of ship who shot if */
    int idir;			/* direction from you shot will intercept nearest */
    int idist;			/* intercept distance */
    int itime;			/* time to intercept */
    int alert;			/*MIN(yatx + timex, xaty + timey) */
    int id;			/*bullet id */
} AIshot[AISHOT_MAX];
struct AIshot_struct AIshot_buffer[AISHOT_MAX];
//Our many thanks to Darel Rex Finley for the quickSort we based this on - <3 the xpilot-ai team
//quickSort
//
//This public-domain C implementation by Darel Rex Finley.
//
//* Returns YES if sort was successful, or NO if the nested
//pivots went too deep, in which case your array will have
//been re-ordered, but probably not sorted correctly.
//
//* This function assumes it is called with valid parameters.
//
//* Example calls:
//quickSort(&myArray[0],5); //sorts elements 0, 1, 2, 3, and 4
//quickSort(&myArray[3],5); //sorts elements 3, 4, 5, 6, and 7
int quickSortShots() {
    #define  MAX_LEVELS  1000
    struct AIshot_struct piv;
    int  beg[MAX_LEVELS], end[MAX_LEVELS], i=0, L, R ;
    while (AIshot_buffer[i].x != -1) i++;
    beg[0]=0; end[0]=i; i=0;
    while (i>=0) {
        L=beg[i]; R=end[i]-1;
        if (L<R) {
            piv=AIshot_buffer[L]; if (i==MAX_LEVELS-1) return -1;
            while (L<R) {
                while (AIshot_buffer[R].alert>=piv.alert && L<R) R--; if (L<R) AIshot_buffer[L++]=AIshot_buffer[R];
                while (AIshot_buffer[L].alert<=piv.alert && L<R) L++; if (L<R) AIshot_buffer[R--]=AIshot_buffer[L]; }
            AIshot_buffer[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L; }
        else {
            i--; }}
    return 1; }
//
//START FROM OLD AI CODE
//
//AI.h
struct AIshot_image_struct {
    int x;
    int y;
} AIshot_image[3][AISHOT_MAX];
int AI_delaystart;
int AI_shotspeed;
int AI_repeatrate;
int AIshot_toggle;
int AIshot_previoustoggle;
int AIshot_shipping;
int AI_alerttimemult;
float AI_degree(float radian) {
    return (360.0 * radian) / (2.0 * PI_AI);
}
int AI_distance(int x1, int y1, int x2, int y2) {
    return (int) sqrt(sqr(abs(x1 - x2)) + sqr(abs(y1 - y2)));
}
float AI_radian(float degree) {
    return 2.0 * PI_AI / 360.0 * degree;
}
void AIshot_reset() {
    int i = 0;
    while (i < AISHOT_MAX && AIshot[i].x != -1) {
	AIshot[i].x = -1;
	AIshot[i].y = -1;
	AIshot[i].dist = -1;
	AIshot[i].xdir = -1;
	AIshot[i].vel = -1;
	AIshot[i].veldir = -1;
	AIshot[i].idir = -1;
	AIshot[i].idist = -1;
	AIshot[i].itime = -1;
	AIshot[i].imaginary = -1;
	AIshot[i].fresh_shot = -1;
	AIshot[i].alert = -1;
	i++;
    }
    return;
}
void AIshot_buffer_reset() {
    int i = 0;
    while (i < AISHOT_MAX && AIshot_buffer[i].x != -1) {
	AIshot_buffer[i].x = -1;
	AIshot_buffer[i].y = -1;
	AIshot_buffer[i].dist = -1;
	AIshot_buffer[i].xdir = -1;
	AIshot_buffer[i].vel = -1;
	AIshot_buffer[i].veldir = -1;
	AIshot_buffer[i].idir = -1;
	AIshot_buffer[i].idist = -1;
	AIshot_buffer[i].itime = -1;
	AIshot_buffer[i].imaginary = -1;
	AIshot_buffer[i].fresh_shot = -1;
	AIshot_buffer[i].alert = -1;
	i++;
    }
    return;
}
void AIshot_image_reset(int index) {
    int i = index;
    while (i < AISHOT_MAX && AIshot_image[index][i].x != -1) {
	AIshot_image[index][i].x = -1;
	AIshot_image[index][i].y = -1;
	i++;
    }
    return;
}
int AIshot_calcxdir(int si) {
    if (AIshot[si].y - pos.y < 0) {
	return (int) (360.0 -
		      (360.0 / (2.0 * PI_AI)) *
		      acos((float)
			   ((float) (AIshot[si].x - pos.x) /
			    (float) AIshot[si].dist)));
    }
    else {
	return (int) (360.0 / (2.0 * PI_AI)) *
	    acos((float)
		 ((float) (AIshot[si].x - pos.x) /
		  (float) AIshot[si].dist));
    }
}
void AIshot_refresh() {
    int i;
    if (AIshot_toggle > 0) {
	i = 1;
	while (i == -1)
	    i = quickSortShots();
	AIshot_reset();
	i = 0;
	while (i < AISHOT_MAX && AIshot_buffer[i].x != -1) {
	    AIshot[i].x = AIshot_buffer[i].x;
	    AIshot[i].y = AIshot_buffer[i].y;
	    AIshot[i].dist =
		sqrt(sqr(abs(AIshot[i].x - pos.x)) +
		     sqr(abs(AIshot[i].y - pos.y)));
	    AIshot[i].xdir = AIshot_calcxdir(i);
	    AIshot[i].vel = AIshot_buffer[i].vel;
	    AIshot[i].veldir = AIshot_buffer[i].veldir;
	    AIshot[i].itime = AIshot_buffer[i].itime;
	    AIshot[i].imaginary = AIshot_buffer[i].imaginary;
	    AIshot[i].fresh_shot = AIshot_buffer[i].fresh_shot;
	    AIshot[i].idist = AIshot_buffer[i].idist;
	    AIshot[i].idir = AIshot_buffer[i].idir;

	    AIshot[i].alert = AIshot_buffer[i].alert;
	    i++;
	}
	AIshot_image_reset(2);
	i = 0;
	while (i < AISHOT_MAX && AIshot_image[1][i].x != -1) {
	    AIshot_image[2][i].x = AIshot_image[1][i].x;
	    AIshot_image[2][i].y = AIshot_image[1][i].y;
	    i++;
	}
	AIshot_image_reset(1);
	i = 0;
	while (i < AISHOT_MAX && AIshot_image[0][i].x != -1) {
	    AIshot_image[1][i].x = AIshot_image[0][i].x;
	    AIshot_image[1][i].y = AIshot_image[0][i].y;
	    i++;
	}
	AIshot_image_reset(0);
	AIshot_buffer_reset();
	AIshot_toggle = 0;
    }
    return;
}
float AIshot_calcVel(int newX, int newY, int oldX, int oldY) {
    if (oldX == -1 && oldY == -1)
	return -1.0;
    else
	return sqrt(sqr(fabs((float) newX - (float) oldX)) +
		    sqr(fabs((float) newY - (float) oldY)));
}
float AIshot_calcVelDir(int newX, int newY, int oldX, int oldY, float vel) {
    if (oldX == -1)
	return -1.0;
    else if (vel > 0.0) {
	if (newY - oldY < 0) {
	    return (360.0 -
		    (360.0 / (2.0 * PI_AI)) *
		    acos((float) ((float) (newX - oldX) / vel)));
	}
	else {
	    return (360.0 / (2.0 * PI_AI)) *
		acos((float) ((float) (newX - oldX) / vel));
	}
    }
    else
	return 0.0;
}
void AIshot_addtobuffer(int x, int y, float vel, float veldir, int imaginary, int fresh_shot) {
    float theta1, theta2;
    float A, B, C, BAC;
    int newx1, newx2, newy1, newy2;
    int i = 0;
    while (i < AISHOT_MAX && AIshot_buffer[i].x != -1)
	i++;
    AIshot_buffer[i].x = x;
    AIshot_buffer[i].y = y;
    AIshot_buffer[i].dist =
                     (int) sqrt(sqr(abs(AIshot_buffer[i].x - pos.x)) +
		     sqr(abs(AIshot_buffer[i].y - pos.y)));
    AIshot_buffer[i].vel = (int) vel;
    AIshot_buffer[i].veldir = (int) veldir;
    AIshot_buffer[i].imaginary = imaginary;
    AIshot_buffer[i].fresh_shot = fresh_shot;
    theta1 = AI_radian((float) selfTrackingDeg());
    theta2 = AI_radian(veldir);
    A = pow(selfSpeed() * sin(theta1) - vel * sin(theta2),
	    2) + pow(selfSpeed() * cos(theta1) - vel * cos(theta2), 2);
    B = 2 * ((pos.y - y) *
	     (selfSpeed() * sin(theta1) - vel * sin(theta2)) + (pos.x - x) *
	     (selfSpeed() * cos(theta1) - vel * cos(theta2)));
    C = pow(pos.x - x, 2) + pow(pos.y - y, 2);
    BAC = pow(B, 2) - 4 * A * C;
    if (BAC >= 0) {
	BAC = (-1 * B + pow(BAC, .5));
	if ((BAC / (2 * A)) < 0)
	    BAC = (-1 * B - pow(pow(B, 2) - 4 * A * C, .5));
	AIshot_buffer[i].itime = BAC / (2 * A);
	AIshot_buffer[i].idist = 777;
	AIshot_buffer[i].idir = 777;
    }
    else {
	AIshot_buffer[i].itime = (-1 * B) / (2 * A);
	AIshot_buffer[i].idist = C - pow(B, 2) / (4 * A);
	AIshot_buffer[i].idir = 777;
    }
    newx1 = pos.x + selfSpeed() * cos(theta1) * AIshot_buffer[i].itime;
    newx2 = x + vel * cos(theta2) * AIshot_buffer[i].itime;
    newy1 = pos.y + selfSpeed() * sin(theta1) * AIshot_buffer[i].itime;
    newy2 = y + vel * sin(theta2) * AIshot_buffer[i].itime;
    AIshot_buffer[i].idist = AI_distance(newx1, newy1, newx2, newy2);
    if ((newy2 - newy1) < 0) {
	AIshot_buffer[i].idir =
	    (int) (360.0 -
		   (360.0 / (2.0 * PI_AI)) *
		   acos((float)
			((float) (newx2 - newx1) /
			 (float) AIshot_buffer[i].idist)));
    }
    else {
	AIshot_buffer[i].idir =
	    (int) (360.0 / (2.0 * PI_AI)) *
	    acos((float)
		 ((float) (newx2 - newx1) /
		  (float) AIshot_buffer[i].idist));
    }
    AIshot_buffer[i].alert =
	abs(AIshot_buffer[i].idist +
	    (int) (AIshot_buffer[i].itime * AI_alerttimemult));
    if (AIshot_buffer[i].itime < 0 || AIshot_buffer[i].itime == 0) {
	AIshot_buffer[i].alert = 30000;
    }
    AIshot[i].id = i;
    return;
}
/* this calculates the velocity and direction of each shot by comparing all possible velocites and directions for three images of shot locations: present, first past, second past.  If it can't find two matching velocites and directions in a row for a present shot, it will check to see if a ship was there two frames ago that could have shot it.  works farely well but occasionally there are some bad calculations where shots that don't really exist are added*/
void AIshot_addshot(int px, int py) {
    int i1, i2;
    float tempvel1, tempdir1, tempvel2, tempdir2;
    int found = 0;
    int i = 0;
    if (AIshot_toggle == 0) AIshot_reset();
    while (AIshot_image[0][i].x != -1 && i < AISHOT_MAX) i++;
    AIshot_image[0][i].x = pos.x - ext_view_width / 2 + WINSCALE(px);
    AIshot_image[0][i].y = pos.y + ext_view_height / 2 - WINSCALE(py);
    i1 = 0;
    while (i1 < AISHOT_MAX && AIshot_image[1][i1].x != -1) {
	tempvel1 =
	    AIshot_calcVel(AIshot_image[0][i].x, AIshot_image[0][i].y,
			   AIshot_image[1][i1].x, AIshot_image[1][i1].y);
	tempdir1 =
	    AIshot_calcVelDir(AIshot_image[0][i].x, AIshot_image[0][i].y,
			      AIshot_image[1][i1].x, AIshot_image[1][i1].y,
			      tempvel1);
	i2 = 0;
	while (i2 < AISHOT_MAX && AIshot_image[2][i2].x != -1) {
	    tempvel2 =
		AIshot_calcVel(AIshot_image[1][i1].x,
			       AIshot_image[1][i1].y,
			       AIshot_image[2][i2].x,
			       AIshot_image[2][i2].y);
	    if (fabs(tempvel1 - tempvel2) < 6.0) {
		tempdir2 =
		    AIshot_calcVelDir(AIshot_image[1][i1].x,
				      AIshot_image[1][i1].y,
				      AIshot_image[2][i2].x,
				      AIshot_image[2][i2].y, tempvel2);
		if (fabs(tempdir1 - tempdir2) < 6.0) {
		    AIshot_addtobuffer(AIshot_image[0][i].x,
				       AIshot_image[0][i].y,
				       (tempvel1 + tempvel2) / 2.0,
				       (tempdir1 + tempdir2) / 2.0, 0, -1);
		    found = 1;
		    i2 = AISHOT_MAX;
		    i1 = AISHOT_MAX;
		}
	    }
	    i2++;
	}
	if (found == 0) { 
	    i2 = 0;
	    while (i2 < num_ship) {
		tempvel2 =
		    AIshot_calcVel(AIshot_image[1][i1].x,
				   AIshot_image[1][i1].y,
				   allShips[i2][2].ship.x +
				   (int) (18.0 *
					  cos(AI_radian
					      ((float)
					       allShips[i2][2].ship.dir))),
				   allShips[i2][2].ship.y +
				   (int) (18.0 *
					  sin(AI_radian
					      ((float)
					       allShips[i2][2].ship.dir))));
		if (fabs(tempvel1 - tempvel2) < 17.0) {
		    tempdir2 =
			AIshot_calcVelDir(AIshot_image[1][i1].x,
					  AIshot_image[1][i1].y,
					  allShips[i2][2].ship.x +
					  (int) (18.0 *
						 cos(AI_radian
						     ((float)
						      allShips[i2][2].ship.dir))),
					  allShips[i2][2].ship.y +
					  (int) (18.0 *
						 sin(AI_radian
						     ((float)
						      allShips[i2][2].ship.dir))),
					  tempvel2);
		    if (fabs(tempdir1 - tempdir2) < 17.0) {
			AIshot_addtobuffer(AIshot_image[0][i].x,
					   AIshot_image[0][i].y,
					   (tempvel1 + tempvel2) / 2.0,
					   (tempdir1 + tempdir2) / 2.0, 0,
					   (int)&allShips[i2][2].ship);
			allShips[i2][0].reload = AI_repeatrate - 1; 
			found = 1;
			i2 = num_ship;
			i1 = AISHOT_MAX;
		    }
		}
		i2++;
	    }
	}
	i1++;
    }
    AIshot_toggle++;
    return;
}
//
//END FROM OLD AI CODE
//
void prepareShots() {
    int num_shots = 0;
    int i, x_areas, y_areas, areas, max_;
    AIshot_refresh();
    x_areas = (active_view_width + 255) >> 8;
    y_areas = (active_view_height + 255) >> 8;
    areas = x_areas * y_areas;
    max_ = areas * (num_spark_colors >= 3 ? num_spark_colors : 4);
    for (i = 0; i < max_; i++) {
        int x, y, j;
        if (num_fastshot[i] > 0) {
            x = BASE_X(i);
            y = BASE_Y(i);
            for (j = 0; j < num_fastshot[i]; j++) {
                num_shots++;
                if (num_shots < 100) AIshot_addshot(x + fastshot_ptr[i][j].x, y - fastshot_ptr[i][j].y);
                else printf("HOLY SHOTS, BATMAN! There are more than 100 shots on screen, we can't track that yet!\n");
            }
        }
    }
    if (quickSortShots() == -1) printf("OH NOES! Unable to sort the shots with given MAX_LEVELS");
}
//END Shot tracking stuff -EGG
//Reload tracker
int reload = 0;
//From xpilot-ng's event.c to make key functions easier -EGG
typedef int xp_keysym_t;
void Keyboard_button_pressed(xp_keysym_t ks)
{
    bool change = false;
    keys_t key;
    for (key = Lookup_key(NULL, ks, true);
	 key != KEY_DUMMY;
	 key = Lookup_key(NULL, ks, false))
	change |= Key_press(key);
    if (change)
	Net_key_change();
}
void Keyboard_button_released(xp_keysym_t ks)
{
    bool change = false;
    keys_t key;
    for (key = Lookup_key(NULL, ks, true);
	 key != KEY_DUMMY;
	 key = Lookup_key(NULL, ks, false))
	change |= Key_release(key);
    if (change)
	Net_key_change();
}
//END from event.c
//All button press methods are documented on 
static PyObject* py_turnLeft(PyObject* pySelf, PyObject* args) {	//turns left as if the 'a' key was pressed -JNE
    int flag;
    if (!PyArg_ParseTuple(args, "i", &flag)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    if (flag) 
        Keyboard_button_pressed(XK_a);
    else
        Keyboard_button_released(XK_a);
    Py_RETURN_NONE;
}
static PyObject* py_turnRight(PyObject* pySelf, PyObject*args) { //turns right as if the 's' key was pressed -JNE
    int flag;
    if (!PyArg_ParseTuple(args, "i", &flag)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    if (flag) 
        Keyboard_button_pressed(XK_s);
    else
        Keyboard_button_released(XK_s);
    Py_RETURN_NONE;
}
void turn(int deg) {	//turns based on the speed, 'deg', that is passed in -JNE          //DO NOT CHANGE, NEEDED IN ORDER FOR turnToDeg to work -JRA
     deg = deg<0 ? MAX(deg, -MAX_PLAYER_TURNSPEED) : MIN(deg, MAX_PLAYER_TURNSPEED);
     if (deg)
	  Send_pointer_move(deg * -128 / 360);
}
static PyObject* py_turn(PyObject* pySelf, PyObject* args) {	//turns based on the speed, 'deg', that is passed in -JNE
    int deg;
    if (!PyArg_ParseTuple(args, "i", &deg)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
     deg = deg<0 ? MAX(deg, -MAX_PLAYER_TURNSPEED) : MIN(deg, MAX_PLAYER_TURNSPEED);
     if (deg)
	  Send_pointer_move(deg * -128 / 360);
     Py_RETURN_NONE;
}
static PyObject* py_turnToDeg(PyObject* pySelf, PyObject* args) {
	//sets the ship's heading to a fixed degree -JNE
	int deg;
	if (!PyArg_ParseTuple(args, "i", &deg)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
	int selfHead;
	selfHead = (int)selfHeadingDeg();
	int speed;
	int dif;
	dif = abs(deg-selfHead);
	if (dif != 0) {			//sets speed depending on how close it is to the target angle -JNE
		if (dif > 20 && dif < 340) {
			speed = 64;
		}
		else if (dif >= 2 && dif <= 358) {
			speed = 8;
		}
		else {		//heading in Xpilot goes in increments of 2.8125 (because it uses a scale of 0-127 instead of 0-360), so we stop if we're close -JNE
			speed = 0;
		}
		if (deg < 180) {
			if (selfHead-deg<180 && selfHead-deg>0) {
				turn(-speed);
			}
			else {
				turn(speed);
			}
		}
		else if (deg >= 180) {
			if (deg-selfHead<180 && deg-selfHead>0) {
				turn(speed);
			}
			else {
				turn(-speed);
			}
		}
	}
    Py_RETURN_NONE;
}
//Converts degrees (int) to radians (double). -EGG
static PyObject* py_thrust(PyObject* pySelf, PyObject* args) {
    int flag;
    if (!PyArg_ParseTuple(args, "i", &flag)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    if (flag) 
        Keyboard_button_pressed(XK_Shift_L);
    else
        Keyboard_button_released(XK_Shift_L);
    Py_RETURN_NONE;
}
//Sets the player's turnspeed. -EGG
//Will not take effect until the player STARTS turning AFTER this is called. -EGG
//Parameters: int for speed, min = 0, max = 64. -EGG
static PyObject* py_setTurnSpeed(PyObject* pySelf, PyObject* args) {
    double s;
    if (!PyArg_ParseTuple(args, "d", &s)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    turnspeed = s;
    turnspeed = MIN(turnspeed, MAX_PLAYER_TURNSPEED);
    turnspeed = MAX(turnspeed, MIN_PLAYER_TURNSPEED);
    Send_turnspeed(turnspeed);
    Config_redraw();
    control_count = CONTROL_DELAY;
    Py_RETURN_NONE;
}
void setTurnSpeed(double s) { //added for setTurnSpeedDeg -CJG
    turnspeed = s;
    turnspeed = MIN(turnspeed, MAX_PLAYER_TURNSPEED);
    turnspeed = MAX(turnspeed, MIN_PLAYER_TURNSPEED);
    Send_turnspeed(turnspeed);
    Config_redraw();
    control_count = CONTROL_DELAY;
}
static PyObject* py_setTurnSpeedDeg(PyObject* pySelf, PyObject* args) { //-CJG
	double s;
	if (!PyArg_ParseTuple(args, "d", &s)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
	setTurnSpeed((8.0/3.0)*(double)s);
}
static PyObject* py_setPower(PyObject* pySelf, PyObject* args) {
    double s;
    if (!PyArg_ParseTuple(args, "d", &s)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    power = s;
    power = MIN(power, MAX_PLAYER_POWER);
    power = MAX(power, MIN_PLAYER_POWER);
    Send_power(power);
    Config_redraw();
    control_count = CONTROL_DELAY;
    Py_RETURN_NONE;
}
static PyObject* py_fasterTurnrate(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_Add);
    Keyboard_button_released(XK_KP_Add);
    Py_RETURN_NONE;
}
static PyObject* py_slowerTurnrate(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_Subtract);
    Keyboard_button_released(XK_KP_Subtract);
    Py_RETURN_NONE;
}
static PyObject* py_morePower(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_Multiply);
    Keyboard_button_released(XK_KP_Multiply);
    Py_RETURN_NONE;
}
static PyObject* py_lessPower(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_Divide);
    Keyboard_button_released(XK_KP_Divide);
    Py_RETURN_NONE;
}
//~ //End movement methods -JNE
//~ //Shooting methods -JNE
static PyObject* py_fireShot(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_Return);
    Keyboard_button_released(XK_Return);
    if (reload==0) reload = AI_repeatrate - 1;
    Py_RETURN_NONE;
}
static PyObject* py_fireMissile(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_backslash);
        Keyboard_button_released(XK_backslash);
        Py_RETURN_NONE;
}
static PyObject* py_fireTorpedo(PyObject* pySelf, PyObject* args) { 
    Keyboard_button_pressed(XK_apostrophe);
    Keyboard_button_released(XK_apostrophe);
    Py_RETURN_NONE;
}
static PyObject* py_fireHeat(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_semicolon);
        Keyboard_button_released(XK_semicolon);
        Py_RETURN_NONE;
}
static PyObject* py_dropMine(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_Tab);
        Keyboard_button_released(XK_Tab);
        Py_RETURN_NONE;
}
static PyObject* py_detachMine(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_bracketright);
        Keyboard_button_released(XK_bracketright);
        Py_RETURN_NONE;
}
static PyObject* py_detonateMines(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_equal);
        Keyboard_button_released(XK_equal);
        Py_RETURN_NONE;
}
static PyObject* py_fireLaser(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_slash);
        Keyboard_button_released(XK_slash);
        Py_RETURN_NONE;
}
//End shooting methods -JNE
//Item usage methods -JNE
static PyObject* py_tankDetach(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_r);
        Keyboard_button_released(XK_r);
	Py_RETURN_NONE;
}
static PyObject* py_cloak(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_BackSpace);
        Keyboard_button_released(XK_BackSpace);
	Py_RETURN_NONE;
}
static PyObject* py_ecm(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_bracketleft);
        Keyboard_button_released(XK_bracketleft);
	Py_RETURN_NONE;
}
static PyObject* py_transporter(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_t);
        Keyboard_button_released(XK_t);
	Py_RETURN_NONE;
}
static PyObject* py_tractorBeam(PyObject* pySelf, PyObject* args) {
	int flag;
	if (!PyArg_ParseTuple(args, "i", &flag)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
        if (flag)
                Keyboard_button_pressed(XK_comma);
        else
                Keyboard_button_released(XK_comma);
	Py_RETURN_NONE;
}
static PyObject* py_pressorBeam(PyObject* pySelf, PyObject* args) {
	int flag;
	if (!PyArg_ParseTuple(args, "i", &flag)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
        if (flag)
                Keyboard_button_pressed(XK_period);
        else
                Keyboard_button_released(XK_period);
	Py_RETURN_NONE;
}
static PyObject* py_phasing(PyObject* pySelf, PyObject* args){
	Keyboard_button_pressed(XK_p);
	Keyboard_button_released(XK_p);
	Py_RETURN_NONE;
}
static PyObject* py_shield(PyObject* pySelf, PyObject* args) {
    int flag;
    if (flag)
       Keyboard_button_pressed(XK_space);
    else
        Keyboard_button_released(XK_space);
    Py_RETURN_NONE;
}
static PyObject* py_emergencyShield(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_g);
        Keyboard_button_released(XK_g);
	Py_RETURN_NONE;
}
static PyObject* py_hyperjump(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_q);
        Keyboard_button_released(XK_q);
	Py_RETURN_NONE;
}
static PyObject* py_nextTank(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_e);
        Keyboard_button_released(XK_e);
	Py_RETURN_NONE;
}
static PyObject* py_prevTank(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_w);
        Keyboard_button_released(XK_w);
	Py_RETURN_NONE;
}
static PyObject* py_toggleAutopilot(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_h);
        Keyboard_button_released(XK_h);
	Py_RETURN_NONE;
}
static PyObject* py_emergencyThrust(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_j);
        Keyboard_button_released(XK_j);
	Py_RETURN_NONE;
}
static PyObject* py_deflector(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_0);
        Keyboard_button_released(XK_0);
	Py_RETURN_NONE;
}
static PyObject* py_selectItem(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_KP_0);
        Keyboard_button_released(XK_KP_0);
	Py_RETURN_NONE;
}
static PyObject* py_loseItem(PyObject* pySelf, PyObject* args) { 
    Keyboard_button_pressed(XK_KP_Decimal);
    Keyboard_button_released(XK_KP_Decimal);
	Py_RETURN_NONE;
}
//End item usage methods -JNE
//Lock methods -JNE
static PyObject* py_lockNext(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_Right);
        Keyboard_button_released(XK_Right);
	Py_RETURN_NONE;
}
static PyObject* py_lockPrev(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_Left);
        Keyboard_button_released(XK_Left);
	Py_RETURN_NONE;
}
static PyObject* py_lockClose(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_Up);
        Keyboard_button_released(XK_Up);
	Py_RETURN_NONE;
}
static PyObject* py_lockNextClose(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_Down);
        Keyboard_button_released(XK_Down);
	Py_RETURN_NONE;
}
static PyObject* py_loadLock1(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_5);
	Keyboard_button_released(XK_5);
	Py_RETURN_NONE;
}
static PyObject* py_loadLock2(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_6);
	Keyboard_button_released(XK_6);
	Py_RETURN_NONE;
}
static PyObject* py_loadLock3(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_7);
	Keyboard_button_released(XK_7);
	Py_RETURN_NONE;
}
static PyObject* py_loadLock4(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_8);
	Keyboard_button_released(XK_8);
	Py_RETURN_NONE;
}
//End lock methods -JNE
//Modifier methods -JNE
static PyObject* py_toggleNuclear(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_n);
        Keyboard_button_released(XK_n);
	Py_RETURN_NONE;
}
static PyObject* py_togglePower(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_b);
        Keyboard_button_released(XK_b);
	Py_RETURN_NONE;
}
static PyObject* py_toggleVelocity(PyObject* pySelf, PyObject* args) {
      Keyboard_button_pressed(XK_v);
        Keyboard_button_released(XK_v);
	Py_RETURN_NONE;
}
static PyObject* py_toggleCluster(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_c);
        Keyboard_button_released(XK_c);
	Py_RETURN_NONE;
}
static PyObject* py_toggleMini(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_x);
    Keyboard_button_released(XK_x);
    Py_RETURN_NONE;
}
static PyObject* py_toggleSpread(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_z);
    Keyboard_button_released(XK_z);
    Py_RETURN_NONE;
}
static PyObject* py_toggleLaser(PyObject* pySelf, PyObject* args) {
	Keyboard_button_pressed(XK_l);
	Keyboard_button_released(XK_l);
	Py_RETURN_NONE;
}
static PyObject* py_toggleImplosion(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_i);
        Keyboard_button_released(XK_i);
	Py_RETURN_NONE;
}
static PyObject* py_toggleUserName(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_u);
    Keyboard_button_released(XK_u);
    Py_RETURN_NONE;
}
static PyObject* py_loadModifiers1(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_1);
        Keyboard_button_released(XK_1);
	Py_RETURN_NONE;
}
static PyObject* py_loadModifiers2(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_2);
        Keyboard_button_released(XK_2);
	Py_RETURN_NONE;
}
static PyObject* py_loadModifiers3(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_3);
        Keyboard_button_released(XK_3);
	Py_RETURN_NONE;
}
static PyObject* py_loadModifiers4(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_4);
        Keyboard_button_released(XK_4);
	Py_RETURN_NONE;
}
static PyObject* py_clearModifiers(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_k);
        Keyboard_button_released(XK_k);
	Py_RETURN_NONE;
}
//End modifier methods -JNE
//Map features -JNE
static PyObject* py_connector(PyObject* pySelf, PyObject* args) {
	int flag;
	if (!PyArg_ParseTuple(args, "i", &flag)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
        if (flag)
                Keyboard_button_pressed(XK_Control_L);
        else
                Keyboard_button_released(XK_Control_L);
	Py_RETURN_NONE;
}
static PyObject* py_dropBall(PyObject* pySelf, PyObject* args) {
        Keyboard_button_pressed(XK_d);
        Keyboard_button_released(XK_d);
	Py_RETURN_NONE;
}
static PyObject* py_refuel(PyObject* pySelf, PyObject* args) {
	int flag;
	if (!PyArg_ParseTuple(args, "i", &flag)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
	if (flag)
                Keyboard_button_pressed(XK_Control_L);
        else
                Keyboard_button_released(XK_Control_L);
	Py_RETURN_NONE;
}
//End map features -JNE
//Other options -JNE

static PyObject* py_keyHome(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_Home);
    Keyboard_button_released(XK_Home);
    Py_RETURN_NONE;
}
static PyObject* py_selfDestruct(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_End);
    Keyboard_button_released(XK_End);
    Py_RETURN_NONE;
}
static PyObject* py_pauseAI(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_Pause);
    Keyboard_button_released(XK_Pause);
    Py_RETURN_NONE;
}
static PyObject* py_swapSettings(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_Escape);
	Keyboard_button_released(XK_Escape);
    Py_RETURN_NONE;
}
static PyObject* py_quitAI(PyObject* pySelf, PyObject* args) {
    quitting = true;
    Py_RETURN_NONE;
}
static PyObject* py_talkKey(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_m);
    Keyboard_button_released(XK_m);
    Py_RETURN_NONE;    
}
static PyObject* py_toggleShowMessage(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_9);
    Keyboard_button_released(XK_KP_9);
    Py_RETURN_NONE;    
}
static PyObject* py_toggleShowItems(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_8);
    Keyboard_button_released(XK_KP_8);
    Py_RETURN_NONE;    
}
static PyObject* py_toggleCompass(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_KP_7);
    Keyboard_button_released(XK_KP_7);
    Py_RETURN_NONE;
}
static PyObject* py_repair(PyObject* pySelf, PyObject* args) {
	Keyboard_button_pressed(XK_f);
	Keyboard_button_released(XK_f);
    Py_RETURN_NONE;
}
static PyObject* py_reprogram(PyObject* pySelf, PyObject* args) {
    Keyboard_button_pressed(XK_grave);
	Keyboard_button_released(XK_grave);
    Py_RETURN_NONE;
}
//Talk Function, can't be called too frequently or client will flood - JTO
static PyObject* py_talk(PyObject* pySelf, PyObject* args) {
    char* talk_str;
    	if (!PyArg_ParseTuple(args, "s", &talk_str)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
	Net_talk(talk_str);
    Py_RETURN_NONE;
}
static PyObject* py_scanMsg(PyObject* pySelf, PyObject* args) {
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    if (id<MAX_MSGS) return Py_BuildValue("s",TalkMsg[id]->txt);
    return Py_BuildValue("s","");
}
static PyObject* py_scanGameMsg(PyObject* pySelf, PyObject* args) {
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    if (id<MAX_MSGS) return Py_BuildValue("s",GameMsg[id]->txt);
    return Py_BuildValue("s","");
}//End other options -JNE
//Self properties -JNE
static PyObject* py_selfX(PyObject* pySelf, PyObject* args) {       //returns the player's x position
        return Py_BuildValue("i",pos.x);
}
static PyObject* py_selfY(PyObject* pySelf, PyObject* args) {       //returns the player's y position
        return Py_BuildValue("i",pos.y);
}
//Returns the player's X coord on radar (int). -EGG
static PyObject* py_selfRadarX(PyObject* pySelf, PyObject* args) {
    
    if(selfVisible) return Py_BuildValue("i",radar_ptr[0].x);
    return Py_BuildValue("i",-1);
}
//Returns the player's Y coord on radar (int). -EGG
static PyObject* py_selfRadarY(PyObject* pySelf, PyObject* args) {
    if(selfVisible) return Py_BuildValue("i",radar_ptr[0].y);
    return Py_BuildValue("i",-1);
}
int velX() {    //returns the player's x velocity //DO NOT CHANGE, NEEDED IN ORDER FOR selfTrackingRad to work -JRA
        return vel.x;
}
static PyObject* py_selfVelX(PyObject* pySelf, PyObject* args) {    //returns the player's x velocity
        return Py_BuildValue("i",vel.x);
}
int velY() {    //returns the player's y velocity //DO NOT CHANGE, NEEDED IN ORDER FOR selfTrackingRad to work -JRA
        return vel.y;
}
static PyObject* py_selfVelY(PyObject* pySelf, PyObject* args) {    //returns the player's y velocity
        return Py_BuildValue("i",vel.y);
}
int selfSpeed() {   //returns speed of the player's ship //DO NOT CHANGE, NEEDED IN ORDER FOR aimdir &  AIshot_addtobuffer to work -JRA
        return sqrt(pow(vel.x,2)+pow(vel.y,2));
}
static PyObject* py_selfSpeed(PyObject* pySelf, PyObject* args) {   //returns speed of the player's ship
        return Py_BuildValue("d",sqrt(pow(vel.x,2)+pow(vel.y,2)));
}
static PyObject* py_lockHeadingDeg(PyObject* pySelf, PyObject* args) {   //returns the angle at which the player's lock is in relation to the player's ship -JNE
                return Py_BuildValue("d",(double)lock_dir*2.8125);
}
static PyObject* py_lockHeadingRad(PyObject* pySelf, PyObject* args) {   //returns the angle at which the player's lock is in relation to the player's ship -JNE
                return Py_BuildValue("d",(double)lock_dir*.049087);
}
static PyObject* py_selfLockDist(PyObject* pySelf, PyObject* args) {      //returns the distance of the ship the player is locked onto -JNE
        return Py_BuildValue("i",lock_dist); 
}
static PyObject* py_selfReload(PyObject* pySelf, PyObject* args) {    //returns the player's reload time remaining
        return Py_BuildValue("i",reload);
}
//Gets the player's ID, returns an int. -EGG
int selfID() { //DO NOT CHANGE, NEEDED IN ORDER FOR addNewShip to work -JRA
    if (self != NULL)
        return self->id;
    return -1;
}

static PyObject* py_selfID(PyObject* pySelf, PyObject* args) { 
    if (self != NULL)
        return Py_BuildValue("i",self->id);
    return Py_BuildValue("i",-1);
}
//Returns 1 if the player is alive, 0 if they are not. -EGG
static PyObject* py_selfAlive(PyObject* pySelf, PyObject* args) {
    return Py_BuildValue("i",selfVisible);
}
//Returns the player's team (int). -EGG
static PyObject* py_selfTeam(PyObject* pySelf, PyObject* args) {
    if (self != NULL)
        return Py_BuildValue("i",self->team);
    return Py_BuildValue("i",-1);
}
//Returns the player's lives remaining (if there is a life limit) or the number of lives spent (int). -EGG
static PyObject* py_selfLives(PyObject* pySelf, PyObject* args) {
    if (self != NULL)
        return Py_BuildValue("i",self->life);
    return Py_BuildValue("i",-1);
}
double selfHeadingRad() {
	return (double)heading*.049087;
}
double selfTrackingRad() {  //returns the player's tracking in radians	-JNE  //DO NOT CHANGE, NEEDED IN ORDER FOR selfTrackingDeg to work -JRA
	if (vel.y == 0 && vel.x == 0) return selfHeadingRad(); //fix for NaN -EGG -CJG
	double ans = atan((double)velY()/(double)velX());       //calculate tracking
        if (velY() >= 0 && velX() < 0) {        //format the tracking properly
                ans += PI_AI;
        }
        else if (velY() < 0 && velX() < 0) {
                //ans = -ans;
                ans += PI_AI;
        }
        else if (velY() < 0 && velX() >= 0) {
                ans+=2*PI_AI;
        }
        return ans;
}
static PyObject* py_selfTrackingRad(PyObject* pySelf, PyObject* args) {  //returns the player's tracking in radians	-JNE
	if (vel.y == 0 && vel.x == 0) return Py_BuildValue("d",(double)selfHeadingRad()); //fix for NaN -EGG -CJG
	double ans = atan((double)velY()/(double)velX());       //calculate tracking
        if (velY() >= 0 && velX() < 0) {        //format the tracking properly
                ans += PI_AI;
        }
        else if (velY() < 0 && velX() < 0) {
                //ans = -ans;
                ans += PI_AI;
        }
        else if (velY() < 0 && velX() >= 0) {
                ans+=2*PI_AI;
        }
        return Py_BuildValue("d",ans);
}

double selfTrackingDeg() {  //returns the player's tracking in degrees -JNE //DO NOT CHANGE, NEEDED IN ORDER FOR aimdir &  AIshot_addtobuffer to work -JRA
                if (vel.y == 0 && vel.x == 0) return selfHeadingDeg(); //fix for NaN -EGG -CJG
		return (double)selfTrackingRad()*180/PI_AI	;
}
static PyObject* py_selfTrackingDeg(PyObject* pySelf, PyObject* args) {  //returns the player's tracking in degrees -JNE
		if (vel.y == 0 && vel.x == 0) return Py_BuildValue("d",(double)selfHeadingDeg()); //fix for NaN -EGG -CJG   
		return Py_BuildValue("d",(double)selfTrackingRad()*180/PI_AI	);
}
double selfHeadingDeg() {   //returns the player's heading in degrees	-JNE //DO NOT CHANGE, NEEDED IN ORDER FOR turnToDeg to work -JRA
                return (double)heading*2.8125;
}
static PyObject* py_selfHeadingDeg(PyObject* pySelf, PyObject* args) {   //returns the player's heading in degrees	-JNE
                return Py_BuildValue("d",(double)heading*2.8125);
}
static PyObject* py_selfHeadingRad(PyObject* pySelf, PyObject* args) {   //returns the player's heading in radians	-JNE
                return Py_BuildValue("d",(double)heading*.049087);
}
static PyObject* py_hud(PyObject* pySelf, PyObject* args) {         //if the HUD is displaying a name, return it	-JNE
        int i; 
	 if (!PyArg_ParseTuple(args, "i", &i)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
	if ( i < MAX_SCORE_OBJECTS) {
                if (score_objects[i].hud_msg_len>0) {
                        return Py_BuildValue("s",score_objects[i].hud_msg);
                }
        }
        return Py_BuildValue("s","");
}
static PyObject* py_hudScore(PyObject* pySelf, PyObject* args) {        //if the HUD is displaying a score, return it	-JNE
        int i;
	if (!PyArg_ParseTuple(args, "i", &i)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
        }
        if (i < MAX_SCORE_OBJECTS) {
                if (score_objects[i].hud_msg_len>0) {
                        return Py_BuildValue("s",score_objects[i].msg);
                }
        }
        return Py_BuildValue("s","");
}
static PyObject* py_hudTimeLeft(PyObject* pySelf, PyObject* args) {      //returns how much time the HUD will keep displaying a score for, in seconds	-JNE
        int i;
	if (!PyArg_ParseTuple(args, "i", &i)){
		PyErr_SetString(PyExc_TypeError, "invalid parameter");
		return NULL;
	}
	if (i<MAX_SCORE_OBJECTS) {
                if (score_objects[i].hud_msg_len>0) { 
                        return (Py_BuildValue("i",100-score_objects[i].count));
                }
        }
        return Py_BuildValue("i",0);
}
//Gets the player's turnspeed, returns a double. -EGG
static PyObject* py_getTurnSpeed(PyObject* pySelf, PyObject* args) {
    return Py_BuildValue("d",turnspeed);
}
static PyObject* py_getPower(PyObject* pySelf, PyObject* args) {
    return Py_BuildValue("d", power);
}
//Returns 1 if the player's shield is on, 0 if it is not, -1 if player is not alive. -EGG
static PyObject* py_selfShield(PyObject* pySelf, PyObject* args) {
    int i;
    for (i=0;i<num_ship;i++) if ((self != NULL) && (ship_ptr[i].id==self->id)) return Py_BuildValue("i",(int)ship_ptr[i].shield);
    return Py_BuildValue("i",-1);
}
//Returns the player's username (string). -EGG
static PyObject* py_selfName(PyObject* pySelf, PyObject* args) {
    if (self != NULL) return Py_BuildValue("s",self->name);
}
//Returns the player's score (double). -EGG
static PyObject* py_selfScore(PyObject* pySelf, PyObject* args) {
    if (self != NULL) return Py_BuildValue("d",self->score);
}
//End self properties -JNE

static PyObject* py_closestRadarX(PyObject* pySelf, PyObject* args) {       //returns x radar coordinate (0-256) of closest ship on radar
        int i, x;
        double best = -1, l = -1;
        for (i = 1;i < num_radar;i++) {         //go through each enemy
                //if (radar_ptr[i].type == RadarEnemy) {  //skip if they are a friend //In Xpilot classic no way to determine friend or foe. -JRA
                        l = sqrt(pow(radar_ptr[i].x-radar_ptr[0].x,2)+pow(radar_ptr[i].y-radar_ptr[0].y,2));    //get enemy distance
                        if ((l < best) || (best == -1)) {       //if distance is the smallest or this is the first distance measured
                                best = l;                       //update distance
                                x = radar_ptr[i].x;             //update value to be returned
                        }
                //}
        }
        if (best ==-1) //If so, there are no enemies (alive).
                return Py_BuildValue("i",-1);
        return Py_BuildValue("i",x);
}
static PyObject* py_closestRadarY(PyObject* pySelf, PyObject* args) {       //returns y radar coordinate (0-256) of closest ship on radar -JNE
        int i, y;
        double best = -1, l = -1;
        for (i = 1;i < num_radar;i++) {         //go through each ship on radar
                //if (radar_ptr[i].type == RadarEnemy) {  //skip if they are a friend //In Xpilot classic no way to determine friend or foe. -JRA
                        l = sqrt(pow(radar_ptr[i].x-radar_ptr[0].y,2)+pow(radar_ptr[i].y-radar_ptr[0].y,2));    //get enemy distance
                        if (l < best || best == -1) {           //if distance is the smallest or this is the first distance measured
                                best = l;                       //update distance
                                y = radar_ptr[i].y;             //update value to be returned
                        }
                //}
        }
        if (best == -1) //if so, there are no enemies alive -JNE
                return Py_BuildValue("i",-1);
        return Py_BuildValue("i",y);
}
static PyObject* py_closestItemX(PyObject* pySelf, PyObject* args) {                //returns x coordinate of closest item on screen -JNE
        int i, x;
        double best = -1, l = -1;
        for (i=0;i<num_itemtype;i++) {          //go through each item on screen
                l = sqrt(pow(itemtype_ptr[i].x-pos.x,2)+pow(itemtype_ptr[i].y-pos.y,2));        //get distance
                if (l < best || best == -1) {           //if distance is smallest yet
                        best = l;                       //update distance
                        x=itemtype_ptr[i].x;            //update x coordinate
                }
        }
        if (best==-1)   //if so there are no items on screen
                return Py_BuildValue("i",-1);
        return Py_BuildValue("i",x);
}
static PyObject* py_closestItemY(PyObject* pySelf, PyObject* args) {                //returns y coordinate of closest item on screen -JNE
        int i, y;
        double best = -1, l = -1;
        for (i=0;i<num_itemtype;i++) {          //go through each item on screen
                l = sqrt(pow(itemtype_ptr[i].x-pos.x,2)+pow(itemtype_ptr[i].y-pos.y,2));        //get distance
                if (l < best || best == -1) {           //if distance is smallest yet
                        best = l;                       //update distance
                        y=itemtype_ptr[i].y;            //update y coordinate
                }
        }
        if (best==-1)   //if so there are no items on screen
                return Py_BuildValue("i",-1);
        return Py_BuildValue("i",y);
}
//Start wrap helper functions -JNE
//Checks if the map wraps between two x or y coordinates; if it does, it returns a usable value for the first coordinate -JNE
//May glitch if the map is smaller than ext_view_width and height -JNE
int wrapX(int firstX, int selfX) { //DO NOT CHANGE -JRA
	int tempX;
	tempX = firstX - selfX;
	if (abs(tempX)>ext_view_width) {
		if (firstX > selfX) {
			firstX = firstX - Setup->width;
		}
		else {
			firstX = firstX + Setup->width;
		}
	}
	return firstX;
}
int wrapY(int firstY, int selfY) { //DO NOT CHANGE -JRA
	int tempY;
	tempY = firstY - selfY;
	if (abs(tempY)>ext_view_height) {
		if (firstY > selfY) {
			firstY = firstY - Setup->height;
		}
		else {
			firstY = firstY + Setup->height;
		}
	}
	return firstY;
}
//End wrap helper functions -JNE
static PyObject* py_closestShipId(PyObject* pySelf, PyObject* args) {		//returns ID of closest ship on screen -JNE
	int i, d;
	double best = -1, l = -1;
	for (i=0;i<num_ship;i++) {		//go through each ship on screen
		if (ship_ptr[i].x!=pos.x || ship_ptr[i].y!=pos.y){		//make sure ship is not player's ship
			l = sqrt(pow(wrapX(ship_ptr[i].x,pos.x)-pos.x,2)+pow(wrapY(ship_ptr[i].y,pos.y)-pos.y,2));		//distance formula
			if (l < best || best == -1) {		//if distance is smallest yet
				best = l;			//update distance
				d = ship_ptr[i].id;		//update id
			}
		}
	}
	if (best == -1)	//if so there are no ships on screen
		return Py_BuildValue("i",-1);
	return Py_BuildValue("i",d);
}
//End closest functions -JNE
//ID functions -JNE
static PyObject* py_enemySpeedId(PyObject* pySelf, PyObject* args) {	//returns velocity (in pixels per frame) of an enemy with a particular ID -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	int i, j;
	for (i=0;i<num_ship;i++) {	//go through each ship
		if (ship_ptr[i].id==id) {	//find ship with correct id
			for (j=0;j<128;j++) {	//go through [][], look for same ship
				if (allShips[j][0].ship.id==id) {
					if (allShips[j][2].vel != -1) {	//ship is there, push onto array, calculate distance, return distance
						return Py_BuildValue("d",allShips[j][0].vel);
					}
				}
			}
		}
	}
	return Py_BuildValue("d",-1);
}
static PyObject* py_enemyTrackingRadId(PyObject* pySelf, PyObject* args) {	//returns direction of a ship's velocity in radians -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	int i, j;
	shipData_t temp;
	for (i=0;i<num_ship;i++) {
		if (ship_ptr[i].id==id) {
			for (j=0;j<128;j++) {
				if (allShips[j][0].ship.id==id) {
					if (allShips[j][2].vel != -1) { //ship is there, push onto array, calculate distance ,return distance
						return Py_BuildValue("d",allShips[j][0].trackingRad);
					}
				}
			}
		}
	}
    Py_RETURN_NONE;
}
static PyObject* py_enemyTrackingDegId(PyObject* pySelf, PyObject* args) {	//returns direction of a ship's velocity in degrees -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	int i, j;
	shipData_t temp;
	for (i=0;i<num_ship;i++) {
		if (ship_ptr[i].id==id) {
			for (j=0;j<128;j++) {
				if (allShips[j][0].ship.id==id) {
					if (allShips[j][2].vel != -1) { //ship is there, push onto array, calculate distance ,return distance
						return Py_BuildValue("d",allShips[j][0].trackingDeg);
					}
				}
			}
		}
	}
    Py_RETURN_NONE;
}
static PyObject* py_enemyReloadId(PyObject* pySelf, PyObject* args) {
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	int i, j;
	for (i=0;i<num_ship;i++) {
		if (ship_ptr[i].id==id) {
			for (j=0;j<128;j++) {
				if (allShips[j][0].ship.id==id) {
					if (allShips[j][2].vel != -1) { 
						return Py_BuildValue("i",allShips[j][0].reload);
					}
				}
			}
		}
	}
    Py_RETURN_NONE;
}
static PyObject* py_screenEnemyXId(PyObject* pySelf, PyObject* args) {	//returns x coordinate of closest ship on screen -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	int i;
	for (i = 0;i < num_ship;i++) {		//go through each ship on screen
		if (ship_ptr[i].id==id) {
			return Py_BuildValue("i",ship_ptr[i].x);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_screenEnemyYId(PyObject* pySelf, PyObject* args) {	//returns y coordinate of closest ship on screen -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	int i;
	for (i = 0;i < num_ship;i++) {		//go through each ship on screen
		if (ship_ptr[i].id==id) {
			return Py_BuildValue("i",ship_ptr[i].y);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_enemyHeadingDegId(PyObject* pySelf, PyObject* args) {  //returns the heading of ship with a particular ID in degrees -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    for (i=0;i<num_ship;i++) {
        if (ship_ptr[i].id==id) {
            return Py_BuildValue("d",(double)ship_ptr[i].dir*2.8125);	//convert from 0-127 scale to 0-360 scale
        }
    }
    return Py_BuildValue("d",-1);              //program reaches here if there is not a ship on screen with that id
}
static PyObject* py_enemyHeadingRadId(PyObject* pySelf, PyObject* args) {  //returns the heading of ship with a particular ID in radians -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    for (i=0;i<num_ship;i++) {
        if (ship_ptr[i].id==id) {
            return Py_BuildValue("d",(ship_ptr[i].dir*.049087)); //convert from 0-127 scale to 0-2pi scale
        }
    }
    return Py_BuildValue("d",-1);
}
static PyObject* py_enemyShieldId(PyObject* pySelf, PyObject* args) {         //return 1 if the enemy has a shield up, 0 if it does not -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    for (i=0;i<num_ship;i++) {              //check all ships on screen for that id
        if (ship_ptr[i].x!=pos.x || ship_ptr[i].y!=pos.y){              //make sure ship is not player's ship
            if (ship_ptr[i].id==id) {
                return Py_BuildValue("i",(int)ship_ptr[i].shield);         //0 if no shield, 1 if shield
            }
        }
    }
    return Py_BuildValue("i",-1);              //program reaches here if there is not a ship on screen with that id 
}
static PyObject* py_enemyLivesId(PyObject* pySelf, PyObject* args) {          //return the number of lives for enemy with a particular id	-JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    for (i=0;i<num_others;i++) {    //look for the other with the id that was passed in
        if (Others[i].id==id) {
            return Py_BuildValue("i",(int)Others[i].life);
        }
    }
    return Py_BuildValue("i",-1);              //return -1 if the id is invalid
}
static PyObject* py_enemyNameId(PyObject* pySelf, PyObject* args) {       //return the name of the enemy with the corresponding id	-JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    for (i=0;i<num_others;i++) {    //look for the other with the id that was passed in
        if (Others[i].id==id) {
            return Py_BuildValue("s",Others[i].name);
        }
    }
    return Py_BuildValue("s",NULL);
}
static PyObject* py_enemyScoreId(PyObject* pySelf, PyObject* args) {        //returns the overall score of a ship with a particular ID	-JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    for (i=0;i<num_others;i++) {
        if (Others[i].id==id) {
            return Py_BuildValue("d",Others[i].score);
        }
    }
    return Py_BuildValue("d",-1);
}
static PyObject* py_enemyTeamId(PyObject* pySelf, PyObject* args) {           //returns team of a ship with a particular ID, or -1 if the map is not a team play map -JNE
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
    if (num_playing_teams == 0) {
        return Py_BuildValue("i",-1);
    }
    else {
        for (i=0;i<num_others;i++) {
            if (Others[i].id==id) {
                return Py_BuildValue("i",Others[i].team);
            }
        }
    }
    return Py_BuildValue("i",-1);
}
static PyObject* py_enemyDistanceId(PyObject* pySelf, PyObject* args) {		//returns the distance of a ship with a particular ID
    int id;
    if (!PyArg_ParseTuple(args, "i", &id)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
	for (i=0;i<128;i++) {
		if (allShips[i][0].ship.id == id) {
			return Py_BuildValue("d",allShips[i][0].d);
		}
	}
	return Py_BuildValue("d",-1);
}
//End ID functions -JNE
//Begin idx functions! -JNE
static PyObject* py_enemyDistance(PyObject* pySelf, PyObject* args) {	//returns the distance of a ship with a particular index -JNE
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	return Py_BuildValue("d",allShips[idx][0].d);
}
static PyObject* py_enemySpeed(PyObject* pySelf, PyObject* args) {	//returns velocity of a ship with a particular index -JNE
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("d",allShips[idx][0].vel);
}
static PyObject* py_enemyReload(PyObject* pySelf, PyObject* args) {	//returns velocity of a ship with a particular index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",allShips[idx][0].reload);
}
static PyObject* py_enemyTrackingRad(PyObject* pySelf, PyObject* args) {	//returns tracking based on index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("d",allShips[idx][0].trackingRad);
}
static PyObject* py_enemyTrackingDeg(PyObject* pySelf, PyObject* args) {	//returns tracking based on index -JNE
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("d",allShips[idx][0].trackingDeg);
}
static PyObject* py_screenEnemyX(PyObject* pySelf, PyObject* args) {		//returns x coordinate of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",allShips[idx][0].ship.x);
}
static PyObject* py_screenEnemyY(PyObject* pySelf, PyObject* args) {		//returns y coordinate of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",allShips[idx][0].ship.y);
}
static PyObject* py_enemyHeadingDeg(PyObject* pySelf, PyObject* args) {		//returns heading in degrees of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("d",allShips[idx][0].ship.dir*2.8125);
}
static PyObject* py_enemyHeadingRad(PyObject* pySelf, PyObject* args) {		//returns heading in radians of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("d",allShips[idx][0].ship.dir*.049087);
}
static PyObject* py_enemyShield(PyObject* pySelf, PyObject* args) {		//returns shield status of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",allShips[idx][0].ship.shield);
}
static PyObject* py_enemyLives(PyObject* pySelf, PyObject* args) {		//returns lives of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
	for (i=0;i<num_others;i++) {
		if (Others[i].id == allShips[idx][0].ship.id) {
			return Py_BuildValue("i",Others[i].life);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_enemyTeam(PyObject* pySelf, PyObject* args) {	//returns team of enemy at an index -JNE
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
	for (i=0;i<num_others;i++) {
		if (Others[i].id == allShips[idx][0].ship.id) {
			return Py_BuildValue("i",Others[i].team);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_enemyName(PyObject* pySelf, PyObject* args) {		//returns name of enemy at an index -JNE
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
	for (i=0;i<num_others;i++) {
		if (Others[i].id == allShips[idx][0].ship.id) {
			return Py_BuildValue("s",Others[i].name);
		}
	}
	return Py_BuildValue("s","");
}
static PyObject* py_enemyScore(PyObject* pySelf, PyObject* args) {		//returns score of enemy at an index -JNE
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i;
	for (i=0;i<num_others;i++) {
		if (Others[i].id == allShips[idx][0].ship.id) {
			return Py_BuildValue("d",Others[i].score);
		}
	}
    Py_RETURN_NONE;
}
//End idx functions. -JNE
static PyObject* py_degToRad(PyObject* pySelf, PyObject* args) {
    int deg;
    if (!PyArg_ParseTuple(args, "i", &deg)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("d",(deg * (PI_AI / 180.0)));
}
//Converts radians (double) to degrees (int). -EGG
static PyObject* py_radToDeg(PyObject* pySelf, PyObject* args) {
    double rad;
    if (!PyArg_ParseTuple(args, "d", &rad)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",(int)(rad * (180.0 / PI_AI)));
}
//Returns the smallest angle which angle1 could add to itself to be equal to angle2. -EGG
//This is useful for turning particular directions. -EGG
static PyObject* py_angleDiff(PyObject* pySelf, PyObject* args) {
    int angle1;
    int angle2;
    if (!PyArg_ParseTuple(args, "ii", &angle1, &angle2)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int difference = angle2 - angle1;
    while (difference < -180) difference += 360;
    while (difference > 180) difference -= 360;
    return Py_BuildValue("i",difference);
}
//Returns the result of adding two angles together. -EGG
static PyObject* py_angleAdd(PyObject* pySelf, PyObject* args) {
    int angle1;
    int angle2;
    if (!PyArg_ParseTuple(args, "ii", &angle1, &angle2)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",(angle1 + angle2) % 360);
}
//Wall_here -EGG
//Parameters: x, y, flag to draw wall feelers, flag to draw wall detection. -EGG
//Returns 1 if there is a wall at the given x,y or 0 if not. -EGG
//Removed detection drawing flags -CJG
int Wall_here(int x_here, int y_here) { //DO NOT CHANGE -JRA
    int			xi, yi, xb, yb;
    int			rxb, ryb;
    int			x, y;
    int			type;
    unsigned char	*mapptr, *mapbase;
    //if (!oldServer) {
	//printf("This is a 'new server'. Wall detection will not work here.\n");
       // return;
    //}
    xb = ((x_here < 0) ? (x_here - (BLOCK_SZ - 1)) : x_here) / BLOCK_SZ;
    yb = ((y_here < 0) ? (y_here - (BLOCK_SZ - 1)) : y_here) / BLOCK_SZ;
    if (!BIT(Setup->mode, WRAP_PLAY)) {
	if (xb < 0)
	    xb = 0;
	if (yb < 0)
	    yb = 0;
	//if (xe >= Setup->x)
	  //  xe = Setup->x - 1;
	//if (ye >= Setup->y)
	//    ye = Setup->y - 1;
    }
    y = yb * BLOCK_SZ;
    yi = mod(yb, Setup->y);
    mapbase = Setup->map_data + yi;
    ryb = yb;
    if (yi == Setup->y) {
	yi = 0;
	mapbase = Setup->map_data;
    }
    x = xb * BLOCK_SZ;
    xi = mod(xb, Setup->x);
    mapptr = mapbase + xi * Setup->y;
    rxb = xb;
    if (xi == Setup->x) {
        xi = 0;
        mapptr = mapbase;
    }
    type = *mapptr;
    if (type & BLUE_BIT)return 1;
    return 0;
}
//wallFeeler! -EGG
//Parameters: distance of line to 'feel', angle in degrees, flag to draw wall feelers, flag to draw wall detection. -EGG
//Returns 1 if there is a wall from the player's ship at the given angle and distance or 0 if not. -EGG
static PyObject* py_wallFeeler(PyObject* pySelf, PyObject* args) { //removed flags -CJG
    int dist;
    int angle;
    if (!PyArg_ParseTuple(args, "ii", &dist, &angle)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    double a = angle * PI_AI / 180.0;
    int x = pos.x + cos(a)*dist;
    int y = pos.y + sin(a)*dist;
    int ret = wallBetween(pos.x, pos.y, x, y);
    if (ret == -1) return Py_BuildValue("i",dist); //Returns the distance of the feeler if no wall is felt - JTO
    return Py_BuildValue("i",ret);
}
//wallFeeler that uses radians! -EGG
static PyObject* py_wallFeelerRad(PyObject* pySelf, PyObject* args) { //Removed flags -CJG
    int dist; 
    double a; 
    if (!PyArg_ParseTuple(args, "id", &dist, &a)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int x = pos.x + cos(a)*dist;
    int y = pos.y + sin(a)*dist;
    int ret =  wallBetween(pos.x, pos.y, x, y);
    if (ret == -1) return  Py_BuildValue("i",dist); //Returns the distance of the feeler if no wall is felt - JTO
    return  Py_BuildValue("i",ret);
}
//Detects if there is a wall between two points, returns 0 or 1 -EGG
//Utilizes Bresenham's line-drawing algorithm (no multiplication or division!) -EGG
//Adopted from http://www.brackeen.com/vga/source/djgpp20/lines.c.html (THANK YOU!) -EGG
//Parameters: x1, y1, x2, y2, flag to draw wall feelers, flag to draw wall detection. -EGG
//Returns distance between the first point and the wall if there is a wall between the two points or -1 if not. -EGG
//Removed detection drawing flags -CJG
int wallBetween(int x1, int y1, int x2, int y2) { //DO NOT CHANGE, NEEDED IN ORDER FOR WallFeeler & WallFeelerRad to work -JRA
    int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py,ret;
    dx=x2-x1;      /* the horizontal distance of the line */
    dy=y2-y1;      /* the vertical distance of the line */
    dxabs=abs(dx);
    dyabs=abs(dy);
    sdx=sgn(dx);
    sdy=sgn(dy);
    x=dyabs>>1;
    y=dxabs>>1;
    px=x1;
    py=y1;
    if (dxabs>=dyabs) /* the line is more horizontal than vertical */{
        for(i=0;i<dxabs;i++){
            y+=dyabs;
        if (y>=dxabs){
        y-=dxabs;
        py+=sdy;
        }
        px+=sdx;
        ret = Wall_here(px, py);
        if (ret) return sqrt(pow((px-x1),2)+pow((py-y1),2));
        }
    }
    else /* the line is more vertical than horizontal */{
        for(i=0;i<dyabs;i++){
            x+=dxabs;
            if (x>=dyabs){
            x-=dyabs;
            px+=sdx;
            }
        py+=sdy;
        ret = Wall_here(px, py);
        if (ret) return sqrt(pow((px-x1),2)+pow((py-y1),2));
        }
    }
    return -1;
}
static PyObject* py_wallBetween(PyObject* pySelf, PyObject* args) {
    int x1;
    int y1;
    int x2;
    int y2;
    if (!PyArg_ParseTuple(args, "iiii", &x1, &y1, &x2, &y2)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py,ret;
    dx=x2-x1;      /* the horizontal distance of the line */
    dy=y2-y1;      /* the vertical distance of the line */
    dxabs=abs(dx);
    dyabs=abs(dy);
    sdx=sgn(dx);
    sdy=sgn(dy);
    x=dyabs>>1;
    y=dxabs>>1;
    px=x1;
    py=y1;
    if (dxabs>=dyabs) /* the line is more horizontal than vertical */{
        for(i=0;i<dxabs;i++){
            y+=dyabs;
        if (y>=dxabs){
        y-=dxabs;
        py+=sdy;
        }
        px+=sdx;
        ret = Wall_here(px, py);
        if (ret) return Py_BuildValue("d",sqrt(pow((px-x1),2)+pow((py-y1),2)));
        }
    }
    else /* the line is more vertical than horizontal */{
        for(i=0;i<dyabs;i++){
            x+=dxabs;
            if (x>=dyabs){
            x-=dyabs;
            px+=sdx;
            }
        py+=sdy;
        ret = Wall_here(px, py);
        if (ret) return Py_BuildValue("d",sqrt(pow((px-x1),2)+pow((py-y1),2)));
        }
    }
    return Py_BuildValue("i",-1);
}
//Shot functions
static PyObject* py_shotAlert(PyObject* pySelf, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
	return Py_BuildValue("i",AIshot_buffer[idx].alert);
}
static PyObject* py_shotX(PyObject* pySelf, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",AIshot_buffer[idx].x);
}
static PyObject* py_shotY(PyObject* pySelf, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",AIshot_buffer[idx].y);
}
static PyObject* py_shotDist(PyObject* pySelf, PyObject* args) {
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",AIshot_buffer[idx].dist);
}
static PyObject* py_shotVel(PyObject* pySelf, PyObject* args) {
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",AIshot_buffer[idx].vel);
}
static PyObject* py_shotVelDir(PyObject* pySelf, PyObject* args) {
	int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    return Py_BuildValue("i",AIshot_buffer[idx].veldir);
}
//Calculates the direction that self would need to point to hit the ship idx (ported from old code) -EGG
static PyObject* py_aimdir(PyObject* pySelf, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)){
        PyErr_SetString(PyExc_TypeError, "invalid parameter");
        return NULL;
    }
    if (allShips[idx][0].vel==-1) return Py_BuildValue("i",-1);
    float Bx, By, Bvel, Cx, Cy, Svx, Svy, Sx, Sy, forgo, tugo, mugo;
    float degs1, degs2, time1, time2, Bvx;
    Bx = (float) pos.x;
    By = (float) pos.y;
    Bvel = (float) AI_shotspeed;
    Cx = cos(AI_radian(selfTrackingDeg())) * selfSpeed();
    Cy = sin(AI_radian(selfTrackingDeg())) * selfSpeed();
    Svx = cos(AI_radian(allShips[idx][0].trackingDeg)) * allShips[idx][0].vel;
    Svy = sin(AI_radian(allShips[idx][0].trackingDeg)) * allShips[idx][0].vel;
    Sx = allShips[idx][0].ship.x;
    Sy = allShips[idx][0].ship.y;
    tugo = pow(pow(Bvel * Sx - Bvel * Bx, 2) + pow(Bvel * By - Bvel * Sy, 2), 0.5);
    mugo = AI_degree(acos((Bvel * Sx - Bvel * Bx) / tugo));
    forgo = AI_degree(asin
		  ((By * Svx - Bx * Svy + Bx * Cy - By * Cx + Sx * Svy -
		    Sy * Svx - Cy * Sx + Sy * Cx) / tugo));
    degs1 = fabs(forgo + mugo);
    degs2 = fabs(forgo - mugo);
    Bvx = cos(AI_radian(degs1)) * Bvel + cos(AI_radian(selfTrackingDeg())) * selfSpeed();
    time1 = (Bx - Sx) / (Svx - Bvx);
    Bvx = cos(AI_radian(degs2)) * Bvel + cos(AI_radian(selfTrackingDeg())) * selfSpeed();
    time2 = (Bx - Sx) / (Svx - Bvx);
    /*It's because those asin and acos that I must do all the below, because they return values that may or may not be in the correct quadrant.  Someone must figure out how to tell what quadrant they should be in */
    if (time1 < 0 && time2 < 0) return Py_BuildValue("i",-1);
    else if (time1 < 0) {
	if ((Sy + Svy * time2) <= (pos.y + sin(AI_radian(selfTrackingDeg())) * selfSpeed() * time2)) return Py_BuildValue("i",360 - (int)degs2);
	else return Py_BuildValue("i",-1);
    }
    else if (time2 < 0) {
	if ((Sy + Svy * time1) >= (pos.y + sin(AI_radian(selfTrackingDeg())) * selfSpeed() * time1)) return Py_BuildValue("i",(int)degs1);
	else return Py_BuildValue("i",-1);
    }
    else {
	if ((Sy + Svy * time1) >= (pos.y + sin(AI_radian(selfTrackingDeg())) * selfSpeed() * time1)) return Py_BuildValue("i",(int)degs1);
	else if ((Sy + Svy * time2) < (pos.y + sin(AI_radian(selfTrackingDeg())) * selfSpeed() * time2)) return Py_BuildValue("i",360 - (int)degs2);
	else return Py_BuildValue("i",(int)degs2);
    }
    return Py_BuildValue("i",-1);
}
//Capture the flag functions - Sarah Penrose
static PyObject* py_ballX(PyObject* pySelf, PyObject* args) {
	int i;
	for (i=0;i<num_ball;i++) {
		if (ball_ptr[i].x != -1) {
			return Py_BuildValue("i",ball_ptr[i].x);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_ballY(PyObject* pySelf, PyObject* args) {
	int i;
	for (i=0;i<num_ball;i++) {
		if (ball_ptr[i].x != -1) {
			return Py_BuildValue("i",ball_ptr[i].y);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_connectorX0(PyObject* pySelf, PyObject* args) {
	int i;
	for (i=0;i<num_connector;i++) {
		if (connector_ptr[i].x0 != -1) {
			return Py_BuildValue("i",connector_ptr[i].x0);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_connectorX1(PyObject* pySelf, PyObject* args) {
	int i;
	for (i=0;i<num_connector;i++) {	
		if (connector_ptr[i].x1 != -1) {
			return Py_BuildValue("i",connector_ptr[i].x1);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_connectorY0(PyObject* pySelf, PyObject* args) {
	int i;
	for (i=0;i<num_connector;i++) {
		if (connector_ptr[i].y0 != -1) {
			return Py_BuildValue("i",connector_ptr[i].y0);
		}
	}
	return Py_BuildValue("i",-1);
}
static PyObject* py_connectorY1(PyObject* pySelf, PyObject* args) {
	int i;
	for (i=0;i<num_connector;i++) {
		if (connector_ptr[i].y1 != -1) {
			return Py_BuildValue("i",connector_ptr[i].y1);
		}
	}	
	return Py_BuildValue("i",-1);
}
//Methods to help AI loop -JNE
void calcStuff(int j) {			//updates data in allShips for velocity and tracking in degrees and radians -JNE
	allShips[j][0].d = sqrt(pow(wrapX(allShips[j][0].ship.x,pos.x)-pos.x,2)+pow(wrapX(allShips[j][0].ship.y,pos.y)-pos.y,2));
	allShips[j][0].vel = sqrt(pow(wrapX(allShips[j][0].ship.x,allShips[j][2].ship.x)-allShips[j][2].ship.x,2)+pow(wrapY(allShips[j][0].ship.y,allShips[j][2].ship.y)-allShips[j][2].ship.y,2))/2;		//calculate velocity
	allShips[j][0].xVel = wrapX(allShips[j][0].ship.x,allShips[j][2].ship.x)-allShips[j][2].ship.x;	//calculate x velocity
	allShips[j][0].yVel = wrapY(allShips[j][0].ship.y,allShips[j][2].ship.y)-allShips[j][2].ship.y;	//calculate y velocity
	allShips[j][0].trackingRad = atan(allShips[j][0].yVel/allShips[j][0].xVel);	//calculate tracking
	if (allShips[j][0].xVel >= 0 && allShips[j][0].yVel < 0) {	//re-format tracking
		allShips[j][0].trackingRad += 2*3.141592653589793;
	}
	else if (allShips[j][0].xVel < 0 && allShips[j][0].yVel < 0) {
		allShips[j][0].trackingRad += 3.141592653589793;
	}
	else if (allShips[j][0].xVel < 0 && allShips[j][0].yVel >= 0) {
		allShips[j][0].trackingRad = 3.141592653589793+allShips[j][0].trackingRad;
	}
	allShips[j][0].trackingDeg = allShips[j][0].trackingRad*180/3.141592653589793;
}
void updateSlots() {	//moves everything in allShips over by a frame -JNE
	int i;
	ship_t theShip;
	theShip.x=-1;
	theShip.y=-1;
	theShip.dir=-1;
	theShip.shield=-1;
	theShip.id=-1;
	for (i=0;i<128;i++) {			//check every slot in allShips
		if (allShips[i][0].vel!=-1 || allShips[i][1].vel!=-1 || allShips[i][2].vel!=-1) {	//only update slots that were updated in the last three frames
			allShips[i][2] = allShips[i][1];	//bump the last two down one
			allShips[i][1] = allShips[i][0];
			allShips[i][0].vel = -1;		//this is updated later if the ship is still on screen
			allShips[i][0].d = 9999;
			allShips[i][0].xVel=-1;
			allShips[i][0].yVel=-1;
			allShips[i][0].trackingDeg=-1;
			allShips[i][0].trackingRad=-1;
                        if (allShips[i][1].reload > 0) allShips[i][0].reload=allShips[i][1].reload-1; //reload tracking -EGG
			else if (allShips[i][1].vel!=-1) allShips[i][0].reload=0;
			else allShips[i][0].reload=-1;
			allShips[i][0].ship=theShip;
		}
	}
}
int updateFirstOpen() {	//goes through allShips, returning the index of the first open spot -JNE
	int i;
	for (i=0;i<128;i++) {
		if (allShips[i][0].vel==-1 && allShips[i][1].vel==-1 && allShips[i][2].vel==-1) {
			return i;
		}
	}
	return -1;
}
bool updateShip(int i) { //goes through allShips and checks if a particular ship is there, returning true if it is and false if it isn't -JNE
        int j;
        for (j=0;j<128;j++) {
                if (ship_ptr[i].id==allShips[j][1].ship.id) {   //find the spot where the ship's ID is located
                        allShips[j][0].ship = ship_ptr[i];
                        if (allShips[j][2].vel >= 0) {
                                calcStuff(j);
                        }
                        else {
                                allShips[j][0].vel = 0;
                        }
                        return true;            //ship was found, so don't add it as a new ship
                }
        }
        return false;
}
void addNewShip(int firstOpen, int i) { //add a ship that has never been on screen before -JNE
        if (selfID() != ship_ptr[i].id) {
                if (updateShip(i)==false) {
                        allShips[firstOpen][0].ship = ship_ptr[i];
                        allShips[firstOpen][0].vel = 0;
                }
        }
}
int sortShips() {	//sorts the ships in the allShips buffer by how far away they are from the player -JNE
//See our previous quicksort thanks ;)
	#define  MAX_LEVELS  1000
	shipData_t piv;
	int  beg[MAX_LEVELS], end[MAX_LEVELS], i=0, L, R ;
	beg[0]=0; end[0]=128;
	while (i>=0) {
		L=beg[i]; R=end[i]-1;
		if (L<R) {
			piv=allShips[L][0]; if (i==MAX_LEVELS-1) return -1;
			while (L<R) {
				while (allShips[R][0].d>=piv.d && L<R) R--;
				if (L<R) {
					allShips[L++][0]=allShips[R][0];
					allShips[L][1]=allShips[R][1];
					allShips[L][2]=allShips[R][2];
				}
				while (allShips[L][0].d<=piv.d && L<R) L++;
				if (L<R) {
					allShips[R--][0]=allShips[L][0];
					allShips[R][1]=allShips[L][1];
					allShips[R][2]=allShips[L][2];
				}
			}
			allShips[L][0]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L; }
		else {
			i--;
		}
	}
   return 1;
}
//update ships' velocity and tracking
void prepareShips() {
    updateSlots();          //move all the ship data one slot (or frame) to the right -JNE
    int firstOpen;
    firstOpen = 0;
    int i;
    for (i=0;i<num_ship;i++) {      //go through each ship on screen, updating their position and adding them if they are not there -JNE
        firstOpen = updateFirstOpen();
        addNewShip(firstOpen, i);
    }
    sortShips();
    if (reload > 0) reload--;
}
//End of methods to help AI_loop -JNE
//THE L00PZ -EGG
static PyObject* py_switchLoop(PyObject* pySelf, PyObject* args) {
    PyObject *temp;
    if (PyArg_ParseTuple(args, "O:set_callback", &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }
        Py_XINCREF(temp);         /* Add a reference to new callback */
        Py_XDECREF(py_loop);  /* Dispose of previous callback */
        py_loop = temp;       /* Remember new callback */
    }
    Py_RETURN_NONE;
}
void AI_loop() {
    if (PyCallable_Check(py_loop)) PyObject_CallObject(py_loop,NULL);
}
//END L00PZ -EGG
//Inject our loop -EGG
void injectAI() { 
    if (AI_delaystart > 2) { 
        prepareShips();
        prepareShots();
        AI_loop();
    }
    else {
        if (AI_delaystart == -5) Net_talk("/get shotspeed");
        else if (AI_delaystart == -4) Net_talk("/get firerepeatrate");
        else if (AI_delaystart < 2) {
             sscanf(TalkMsg[0]->txt, "The value of firerepeatrate is %d", &AI_repeatrate);
             sscanf(TalkMsg[1]->txt, "The value of shotspeed is %d", &AI_shotspeed);
        }
        if ((AI_shotspeed != -1 && AI_repeatrate != -1) || AI_delaystart < 1) AI_delaystart++;
    }
}
//END inject -EGG
//Run xpilot without a window -EGG translate -CJG
static PyObject* py_headlessMode(PyObject* pySelf, PyObject* args) {
	headless=1;
	Py_RETURN_NONE;}
//Oh glorious py_main(), with the regular main(), you just start the Python shell. -.-; -EGG
static PyObject* py_start(PyObject* pySelf, PyObject* args) {
    int j,k;
    ship_t theShip;
    theShip.x=-1;
    theShip.y=-1;
    theShip.dir=-1;
    theShip.shield=-1;
    theShip.id=-1;
    for (j=0;j<128;j++) {	//Initialize allShips for enemy velocity
        for (k=0;k<3;k++) {
            allShips[j][k].vel=-1;
            allShips[j][k].d=9999;		//needs to be arbitrarily high so that it is sorted correctly in allShips
            allShips[j][k].ship = theShip;
            allShips[j][k].xVel=-1;
            allShips[j][k].yVel=-1;
            allShips[j][k].trackingDeg=-1;
            allShips[j][k].trackingRad=-1;
            allShips[j][k].reload=-1;
        }
    }
    AI_delaystart = -5;
    AIshot_reset();
    AIshot_toggle = 1;
    AI_shotspeed = -1;
    AI_repeatrate = -1;
    AI_alerttimemult = 5;
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~\nAI INTERFACE INITIALIZED\n~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    //Parse python arguments
    int i;
    PyObject* listObj = PyList_New(0);
    PyObject* strObj;
    PyObject* temp;
    if (!PyArg_ParseTuple(args, "O|O", &temp, &listObj))
        return Py_BuildValue("s","INVALID ARGUMENTS!\n");
    int argc = PyList_Size(listObj)+1;
    char* argv[argc];
    argv[0] = "xpilot-ng-x11";
    for (i=0; i<argc-1; i++) {
        strObj = PyList_GetItem(listObj, i);
        strObj = PyUnicode_AsEncodedString(strObj,"utf-8","strict");
        argv[i+1] = PyBytes_AS_STRING(strObj);
    }
    //Set AI loop
    args = Py_BuildValue("(O)",temp);
    py_switchLoop(pySelf, args);
    return Py_BuildValue("i",mainAI(argc,argv));
}
//Python/C method definitions -EGG
static PyMethodDef libpyAI_methods[] = {
    {"start",py_start,METH_VARARGS,"Initialize AI interface and start XPilot"},
    {"switchLoop",py_switchLoop,METH_VARARGS,"Switch the method that AI_loop calls"},
    {"headlessMode", py_headlessMode,METH_NOARGS, "Allows XPilot client to run without opening a window"}, //-CJG
    //Movement methods -JRA
    {"turnLeft",py_turnLeft,METH_VARARGS,"Turns left"},
    {"turnRight",py_turnRight,METH_VARARGS,"Turns right"},
    {"turn",py_turn,METH_VARARGS,"Turns to an inputed degree"},
    {"turnToDeg",py_turnToDeg,METH_VARARGS,"Turns the ship to the inputed Speed of Degree"},
    {"thrust",py_thrust,METH_VARARGS,"Thrust the ship"},
    {"setTurnSpeed",py_setTurnSpeed,METH_VARARGS,"Sets the speed the ship will turn by, the minimum power level is 4.0 and the maximum power is 64.0"},
    {"setTurnSpeedDeg",py_setTurnSpeedDeg,METH_VARARGS,"Sets turn speed in degrees"}, //-CJG
    {"setPower",py_setPower,METH_VARARGS,"Sets the speed the ship will thrust by, the minimum power level is 5.0 and the maximum power is 55.0"},
    {"fasterTurnrate",py_fasterTurnrate,METH_NOARGS,"Increases the ship's Turn Rate"},
    {"slowerTurnrate",py_slowerTurnrate,METH_NOARGS,"Decreases the ship's Turn Rate"},
    {"morePower",py_morePower,METH_NOARGS,"Increases the ship's Thrusting Power"},
    {"lessPower",py_lessPower,METH_NOARGS,"Decreases the ship's Thrusting Power"},
    //Shooting methods -JRA
    //Following commands have not been tested, due to lack of items on map -JRA
    {"fireShot",py_fireShot,METH_NOARGS,"Fires a Shot"},
    {"fireMissile",py_fireMissile,METH_NOARGS,"Fires a Missile"},
    {"fireTorpedo",py_fireTorpedo,METH_NOARGS,"Fires a Torpedo"},
    {"fireHeat",py_fireHeat,METH_NOARGS,"Fires a Heat Seeking Missile"},
    {"dropMine",py_dropMine,METH_NOARGS,"Drops a Stationary Mine from the ship"},
    {"detachMine",py_detachMine,METH_NOARGS,"Releases a Mine from the ship"},
    {"detonateMines",py_detonateMines,METH_NOARGS,"Detonates released Mines"},
    {"fireLaser",py_fireLaser,METH_NOARGS,"Fires a Laser"},
    //Item usage methods -JRA
    {"tankDetach",py_tankDetach,METH_NOARGS,"Detaches a fuel tank from the ship"},
    {"cloak",py_cloak,METH_NOARGS,"Cloaks the ship"},
    {"ecm",py_ecm,METH_NOARGS,"Launches an ECM to temporarily blind opponents"},
    {"transporter",py_transporter,METH_NOARGS,"Uses the transporter item to steal an opponent's item or fuel"},
    {"tractorBeam",py_tractorBeam,METH_VARARGS,"Uses the ship's Tractor Beam to pull in enemy ships"},
    {"pressorBeam",py_pressorBeam,METH_VARARGS,"Uses the ship's Pressor Beam to push away enemy ships"},
    {"phasing",py_phasing,METH_NOARGS,"Uses the Phasing item to allow the ship to pass through walls"},
    {"shield",py_shield,METH_NOARGS,"Turns on or off the ship's Shield"},
    {"emergencyShield",py_emergencyShield,METH_NOARGS,"Uses the Emergency Shield item to protect your ship from damage for a period of time"},
    {"hyperjump",py_hyperjump,METH_NOARGS,"Uses the Hyper Jump item to warp the ship to a random location"},
    {"nextTank",py_nextTank,METH_NOARGS,"Switches to the ship's next fuel tank"},
    {"prevTank",py_prevTank,METH_NOARGS,"Switches to the ship's previous fuel tank"},
    {"toggleAutopilot",py_toggleAutopilot,METH_NOARGS,"Uses the Autopilot item to stop the ship's movement"},
    {"emergencyThrust",py_emergencyThrust,METH_NOARGS,"Uses the Emergency Thrust item to increase the ship's movement speed for a period of time"},
    {"deflector",py_deflector,METH_NOARGS,"Uses the deflector item to push everything away from the ship"},
    {"selectItem",py_selectItem,METH_NOARGS,"Selects the ships item to be dropped"},
    {"loseItem",py_loseItem,METH_NOARGS,"Drops the ships selected item"},
    //Lock methods -JRA
    {"lockNext",py_lockNext,METH_NOARGS,"Locks onto the next ship in the ship buffer"},
    {"lockPrev",py_lockPrev,METH_NOARGS,"Locks onto the prev ship in the ship buffer"},
    {"lockClose",py_lockClose,METH_NOARGS,"Locks onto the closest ship"},
    {"lockNextClose",py_lockNextClose,METH_NOARGS,"Locks-on to the next closest ship"},
    {"loadLock1",py_loadLock1,METH_NOARGS,"Load a saved lock-on enemy ship"},
    {"loadLock2",py_loadLock2,METH_NOARGS,"Load a saved lock-on enemy ship"},
    {"loadLock3",py_loadLock3,METH_NOARGS,"Load a saved lock-on enemy ship"},
    {"loadLock4",py_loadLock4,METH_NOARGS,"Load a saved lock-on enemy ship"},
    //Modifier methods -JRA
    {"toggleNuclear",py_toggleNuclear,METH_NOARGS,"Toggles the option to have the ship fire Nuclear weapons instead of regualar weapons, takes up five mines or seven missile"},
    {"togglePower",py_togglePower,METH_NOARGS,"Toggles the Power of the weapon"},
    {"toggleVelocity",py_toggleVelocity,METH_NOARGS,"Modifies explosion velocity of mines and missiles"},
    {"toggleCluster",py_toggleCluster,METH_NOARGS,"Toggles the option to have the ship fire Cluster weapons instead of regular weapons"},
    {"toggleMini",py_toggleMini,METH_NOARGS,"Modifies explosion velocity of mines and missiles"},
    {"toggleSpread",py_toggleSpread,METH_NOARGS,"Toggles the option to have the ship fire Spread weapons instead of regular weapons"},
    {"toggleLaser",py_toggleLaser,METH_NOARGS,"Toggles between the LS stun laser and the LB blinding laser"},
    {"toggleImplosion",py_toggleImplosion,METH_NOARGS,"Toggle the option to have mines and missiles implode instead of exlode, the explosion will draw in players instead of blowing them away"},
    {"toggleUserName",py_toggleUserName,METH_NOARGS,"Toggle the option to have mines and missiles implode instead of exlode, the explosion will draw in players instead of blowing them away"},
    {"loadModifiers1",py_loadModifiers1,METH_NOARGS,"Loads Modifiers"},
    {"loadModifiers2",py_loadModifiers2,METH_NOARGS,"Loads Modifiers"},
    {"loadModifiers3",py_loadModifiers3,METH_NOARGS,"Loads Modifiers"},
    {"loadModifiers4",py_loadModifiers1,METH_NOARGS,"Loads Modifiers"},
    {"clearModifiers",py_clearModifiers,METH_NOARGS,"Clears Modifiers"},
    //map features -JRA
    {"connector",py_connector,METH_VARARGS,"Connects the ship to the ball in Capture the Flag Mode"},
    {"dropBall",py_dropBall,METH_NOARGS,"Drops the ball in Capture the Flag Mode"},
    {"refuel",py_refuel,METH_VARARGS,"Refuels the ship"},
    //other options -JRA
    {"keyHome",py_keyHome,METH_NOARGS,"Changes the ship's Home Base or respawn location"},
    {"selfDestruct",py_selfDestruct,METH_NOARGS,"Triggers the ship's Self Destruct mechanism"}, //Do not repeatedly press or the ship will not self destruct, it works as a toggle and has a timer -JRA
    {"pauseAI",py_pauseAI,METH_NOARGS,"Pauses the game for the ship, does not affect other ships"},
    {"swapSettings",py_swapSettings,METH_NOARGS,"Swaps between ship Settings for turn rate and thrusting power"},
    {"quitAI",py_quitAI,METH_NOARGS,"Quits the game"}, //Do not have toggleNuclear in the same code segment or else it will not quit -JRA
    {"talkKey",py_talkKey,METH_NOARGS,"Opens up the chat window"},
    {"toggleShowMessage",py_toggleShowMessage,METH_NOARGS,"Toggles Messages on the HUD on the left side of the screen"}, 
    {"toggleShowItems",py_toggleShowItems,METH_NOARGS,"Toggles Items on the HUD on the left side of the screen"}, 
    {"toggleCompass",py_toggleCompass,METH_NOARGS,"Toggles the ship's Compass"}, 
    {"repair",py_repair,METH_NOARGS,"Repairs a target"},
    {"reprogram",py_reprogram,METH_NOARGS,"Reprogram a modifier or lock bank"},
    {"talk",py_talk,METH_VARARGS,"Sends a message"},
    {"scanMsg",py_scanMsg,METH_VARARGS,"Returns the specified message"}, //-EGG
    {"scanGameMsg",py_scanGameMsg,METH_VARARGS,"Returns message at specified index from HUD game feed."}, //-CJG
    //self properties -JRA
    {"selfX",py_selfX,METH_NOARGS,"Returns the ship's X Position"},
    {"selfY",py_selfY,METH_NOARGS,"Returns the ship's Y Position"},
    {"selfRadarX",py_selfRadarX,METH_NOARGS,"Returns the ship's X Radar Coordinate"},
    {"selfRadarY",py_selfRadarY,METH_NOARGS,"Returns the ship's Y Radar Coordinate"},
    {"selfVelX",py_selfVelX,METH_NOARGS,"Returns the ship's X Velocity"},
    {"selfVelY",py_selfVelY,METH_NOARGS,"Returns the ship's Y Velocity"},
    {"selfSpeed",py_selfSpeed,METH_NOARGS,"Returns the ship's Speed"},
    {"lockHeadingDeg",py_lockHeadingDeg,METH_NOARGS,"Returns in Degrees the direction of the ship's Lock-on of an enemy"},
    {"lockHeadingRad",py_lockHeadingRad,METH_NOARGS,"Returns in Radians the direction of the ship's Lock-on of an enemy"},
    {"selfLockDist",py_selfLockDist,METH_NOARGS,"Returns the Distance of the enemy that the ship has Locked-on to"},
    {"selfReload",py_selfReload,METH_NOARGS,"Returns the player's Reload time remaining, based on a call to fireShot()"},
    {"selfID",py_selfID,METH_NOARGS,"Returns the ID of the ship"},
    {"selfAlive",py_selfAlive,METH_NOARGS,"Returns if the ship is Dead or Alive"},
    {"selfTeam",py_selfTeam,METH_NOARGS,"Returns the ship's Team"},
    {"selfLives",py_selfLives,METH_NOARGS,"Returns how many Lives are left for the ship"},
    {"selfTrackingRad",py_selfTrackingRad,METH_NOARGS,"Returns the ship's Tracking in Radians"},
    {"selfTrackingDeg",py_selfTrackingDeg,METH_NOARGS,"Returns the ship's Tracking in Degrees"},
    {"selfHeadingDeg",py_selfHeadingDeg,METH_NOARGS,"Returns the Direction of the ship's Lock-on from the ship in Degrees"},
    {"selfHeadingRad",py_selfHeadingRad,METH_NOARGS,"Returns the Direction of the ship's Lock-on from the ship in Radians"},
    {"hud",py_hud,METH_VARARGS,"Returns the Name on the HUD"}, // -CJG
    {"hudScore",py_hudScore,METH_VARARGS,"Returns the Score on the HUD"},// -CJG
    {"hudTimeLeft",py_hudTimeLeft,METH_VARARGS,"Returns the Time Left on the HUD"},// -CJG
    {"getTurnSpeed",py_getTurnSpeed,METH_NOARGS,"Returns the ship's Turn Speed"},
    {"getPower",py_getPower,METH_NOARGS,"Returns the ship's Turn Speed"},
    {"selfShield",py_selfShield,METH_NOARGS,"Returns the ship's Shield status"},
    {"selfName",py_selfName,METH_NOARGS,"Returns the ship's Name"},
    {"selfScore",py_selfScore,METH_NOARGS,"Returns the ship's Score"},
    //Closest functions -JRA
    {"closestRadarX",py_closestRadarX,METH_NOARGS,"Returns the Closest ship's X Radar Coordinate"},
    {"closestRadarY",py_closestRadarY,METH_NOARGS,"Returns the Closest ship's Y Radar Coordinate"},
    {"closestItemX",py_closestItemX,METH_NOARGS,"Returns the Closest Item's X Radar Coordinate"},
    {"closestItemY",py_closestItemY,METH_NOARGS,"Returns the Closest Item's Y Radar Coordinate"},
    {"closestShipId",py_closestShipId,METH_NOARGS,"Returns the Closest ship's ID"},
    //ID functions -JRA
    {"enemySpeedId",py_enemySpeedId,METH_VARARGS,"Returns the Speed of the Specified Enemy"},
    {"enemyTrackingRadId",py_enemyTrackingRadId,METH_VARARGS,"Returns the Specified Enemy's Tracking in Radians"},
    {"enemyTrackingDegId",py_enemyTrackingDegId,METH_VARARGS,"Returns the Specified Enemy's Tracking in Degrees"},
    {"enemyReloadId",py_enemyReloadId,METH_VARARGS,"Returns the Specified Enemy's Reload time remaining"},
    {"screenEnemyXId",py_screenEnemyXId,METH_VARARGS,"Returns the Specified Enemy's X Coordinate"},
    {"screenEnemyYId",py_screenEnemyYId,METH_VARARGS,"Returns the Specified Enemy's Y Coordinate"},
    {"enemyHeadingDegId",py_enemyHeadingDegId,METH_VARARGS,"Returns the Heading of the Specified Enemy from the ship in Degrees"},
    {"enemyHeadingRadId",py_enemyHeadingRadId,METH_VARARGS,"Returns the Heading of the Specified Enemy from the ship in Radians"},
    {"enemyShieldId",py_enemyShieldId,METH_VARARGS,"Returns the Specified Enemy's Shield Status"},
    {"enemyLivesId",py_enemyLivesId,METH_VARARGS,"Returns the Specified Enemy's Remaining Lives"},
    {"enemyNameId",py_enemyNameId,METH_VARARGS,"Returns the Specified Enemy's Name"},
    {"enemyScoreId",py_enemyScoreId,METH_VARARGS,"Returns the Specified Enemy's Score"},
    {"enemyTeamId",py_enemyTeamId,METH_VARARGS,"Returns the Specified Enemy's Team ID"},
    {"enemyDistanceId",py_enemyDistanceId,METH_VARARGS,"Returns the Distance between the ship and the Specified Enemy"},
    //idx functions -JRA
    {"enemyDistance",py_enemyDistance,METH_VARARGS,"Returns the Distance between the ship and the Specified Enemy"},
    {"enemySpeed",py_enemySpeed,METH_VARARGS,"Returns the Speed of the Specified Enemy"},
    {"enemyReload",py_enemyReload,METH_VARARGS,"Returns the Specified Enemy's Reload time remaining"},
    {"enemyTrackingRad",py_enemyTrackingRad,METH_VARARGS,"Returns the Specified Enemy's Tracking in Radians"},
    {"enemyTrackingDeg",py_enemyTrackingDeg,METH_VARARGS,"Returns the Specified Enemy's Tracking in Degrees"},
    {"screenEnemyX",py_screenEnemyX,METH_VARARGS,"Returns the Specified Enemy's X Coordinate"},
    {"screenEnemyY",py_screenEnemyY,METH_VARARGS,"Returns the Specified Enemy's Y Coordinate"},
    {"enemyHeadingDeg",py_enemyHeadingDeg,METH_VARARGS,"Returns the Heading of the Specified Enemy from the ship in Degrees"},
    {"enemyHeadingRad",py_enemyHeadingRad,METH_VARARGS,"Returns the Heading of the Specified Enemy from the ship in Radians"},
    {"enemyShield",py_enemyShield,METH_VARARGS,"Returns the Specified Enemy's Shield Status"},
    {"enemyLives",py_enemyLives,METH_VARARGS,"Returns the Specified Enemy's Remaining Lives"},
    {"enemyTeam",py_enemyTeam,METH_VARARGS,"Returns the Specified Enemy's Team"},
    {"enemyName",py_enemyName,METH_VARARGS,"Returns the Specified Enemy's Name"},
    {"enemyScore",py_enemyScore,METH_VARARGS,"Returns the Specified Enemy's Score"},
    {"degToRad",py_degToRad,METH_VARARGS,"Converts Degrees to Radians"},
    {"radToDeg",py_radToDeg,METH_VARARGS,"Converts Radians to Degrees"},
    {"angleDiff",py_angleDiff,METH_VARARGS,"Calculates Difference between Two Angles"},
    {"angleAdd",py_angleAdd,METH_VARARGS,"Calculates the Addition of Two Angles"},
    {"wallFeeler",py_wallFeeler,METH_VARARGS,"Returns if there is a wall or not at the Specified Angle within the Specified Distance of the ship"},
    {"wallFeelerRad",py_wallFeelerRad,METH_VARARGS,"Returns if there is a wall or not at the Specified Angle within the Specified Distance of the ship"},
    {"wallBetween",py_wallBetween,METH_VARARGS,"Returns if there is a wall or not between two Specified Points"},
    //Shot functions -JNE
    {"shotAlert",py_shotAlert,METH_VARARGS,"Returns a Danger Rating of a shot"},
    {"shotX",py_shotX,METH_VARARGS,"Returns the X coordinate of a shot"},
    {"shotY",py_shotY,METH_VARARGS,"Returns the Y coordinate of a shot"},
    {"shotDist",py_shotDist,METH_VARARGS,"Returns the Distance of a shot from the ship"},
    {"shotVel",py_shotVel,METH_VARARGS,"Returns the Velocity of a shot"},
    {"shotVelDir",py_shotVelDir,METH_VARARGS,"Returns the Direction of the Velocity of a shot"},
    {"aimdir",py_aimdir,METH_VARARGS,"Returns the Direction that the ship needs to turn to shot the Enemy"},
    //Capture the flag functions - Sarah Penrose
    {"ballX",py_ballX,METH_NOARGS,"Returns the ball's X Position"},
    {"ballY",py_ballY,METH_NOARGS,"Returns the ball's Y Position"},
    {"connectorX0",py_connectorX0,METH_NOARGS,"Returns the connector's X Position"},
    {"connectorX1",py_connectorX1,METH_NOARGS,"Returns the connector's X Position"},
    {"connectorY0",py_connectorY0,METH_NOARGS,"Returns the connector's Y Position"},
    {"connectorY1",py_connectorY1,METH_NOARGS,"Returns the connector's Y Position"},
    {NULL, NULL,0,NULL}
};
//Python/C module definition -EGG
static struct PyModuleDef libpyAI_module = {
    PyModuleDef_HEAD_INIT,
    "libpyAI",
    NULL,
    -1,
    libpyAI_methods
};
//Python initializer -EGG
PyMODINIT_FUNC PyInit_libpyAI(void) {
    return PyModule_Create(&libpyAI_module);
}
