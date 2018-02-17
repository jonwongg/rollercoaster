/*
 *	rollercoaster.c
 *
 *	An OpenGL implementation of a rollercoaster ride
 *	By Jon Wong
 *
 *	Nov. 23, 2011
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>

#define RAD2DEG 180.0/M_PI
#define DEG2RAD M_PI/180.0

#define myTranslate2D(x,y) glTranslated(x, y, 0.0)
#define myScale2D(x,y) glScalef(x, y, 1.0)
#define myRotate2D(angle) glRotatef(RAD2DEG*angle, 0.0, 0.0, 1.0)


static void    myDisplay(void);
static void    myTimer(int value);
static void    myKey(unsigned char key, int x, int y);
static void    keyPress(int key, int x, int y);
static void    keyRelease(int key, int x, int y);
static void    myReshape(int w, int h);

static void    init(void);
static double    myRandom(double min, double max);

static void    drawTrack(void);
static void    drawSky(void);
static void    drawSkyCeiling(void);
static void    drawGround(void);
static void    drawSupportColumn(void);
static void    calculateNVW(void);
static void    calculateK(float u);
static void    calculateTilt(void);

static void    calculateQ(float u, int degree);

typedef struct Vector3 {
	double x, y, z;
} Vector3;

/* -- global variables ------------------------------------------------------ */

static double xMax, yMax, zMax;
static float cp_count = 18.0, theta, maxHeight, workTotal, speed, k;
Vector3 q, qp, qpp, n, v, w, up, cEye, cLA;
int camera = 0;
float globalu = 3.0;
Vector3 CP[18];

/* -- main ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Jon's Roller Coaster");
    glutDisplayFunc(myDisplay);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(myKey);
    glutReshapeFunc(myReshape);
    glutTimerFunc(33, myTimer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    init();
   
    glutMainLoop();
   
    return 0;
}


/* -- callback functions ---------------------------------------------------- */

void
myDisplay()
{
    /*
    *    display callback function
    */

   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   //checks for the camera mode, toggled by a key
   if(camera %2 == 0)
      gluLookAt( 100*cos(theta),  20,  -100*sin(theta),
                 0,  0,  0,
                 0,  1,  0);

   else
      gluLookAt( cEye.x, cEye.y, cEye.z,
		 cLA.x, cLA.y, cLA.z,
		 up.x, up.y, up.z);

   //draws the plane representing the ground
   drawGround();

   //draws the cylinder surrounding the scene to represent the sky
   glPushMatrix();
      glTranslated(0, -1.5, 0);
      glRotated(-90.0, 1, 0, 0);
      drawSky();
   glPopMatrix();

   //draws the plane to close the scene from the top
   drawSkyCeiling();
   //draws the roller coaster
   drawTrack();    
   //draws the support columns
   drawSupportColumn();

   glutSwapBuffers();
}

void
myTimer(int value)
{
    /*
    *    timer callback function
    */

    //calculates a global q(u) and q`(u) value for the camera (to use in following the track)
    calculateQ(globalu, 0); calculateQ(globalu, 1); calculateQ(globalu, 2);

    //calculates speed
    speed = sqrt(2 * (workTotal + 9.81 * q.y));
    //calculates the value of k and then uses it to rotate about unit vector n
    calculateK(globalu); calculateTilt();

    //updates the eye coordinates of the camera
    cEye.x = q.x;   cEye.y = (q.y * up.y) + 3.0;   cEye.z = q.z;
    //updates the At coordinates of the camera
    cLA.x = q.x + qp.x;   cLA.y = q.y + qp.y;   cLA.z = q.z + qp.z;

    //increases the u value to advance the camera
    globalu += 0.05;

    //when the camera has reached the end of the track, it is now at the beginning of the track
    if(globalu >= cp_count)
       globalu = 3.0;

    //increases the theta value for the camera rotating about the center
    theta += 0.01;
    glutPostRedisplay();
   
    glutTimerFunc(33, myTimer, value);        /* 30 frames per second */
}

void
myKey(unsigned char key, int x, int y)
{
    /*
    *    keyboard callback function
    */

   //exits the program
   if(key == 'q' || key == 'Q')
      exit(0);

   //changes the camera mode
   if(key == 'c' || key == 'C')
      camera++;
}


void
myReshape(int w, int h)
{
    /*
    *    reshape callback function; the upper and lower boundaries of the
    *    window are at 100.0 and 0.0, respectively; the aspect ratio is
    *  determined by the aspect ratio of the viewport
    */

    xMax = 100.0*w/h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(60.0, w/h, 0.5, 1000.0);

    glMatrixMode(GL_MODELVIEW);
}


/* -- other functions ------------------------------------------------------- */

void
init()
{
   int i;
   glEnable(GL_DEPTH_TEST);
   glCullFace(GL_FRONT);

   //control points, not read from a notepad file because reasons
   CP[0].x = 10.0;   CP[0].y = 10.0;   CP[0].z = 0.0;
   CP[1].x = 8.0;   CP[1].y = 12.0;   CP[1].z = -3.0;
   CP[2].x = 3.0;   CP[2].y = 17.0;   CP[2].z = -8.0;
   CP[3].x = -4.0;   CP[3].y = 17.0;   CP[3].z = -6.0;
   CP[4].x = -8.0;   CP[4].y = 17.0;   CP[4].z = -5.0;
   CP[5].x = -12.0;   CP[5].y = 20.0;   CP[5].z = 0.0;
   CP[6].x = -12.0;   CP[6].y = 20.0;   CP[6].z = 5.0;
   CP[7].x = -7.0;   CP[7].y = 30.0;   CP[7].z = 5.0;
   CP[8].x = -3.0;   CP[8].y = 37.0;   CP[8].z = 5.0;
   CP[9].x = -1.0;   CP[9].y = 37.0;   CP[9].z = 5.0;
   CP[10].x = 1.0;   CP[10].y = 32.0;   CP[10].z = 5.0;
   CP[11].x = 3.0;   CP[11].y = 27.0;   CP[11].z = 5.0;
   CP[12].x = 5.0;   CP[12].y = 22.0;   CP[12].z = 5.0;
   CP[13].x = 7.0;   CP[13].y = 17.0;   CP[13].z = 5.0;
   CP[14].x = 9.0;   CP[14].y = 15.0;   CP[14].z = 5.0;
   CP[15].x = 10.0;   CP[15].y = 10.0;   CP[15].z = 0.0;
   CP[16].x = 8.0;   CP[16].y = 12.0;   CP[16].z = -3.0;
   CP[17].x = 3.0;   CP[17].y = 17.0;   CP[17].z = -8.0;

   //calculates the maximum height of the track
   maxHeight = CP[0].y;
   for(i = 0; i < cp_count; i++)
      if(CP[i].y > maxHeight)
	 maxHeight = CP[i].y;

   //increases the total work so overcoming the peak of the track is easier
   workTotal = 1.0 * 9.81 * maxHeight;
   workTotal += 3.0;

   globalu = 3.0;
}


void 
drawTrack()
{
   //SHR and SHU stand for shift right and shift up, respectively
   float u, SHR = 2.0, SHU = 2.0;

   //first side of main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(0, 0.6, 1.0);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 //calculates q(u), q`(u), and n, v, u (denoted w)
         calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 //halves v and u (denoted w) to lessen the size of the rail
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x + v.x - w.x, q.y + v.y - w.y, q.z + v.z - w.z);
	 glVertex3f(q.x - v.x - w.x, q.y - v.y - w.y, q.z - v.z - w.z);
      }
   glEnd();

   //first side of the rail shifted up and to the right of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x + SHR*w.x;
	 q.y += SHU*v.y + SHR*w.y;
	 q.z += SHU*v.z + SHR*w.z;
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;

	 glVertex3f(q.x + v.x - w.x, q.y + v.y - w.y, q.z + v.z - w.z);
	 glVertex3f(q.x - v.x - w.x, q.y - v.y - w.y, q.z - v.z - w.z);
      }
   glEnd();

   //first side of the rail shifted up and to the left of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x - SHR*w.x;
	 q.y += SHU*v.y - SHR*w.y;
	 q.z += SHU*v.z - SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x + v.x - w.x, q.y + v.y - w.y, q.z + v.z - w.z);
	 glVertex3f(q.x - v.x - w.x, q.y - v.y - w.y, q.z - v.z - w.z);
      }
   glEnd();

   //second side of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(0, 0.6, 1.0);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x - v.x - w.x, q.y - v.y - w.y, q.z - v.z - w.z);
	 glVertex3f(q.x - v.x + w.x, q.y - v.y + w.y, q.z - v.z + w.z);
      }
   glEnd();

   //second side of the rail shifted up and to the right of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x + SHR*w.x;
	 q.y += SHU*v.y + SHR*w.y;
	 q.z += SHU*v.z + SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;

	 glVertex3f(q.x - v.x - w.x, q.y - v.y - w.y, q.z - v.z - w.z);
	 glVertex3f(q.x - v.x + w.x, q.y - v.y + w.y, q.z - v.z + w.z);
      }
   glEnd();

   //second side of the rail shifted up and to the left of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x - SHR*w.x;
	 q.y += SHU*v.y - SHR*w.y;
	 q.z += SHU*v.z - SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x - v.x - w.x, q.y - v.y - w.y, q.z - v.z - w.z);
	 glVertex3f(q.x - v.x + w.x, q.y - v.y + w.y, q.z - v.y + w.z);
      }
   glEnd();

   //third side of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(0, 0.6, 1.0);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1);
	 calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x - v.x + w.x, q.y - v.y + w.y, q.z - v.z + w.z);
	 glVertex3f(q.x + v.x + w.x, q.y + v.y + w.y, q.z + v.z + w.z);
      }
   glEnd();

   //third side of the rail shifted up and to the right of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x + SHR*w.x;
	 q.y += SHU*v.y + SHR*w.y;
	 q.z += SHU*v.z + SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x - v.x + w.x, q.y - v.y + w.y, q.z - v.z + w.z);
	 glVertex3f(q.x + v.x + w.x, q.y + v.y + w.y, q.z + v.z + w.z);
      }
   glEnd();

   //third side of the rail shifted up and to the left of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x - SHR*w.x;
	 q.y += SHU*v.y - SHR*w.y;
	 q.z += SHU*v.z - SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x - v.x + w.x, q.y - v.y + w.y, q.z - v.z + w.z);
	 glVertex3f(q.x + v.x + w.x, q.y + v.y + w.y, q.z + v.z + w.z);
      }
   glEnd();

   //fourth side of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(0, 0.6, 1.0);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x + v.x + w.x, q.y + v.y + w.y, q.z + v.z + w.z);
	 glVertex3f(q.x + v.x - w.x, q.y + v.y - w.y, q.z + v.z - w.z);
      }
   glEnd();

   //fourth side of the rail shifted up and to the right of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x + SHR*w.x;
	 q.y += SHU*v.y + SHR*w.y;
	 q.z += SHU*v.z + SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x + v.x + w.x, q.y + v.y + w.y, q.z + v.z + w.z);
	 glVertex3f(q.x + v.x - w.x, q.y + v.y - w.y, q.z + v.z - w.z);
      }
   glEnd();

   //fourth side of the rail shifted up and to the left of the main rail
   glBegin(GL_QUAD_STRIP);
      glColor3f(1.0, 0, 0.8);
      for(u = 3.0; u < cp_count; u += 0.01)
      {
	 calculateQ(u, 0); calculateQ(u, 1); calculateNVW();
	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 q.x += SHU*v.x - SHR*w.x;
	 q.y += SHU*v.y - SHR*w.y;
	 q.z += SHU*v.z - SHR*w.z;

	 v.x /= 2.0; v.y /= 2.0; v.z /= 2.0;
	 w.x /= 2.0; w.y /= 2.0; w.z /= 2.0;
	 glVertex3f(q.x + v.x + w.x, q.y + v.y + w.y, q.z + v.z + w.z);
	 glVertex3f(q.x + v.x - w.x, q.y + v.y - w.y, q.z + v.z - w.z);
      }
   glEnd();
   glFlush();
}

void
calculateQ(float u, int degree)
{
   float t, r0, r1, r2, r3;
   int i = floor(u);
   t = u - i;
   //q(u), as provided in the roller coaster slides
   if(degree == 0)
   {
      r3 = (1.0/6.0) * ((1.0 - t) * (1.0 - t) * (1.0 - t));
      r2 = (1.0/6.0) * ((3.0 * t * t * t) - (6.0 * t * t) + 4.0);
      r1 = (1.0/6.0) * ((-3.0 * t * t * t) + (3.0 * t * t) + (3.0 * t) + 1.0);
      r0 = (1.0/6.0) * (t * t * t);

      q.x = (r3 * CP[i-3].x) + (r2 * CP[i-2].x) + (r1 * CP[i-1].x) + (r0 * CP[i].x);
      q.y = (r3 * CP[i-3].y) + (r2 * CP[i-2].y) + (r1 * CP[i-1].y) + (r0 * CP[i].y);
      q.z = (r3 * CP[i-3].z) + (r2 * CP[i-2].z) + (r1 * CP[i-1].z) + (r0 * CP[i].z);
   }
   //q`(u)
   else if(degree == 1)
   {
      r3 = (-0.5) * ((1.0 - t) * (1.0 - t));
      r2 = (1.5) * (t * t) - (2.0 * t);
      r1 = (-1.5) * (t * t) + t + 0.5;
      r0 = (0.5) * (t * t);

      qp.x = (r3 * CP[i-3].x) + (r2 * CP[i-2].x) + (r1 * CP[i-1].x) + (r0 * CP[i].x);
      qp.y = (r3 * CP[i-3].y) + (r2 * CP[i-2].y) + (r1 * CP[i-1].y) + (r0 * CP[i].y);
      qp.z = (r3 * CP[i-3].z) + (r2 * CP[i-2].z) + (r1 * CP[i-1].z) + (r0 * CP[i].z);
   }
   //q``(u)
   else if(degree == 2)
   {
      r3 = (1.0 - t);
      r2 = (3.0 * t) - 2.0;
      r1 = (-3.0 * t) + 1.0;
      r0 = t;

      qpp.x = (r3 * CP[i-3].x) + (r2 * CP[i-2].x) + (r1 * CP[i-1].x) + (r0 * CP[i].x);
      qpp.y = (r3 * CP[i-3].y) + (r2 * CP[i-2].y) + (r1 * CP[i-1].y) + (r0 * CP[i].y);
      qpp.z = (r3 * CP[i-3].z) + (r2 * CP[i-2].z) + (r1 * CP[i-1].z) + (r0 * CP[i].z);
   }
}

void
calculateNVW()
{
   float nMagnitude, wMagnitude;;
   //n = -q`(u)
   n.x = -qp.x; n.y = -qp.y; n.z = -qp.z;
   nMagnitude = sqrt((qp.x * qp.x) + (qp.y * qp.y) + (qp.z * qp.z));
   //normalizing n
   n.x /= nMagnitude;
   n.y /= nMagnitude;
   n.z /= nMagnitude;

   //calculating u, called w since u has been taken for the q(u) calculation
   w.x = (up.y * n.z) - (up.z * n.y);
   w.y = (up.z * n.x) - (up.x * n.z);
   w.z = (up.x * n.y) - (up.y * n.x);

   wMagnitude = sqrt((w.x * w.x) + (w.y * w.y) + (w.z * w.z));
   //normalizing u (denoted w)
   w.x /= wMagnitude;
   w.y /= wMagnitude;
   w.z /= wMagnitude;
   //calculating v
   v.x = (n.y * w.z) - (n.z * w.y);
   v.y = (n.z * w.x) - (n.x * w.z);
   v.z = (n.x * w.y) - (n.y * w.x);
}

void
calculateK(float u)
{
   float r;
   calculateQ(u, 1); calculateQ(u, 2);
   r = sqrt((qp.x * qp.x) + (qp.y * qp.y) + (qp.z * qp.z));
   //assume there is no tilt
   if(r <= 0.01)
      k = 0;
   else
      k = ((qp.z * qpp.x) - (qp.x * qpp.z)) / (r * r * r);
}

void
calculateTilt()
{
   float c = cos(k), s = sin(k);
   //because up = <0,1,0> is being used, only the y component need be calculated
   up.y = ((1-c)*n.x*n.y-s*n.z) + ((1-c)*n.y*n.y+c) + ((1-c)*n.y*n.z+s*n.x);
}

void
drawSupportColumn()
{
   int i;

   //draws the support columns
   for(i = 3; i < cp_count; i++)
   {
      glBegin(GL_QUAD_STRIP);
	 glColor3f(0, 0, 0);
         calculateQ(i, 0); calculateQ(i, 1); calculateNVW();

	 //lessens the thickness of the columns
	 n.x /= 4.0; n.y /= 4.0; n.z /= 4.0;
	 w.x /= 4.0; w.y /= 4.0; w.z /= 4.0;

	 //draws from the main rail down to below the ground
	 glVertex3f(q.x - n.x + w.x, q.y - n.y + w.y, q.z - n.z + w.z);
	 glVertex3f(q.x - n.x + w.x, -5.0 - n.y + w.y, q.z - n.z + w.z);
	 glVertex3f(q.x + n.x + w.x, q.y + n.y + w.y, q.z + n.z + w.z);
	 glVertex3f(q.x + n.x + w.x, -5.0 + n.y + w.y, q.z + n.z + w.z);
	 glVertex3f(q.x + n.x - w.x, q.y + n.y - w.y, q.z + n.z - w.z);
	 glVertex3f(q.x + n.x - w.x, -5.0 + n.y - w.y, q.z + n.z - w.z);
	 glVertex3f(q.x - n.x - w.x, q.y - n.y - w.y, q.z - n.z - w.z);
	 glVertex3f(q.x - n.x - w.x, -5.0 - n.y - w.y, q.z - n.z - w.z);
      glEnd();
   }
}

//draws a cylinder around the scene
void
drawSky()
{
   GLUquadricObj *sky = gluNewQuadric();
   glColor3f(0.3, 0.4, 0.55);
   gluCylinder(sky, 100, 100, 200, 200, 200);
}
//draws a plane in the upper part of the scene, to close the cylinder from the top
void
drawSkyCeiling()
{
   glColor3f(0.3, 0.4, 0.7);
   glBegin(GL_QUADS);
      glVertex3f(100, 100, -100);
      glVertex3f(100, 100, 100);
      glVertex3f(-100, 100, 100);
      glVertex3f(-100, 100, -100);
   glEnd();
}
//draws a plane in the lower part of the scene, to close the cylinder from the bottom
void
drawGround()
{
   glColor3f(0.2, 0.7, 0.33);
   glBegin(GL_QUADS);
      glVertex3f(100, 0, -100);
      glVertex3f(100, 0, 100);
      glVertex3f(-100, 0, 100);
      glVertex3f(-100, 0, -100);
   glEnd();
}
