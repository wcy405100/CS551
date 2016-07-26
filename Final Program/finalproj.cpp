#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#include "glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"


const char *WINDOWTITLE = { "Final Project" };
const char *GLUITITLE   = { "User Interface Window" };


// what the glui package defines as true and false:

const int GLUITRUE  = { true  };
const int GLUIFALSE = { false };


// the escape key:

#define ESCAPE		0x1b



// initial window size:

const int INIT_WINDOW_SIZE = { 600 };


// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };


// minimum allowable scale factor:

const float MINSCALE = { 0.05f };


// active mouse buttons (or them together):

const int LEFT   = { 4 };
const int MIDDLE = { 2 };
const int RIGHT  = { 1 };


// which projection:

enum Projections
{
	ORTHO,
	PERSP
};


// which button:

enum ButtonVals
{
	RESET,
	QUIT
};


// window background color (rgba):

const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };


// the color numbers:
// this order must match the radio button order

enum Colors
{
	RED,
	YELLOW,
	GREEN,
	CYAN,
	BLUE,
	MAGENTA,
	WHITE,
	BLACK
};

char * ColorNames[ ] =
{
	"Red",
	"Yellow",
	"Green",
	"Cyan",
	"Blue",
	"Magenta",
	"White",
	"Black"
};


// the color definitions:
// this order must match the menu order

const GLfloat Colors[ ][3] = 
{
	{ 1., 0., 0. },		// red
	{ 1., 1., 0. },		// yellow
	{ 0., 1., 0. },		// green
	{ 0., 1., 1. },		// cyan
	{ 0., 0., 1. },		// blue
	{ 1., 0., 1. },		// magenta
	{ 1., 1., 1. },		// white
	{ 0., 0., 0. },		// black
};


// fog parameters:

const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE     = { GL_LINEAR };
const GLfloat FOGDENSITY  = { 0.30f };
const GLfloat FOGSTART    = { 1.5 };
const GLfloat FOGEND      = { 4. };

GLfloat zbuff[600][600]; // set the zbuffer to store the depth information
GLfloat sil[600][600][3]; // silhouettes martix information
GLfloat g[600][600];  
GLfloat ambientcor[600][600][3]; // ambient color term
GLfloat depthImage[600][600];  // store the image depth from the light source
GLuint light_texture; // store light position depth map

// non-constant global variables:

int		ActiveButton;			// current button that is down
int		DebugOn;				// != 0 means to print debugging info
int		DepthCueOn;				// != 0 means to use intensity depth cueing
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		WhichColor;				// index into Colors[ ]
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees

float White[] = { 1., 1., 1., 1. };
bool Freeze = 0;
bool Light0On = 1;
bool Light1On = 1;
float angle = 0;
float Time;
#define MS_PER_CYCLE 5000
#define min(a,b)    (((a) < (b)) ? (a) : (b))

// function prototypes:

void	Animate( );
void	Display( );
float	ElapsedSeconds( );
void	InitGraphics( );
void	MouseButton( int, int, int, int );
void	MouseMotion( int, int );
void	Reset( );
void	Resize( int, int );
void	Visibility( int );

// utility to create an array from 3 separate values:
float *
Array3(float a, float b, float c)
{
	static float array[4];
	array[0] = a;
	array[1] = b;
	array[2] = c;
	array[3] = 1.;
	return array;
}

// utility to create an array from a multiplier and an array:
float *
MulArray3(float factor, float array0[3])
{
	static float array[4];
	array[0] = factor * array0[0];
	array[1] = factor * array0[1];
	array[2] = factor * array0[2];
	array[3] = 1.;
	return array;
}

void
SetMaterial(float r, float g, float b, float shininess)
{
	glMaterialfv(GL_BACK, GL_EMISSION, Array3(0., 0., 0.));
	glMaterialfv(GL_BACK, GL_AMBIENT, MulArray3(.4f, White));
	glMaterialfv(GL_BACK, GL_DIFFUSE, MulArray3(1., White));
	glMaterialfv(GL_BACK, GL_SPECULAR, Array3(0., 0., 0.));
	glMaterialf(GL_BACK, GL_SHININESS, 2.f);
	glMaterialfv(GL_FRONT, GL_EMISSION, Array3(0., 0., 0.)); 
	glMaterialfv(GL_FRONT, GL_AMBIENT, Array3(r, g, b));
	glMaterialfv(GL_FRONT, GL_DIFFUSE, Array3(r, g, b));
	glMaterialfv(GL_FRONT, GL_SPECULAR, MulArray3(.8f, White));
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}

void
SetPointLight(int ilight, float x, float y, float z, float r, float g, float b)
{
	glLightfv(ilight, GL_POSITION, Array3(x, y, z));
	glLightfv(ilight, GL_AMBIENT, Array3(0., 0., 0.));
	glLightfv(ilight, GL_DIFFUSE, Array3(r, g, b));
	glLightfv(ilight, GL_SPECULAR, Array3(r, g, b));
	glLightf(ilight, GL_CONSTANT_ATTENUATION, 1.);
	glLightf(ilight, GL_LINEAR_ATTENUATION, 0.);
	glLightf(ilight, GL_QUADRATIC_ATTENUATION, 0.);
	glEnable(ilight);
}

void
SetSpotLight(int ilight, float x, float y, float z, float xdir, float ydir, float zdir, float r, float g, float b)
{
	glLightfv(ilight, GL_POSITION, Array3(x, y, z));
	glLightfv(ilight, GL_SPOT_DIRECTION, Array3(xdir, ydir, zdir));
	glLightf(ilight, GL_SPOT_EXPONENT, 1.);
	glLightf(ilight, GL_SPOT_CUTOFF, 45.);
	glLightfv(ilight, GL_AMBIENT, Array3(0., 0., 0.));
	glLightfv(ilight, GL_DIFFUSE, Array3(r, g, b));
	glLightfv(ilight, GL_SPECULAR, Array3(r, g, b));
	glLightf(ilight, GL_CONSTANT_ATTENUATION, 1.);
	glLightf(ilight, GL_LINEAR_ATTENUATION, 0.);
	glLightf(ilight, GL_QUADRATIC_ATTENUATION, 0.);
	glEnable(ilight);
}

//GLfloat **initialmatrix(int x, int y){
//
//	GLfloat **mat = (GLfloat **)malloc(sizeof(GLfloat *)*x);
//	for (int i= 0; i<x ; i++)
//		/* Allocate array, store pointer  */
//		mat[i] = (GLfloat *)malloc(sizeof(GLfloat)*y);
//
//	return mat;
//}
//
//void free_matrix(int rows, GLfloat **mat){
//	for (int i = 0; i<rows; i++)
//		free(mat[i]);
//	free(mat);
//}
//
//GLfloat ***initialmatrix3(int x, int y){
//
//	GLfloat ***mat = (GLfloat ***)malloc(sizeof(GLfloat **)*x);
//	for (int i = 0; i < x; i++){
//		/* Allocate array, store pointer  */
//		mat[i] = (GLfloat **)malloc(sizeof(GLfloat*)*y);
//		for (int j = 0; j < y; j++){
//			mat[i][j] = (GLfloat *)malloc(sizeof(GLfloat)*3);
//			mat[i][j][0] = 1.;
//			mat[i][j][1] = 1.;
//			mat[i][j][2] = 1.;
//		}
//	}
//	return mat;
//}
//
//void free_matrix3(int rows, int col, GLfloat ***mat){
//	
//	for (int i = 0; i < rows; i++){
//		for (int j = 0; j < col; j++){
//			free(mat[i][j]);
//		}
//		free(mat[i]);
//	}
//	free(mat);
//}

float get_max(float a[9])
{
	float result = a[0];
	for (int i = 1; i < 9; i++)
	{
		if (a[i] >= result)
		{
			result = a[i];
		}
	}
	return result;
}

float get_min(float a[9])
{
	float result = a[0];
	for (int i = 1; i < 9; i++)
	{
		if (a[i] <= result)
		{
			result = a[i];
		}
	}
	return result;
}

//void gival(int i, int j, GLfloat *mat){
//	for (int x = 0; x < i; x++){
//		for (int y = 0; y < j; y++){
//			mat[x][y] = 10 * x + y;
//		}
//	}
//}
//
//void prnt(int x, int y, GLfloat **mat){
//	for (int i = 0; i<x; i++){
//		for (int j = 0; j<y; j++){
//			printf("[%d,%d]data is : %f\n", i, j, mat[i][j]);
//		}
//	}
//}

void drawshadow(){

	GLint   viewport[4];
	GLfloat  lightPos[4];

	glGetLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluLookAt(lightPos[0], lightPos[1], lightPos[2], 0, 0, 0, 0, 1, 0);

	//get light position depth buffer store in gramebuffer
	glGenTextures(1, &light_texture);
	glBindTexture(GL_TEXTURE_2D, light_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 600, 600, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, 600, 600, 0);

	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	
	//glReadPixels(0, 0, 600, 600, GL_DEPTH_COMPONENT, GL_FLOAT, depthImage);
	//glWindowPos2f(viewport[2] , 0);
	//glDrawPixels(600, 600, GL_LUMINANCE, GL_FLOAT, depthImage);
	//glutSwapBuffers();
}


void get_silhouettes()
{
	float p;
	float nib[9];
	float gmax, gmin;
	float kp = 0.0003;
	float edgeth = 0.2;
	
	GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
	GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);

	//free_matrix(vx, zbuff); // Here the row should be the width of the windows in the last time
	//zbuff = initialmatrix(vx, vy);
	/*gival(vx, vy, zbuff);
	prnt(vx,vy,zbuff);*/
	//free_matrix3(vx, vy, sil);
	//sil = initialmatrix3(vx, vy);

	//prnt(5, 5, &zbuff);

	// read the buffer into vx*vy dimension matrix
	//for (int j = 1; j < vy+1; j++){
		//GLfloat *z_temp = (GLfloat *)malloc(sizeof(GLfloat)* vx);
		//glReadBuffer(GL_FLOAT);
		//glReadPixels(0, j-1, vx , j, GL_DEPTH_COMPONENT, GL_FLOAT, z_temp);
		//for (int i = 0; i < vx; i++){
		//	zbuff[i][j - 1] = z_temp[i];
		//	printf("%d,%d\n", i, j - 1);
		//}
		//free(z_temp);
	//}

	/*float **arr = new float*[vx];
	for (int i = 0; i < vx; i++){
		arr[i] = new float[vy];
	}*/

	glReadBuffer(GL_FLOAT);
	glReadPixels(0, 0, 600, 600, GL_DEPTH_COMPONENT, GL_FLOAT, zbuff);

	//printf("Read zbuffer done!\n");
	//prnt(5, 5, zbuff);

	for (int i = 0; i < 600; i++){
		for (int j = 0; j < 600; j++){
			sil[i][j][0] = 1.;
			sil[i][j][1] = 1.;
			sil[i][j][2] = 1.;
		}
	}

	for (int i = 1; i < vx-1; i++)
	{
		for (int j = 1; j < vy-1; j++)
		{
			g[i][j] = (fabs(zbuff[i - 1][j + 1] - zbuff[i][j]) +
				2 * fabs(zbuff[i][j + 1] - zbuff[i][j]) +
				fabs(zbuff[i + 1][j + 1] - zbuff[i][j]) +
				2 * fabs(zbuff[i - 1][j] - zbuff[i][j]) +
				2 * fabs(zbuff[i + 1][j] - zbuff[i][j]) +
				fabs(zbuff[i - 1][j - 1] - zbuff[i][j]) +
				2 * fabs(zbuff[i][j - 1] - zbuff[i][j]) +
				fabs(zbuff[i + 1][j - 1] - zbuff[i][j])) / 8;
		}
	}

	for (int i = 1; i < 599; i++)
	{
		for (int j = 1; j < 599; j++)
		{
			nib[0] = g[i - 1][j + 1];
			nib[1] = g[i][j + 1];
			nib[2] = g[i + 1][j + 1];
			nib[3] = g[i - 1][j];
			nib[4] = g[i][j];
			nib[5] = g[i + 1][j];
			nib[6] = g[i - 1][j - 1];
			nib[7] = g[i][j - 1];
			nib[8] = g[i + 1][j - 1];

			gmax = get_max(nib);
			gmin = get_min(nib);

			p = min(pow(((gmax - gmin) / kp), 2), 1);

			if (p >= edgeth)
			{
				sil[i][j][0] = 0.;
				sil[i][j][1] = 0.;
				sil[i][j][2] = 0.;
			}
		}
	}
}

void getcor(){
	glReadBuffer(GL_FLOAT);
	glReadPixels(0, 0, 600, 600, GL_RGB, GL_FLOAT, ambientcor);
}

void setcolor()
{
	for (int i = 0; i < 600; i++)
	{
		for (int j = 0; j < 600; j++)
		{
			for (int q = 0; q < 3; q++)
			{
				if (ambientcor[i][j][q] < 0.33)
				{
					ambientcor[i][j][q] = ambientcor[i][j][q] / 1.5;
					//ambientcor[i][j][q] = 0.165;
				}
				else if ((ambientcor[i][j][q] >= 0.34) && (ambientcor[i][j][q] < 0.66))
				{
					ambientcor[i][j][q] = (ambientcor[i][j][q] - 0.33) / 2 + 0.33;
					//ambientcor[i][j][q] = 0.5;
				}
				else if (ambientcor[i][j][q] >= 0.67)
				{
					ambientcor[i][j][q] = (ambientcor[i][j][q] - 0.66) / 2 + 0.66;
					//ambientcor[i][j][q] = 0.835;
				}

				if (sil[i][j][q] == 0)
				{
					ambientcor[i][j][q] = 0;
				}
			}
		}
	}
	glDrawPixels(600, 600, GL_RGB, GL_FLOAT, ambientcor);
	glFlush();
}


void edgedisp(){
	for (int i = 0; i < 600; i++)
	{
		for (int j = 0; j < 600; j++)
		{
			for (int q = 0; q < 3; q++)
			{
				if (sil[i][j][q] == 0)
				{
					ambientcor[i][j][q] = 0;
				}
				else{
					ambientcor[i][j][q] = 1;
				}
			}
		}
	}
	glDrawPixels(600, 600, GL_RGB, GL_FLOAT, ambientcor);
	glFlush();
}

// main program:

int
main( int argc, char *argv[ ] )
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit( &argc, argv );


	// setup all the graphics stuff:

	InitGraphics( );


	glutSetWindow(MainWindow);

	// init all the global variables used by Display( ):
	// this will also post a redisplay

	Reset( );

	// draw the scene once and wait for some interaction:
	// (this will never return)

	glutSetWindow( MainWindow );
	glutMainLoop( );

	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutMainLoop( ) do it

void
Animate( )
{
	// put animation stuff in here -- change some global variables
	// for Display( ) to find:

	// force a call to Display( ) next time it is convenient:
	//angle = angle + M_PI / 4;

	int ms = glutGet(GLUT_ELAPSED_TIME);
	ms %= MS_PER_CYCLE;
	Time = (float)ms / (float)(MS_PER_CYCLE - 1);

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}

void myDisplay()
{
	glClearColor(0.8f, 0.9f, 0.9f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
}

// draw the complete scene:

void
Display( )
{
	if( DebugOn != 0 )
	{
		fprintf( stderr, "Display\n" );
	}


	// set which window we want to do the graphics into:

	glutSetWindow( MainWindow );

	myDisplay();
	// erase the background:

	glDrawBuffer( GL_BACK );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_DEPTH_TEST );


	// specify shading to be flat:

	glShadeModel( GL_SMOOTH );


	// set the viewport to a square centered in the window:

	GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
	GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = ( vx - v ) / 2;
	GLint yb = ( vy - v ) / 2;
	glViewport( xl, yb,  v, v );

	//printf("The windows width:%d\n", vx);
	//printf("The windows height:%d\n", vy);

	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	if( WhichProjection == ORTHO )
		glOrtho( -3., 3., -3., 3., 0.1, 1000. );
	else
		gluPerspective( 90., 1., 0.1, 1000. );


	// place the objects into the scene:

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );


	// set the eye position, look-at position, and up-vector:

	gluLookAt( 3., 3.5, 6.,     0., 0., 0.,     0., 1., 0. );


	// rotate the scene:

	glRotatef( (GLfloat)Yrot, 0., 1., 0. );
	glRotatef( (GLfloat)Xrot, 1., 0., 0. );


	// uniformly scale the scene:

	if( Scale < MINSCALE )
		Scale = MINSCALE;
	glScalef( (GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale );


	// set the fog parameters:

	if( DepthCueOn != 0 )
	{
		glFogi( GL_FOG_MODE, FOGMODE );
		glFogfv( GL_FOG_COLOR, FOGCOLOR );
		glFogf( GL_FOG_DENSITY, FOGDENSITY );
		glFogf( GL_FOG_START, FOGSTART );
		glFogf( GL_FOG_END, FOGEND );
		glEnable( GL_FOG );
	}
	else
	{
		glDisable( GL_FOG );
	}


	// since we are using glScalef( ), be sure normals get unitized:

	glEnable( GL_NORMALIZE );
	glEnable(GL_TEXTURE_2D);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, MulArray3(.2, White));
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	// draw the current object:
	// Light source
	SetPointLight(GL_LIGHT0, 3.0, 3., 3., 1., 1., 1.);
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glColor3f(1., 1., 1.);
	glTranslatef(3., 3., 3.);
	glutSolidSphere(0.1, 50, 50);
	glEnd();
	glEnable(GL_LIGHTING);
	glPopMatrix();

	/*SetPointLight(GL_LIGHT1, -3.0, 3., 3., 1., 1., 1.);
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glColor3f(1., 1., 0.);
	glTranslatef(-3., 3., 3.);
	glutSolidSphere(0.1, 50, 50);
	glEnd();
	glEnable(GL_LIGHTING);
	glPopMatrix();*/

	if (Light0On == 1)
		glEnable(GL_LIGHT0);
	else
		glDisable(GL_LIGHT0);


	//teapot
	glPushMatrix();
	glShadeModel(GL_SMOOTH);
	SetMaterial(0.8, 0.4, 0.0, 50.0);
	glRotatef(360.*Time, 0., 1., 0.);
	glutSolidTeapot(2);
	glPopMatrix();

	//glEnable(GL_TEXTURE_2D);
	glPushMatrix();
	drawshadow();
	glPopMatrix(); 

	glPushMatrix();
	glTranslatef(0., -6., 0.);
	glBindTexture(GL_TEXTURE_2D, light_texture);
	glutSolidCube(8.0);

	//glBegin(GL_QUADS);
	//glNormal3f(0., 1., 0.);
	//glTexCoord2f(0., 1.);
	//glVertex3f(-5., 0., 5.);
	//glTexCoord2f(1., 1.);
	//glVertex3f(-5., 0., -5.);
	//glTexCoord2f(1., 0.);
	//glVertex3f(5., 0., -5.);
	//glTexCoord2f(0., 0.);
	//glVertex3f(5., 0., 5.);
	//glEnd();

	glPopMatrix();

	// draw some gratuitous text that is fixed on the screen:
	//
	// the projection matrix is reset to define a scene whose
	// world coordinate system goes from 0-100 in each axis
	//
	// this is called "percent units", and is just a convenience
	//
	// the modelview matrix is reset to identity as we don't
	// want to transform these coordinates

	glDisable( GL_DEPTH_TEST );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
	gluOrtho2D( 0., 100.,     0., 100. );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	//use the paper algorithm to generate the cartoon looking part.
	
	
	getcor();
	get_silhouettes();
	setcolor();
	//edgedisp();
	//drawshadow();
	
	// swap the double-buffered framebuffers:
	glutSwapBuffers( );

	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !

	glFlush( );

}

// return the number of seconds since the start of the program:

float
ElapsedSeconds( )
{
	// get # of milliseconds since the start of the program:

	int ms = glutGet( GLUT_ELAPSED_TIME );

	// convert it to seconds:

	return (float)ms / 1000.f;
}


// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions

void
InitGraphics( )
{
	int Width, Height;
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );

	// set the initial window configuration:

	glutInitWindowPosition( 0, 0 );
	glutInitWindowSize( 600, 600 );

	// open the window and set its title:

	MainWindow = glutCreateWindow( WINDOWTITLE );
	glutSetWindowTitle( WINDOWTITLE );

	// set the framebuffer clear values:

	glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

	glutSetWindow( MainWindow );
	glutDisplayFunc( Display );
	glutReshapeFunc( Resize );
	glutMouseFunc( MouseButton );
	glutMotionFunc( MouseMotion );
	glutPassiveMotionFunc( NULL );
	glutVisibilityFunc( Visibility );
	glutEntryFunc( NULL );
	glutSpecialFunc( NULL );
	glutSpaceballMotionFunc( NULL );
	glutSpaceballRotateFunc( NULL );
	glutSpaceballButtonFunc( NULL );
	glutButtonBoxFunc( NULL );
	glutDialsFunc( NULL );
	glutTabletMotionFunc( NULL );
	glutTabletButtonFunc( NULL );
	glutTimerFunc( -1, NULL, 0 );
	glutIdleFunc( Animate );

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// init glew (a window must be open to do this):
#ifdef WIN32
	GLenum err = glewInit( );
	if( err != GLEW_OK )
	{
		fprintf( stderr, "glewInit Error\n" );
	}
	else
		fprintf( stderr, "GLEW initialized OK\n" );
	fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

}


// called when the mouse button transitions down or up:

void
MouseButton( int button, int state, int x, int y )
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if( DebugOn != 0 )
		fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

	
	// get the proper button bit mask:

	switch( button )
	{
		case GLUT_LEFT_BUTTON:
			b = LEFT;		break;

		case GLUT_MIDDLE_BUTTON:
			b = MIDDLE;		break;

		case GLUT_RIGHT_BUTTON:
			b = RIGHT;		break;

		default:
			b = 0;
			fprintf( stderr, "Unknown mouse button: %d\n", button );
	}


	// button down sets the bit, up clears the bit:

	if( state == GLUT_DOWN )
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}
}


// called when the mouse moves while a button is down:

void
MouseMotion( int x, int y )
{
	if( DebugOn != 0 )
		fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if( ( ActiveButton & LEFT ) != 0 )
	{
		Xrot += ( ANGFACT*dy );
		Yrot += ( ANGFACT*dx );
	}


	if( ( ActiveButton & MIDDLE ) != 0 )
	{
		Scale += SCLFACT * (float) ( dx - dy );

		// keep object from turning inside-out or disappearing:

		if( Scale < MINSCALE )
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow( MainWindow );
	glutPostRedisplay( );
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset( )
{
	ActiveButton = 0;
	DebugOn = 0;
	DepthCueOn = 0;
	Scale  = 1.0;
	WhichColor = WHITE;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	Freeze = 1;
	Light0On = 1;
	Light1On = 1;
}


// called when user resizes the window:

void
Resize( int width, int height )
{
	glutReshapeWindow(600, 600);
}


// handle a change to the window's visibility:

void
Visibility ( int state )
{
	if( DebugOn != 0 )
		fprintf( stderr, "Visibility: %d\n", state );

	if( state == GLUT_VISIBLE )
	{
		glutSetWindow( MainWindow );
		glutPostRedisplay( );
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}