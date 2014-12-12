/* This sample is based on a sample that is included with ASSIMP. Much
 * of the logic of the program was unchanged. However, texture loading
 * and other miscellaneous changes were made.
 *
 * Changes by: Scott Kuhl
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include "kuhl-util.h"
#include "vecmat.h"
#include "dgr.h"
#include "projmat.h"
#include "viewmat.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/anim.h>

GLuint program = 0; // id value for the GLSL program
kuhl_geometry *modelgeom = NULL;
float bbox[6];

/** Set this variable to 1 to force this program to scale the entire
 * model and translate it so that we can see the entire model. This is
 * a useful setting to use when you are loading a new model that you
 * are unsure about the units and position of the model geometry. */
#define FIT_TO_VIEW_AND_ROTATE 1
/** The location in 3D space that we want the center of the bounding
 * box to be (if FIT_TO_VIEW_AND_ROTATE is set) or the location that
 * we should put the origin of the model */
float placeToPutModel[3] = { 0, 0, 0 };
/** SketchUp produces files that older versions of ASSIMP think 1 unit
 * is 1 inch. However, all of this software assumes that 1 unit is 1
 * meter. So, we need to convert some models from inches to
 * meters. Newer versions of ASSIMP correctly read the same files and
 * give us units in meters. */
#define INCHES_TO_METERS 0

GLuint scene_list = 0; // display list for model
char *modelFilename = NULL;
char *modelTexturePath = NULL;
int renderStyle = 2;

struct aiScene *scene;
kuhl_geometry geom;

typedef struct {
	GLfloat velocity[3];
} particle;

particle **particles;

#define GLSL_VERT_FILE "ogl3-assimp.vert"
#define GLSL_FRAG_FILE "ogl3-assimp.frag"

/** Give each vertex a velocity when the explosion occurs. */
void explode()
{
	kuhl_geometry *g = modelgeom;
	for(unsigned int i=0; i<kuhl_geometry_count(modelgeom); i++)
	{
		/* Get the normal information from each of the vertices. */
		GLint numFloats = 0;
		GLfloat *norm = kuhl_geometry_attrib_get(g, "in_Normal",
		                                         &numFloats);
		
		/* Calculate the velocity of each vertex when the explosion occurs */
		for(unsigned int j=0; j<g->vertex_count; j++)
		{
			// Start by setting the velocity equal to the normal to
			// make the particles move out.
			vec3f_copy(particles[i][j].velocity, &norm[j*3]);

			// Instead of moving the particles only in the direction
			// of the normal, make them move 'up' (in object
			// coordinates) too.
			particles[i][j].velocity[1] += .5;

			// Add a bit of randomness
			for(int k=0; k<3; k++)
				particles[i][j].velocity[k] += (drand48()-.5);
		}
		g = g->next;
	}
}

/** Update the vertex positions and the velocity stored in the
 * particles array. */
void update()
{
	kuhl_geometry *g = modelgeom;
	for(unsigned int i=0; i<kuhl_geometry_count(modelgeom); i++)
	{
		int numFloats = 0;
		GLfloat *pos = kuhl_geometry_attrib_get(g, "in_Position",
		                                        &numFloats);

		for(unsigned int j=0; j<g->vertex_count; j++)
		{
			/* If the first point isn't moving, don't update anything */
			if(vec3f_norm(particles[i][j].velocity) == 0)
				return;

			/* Gravity is pushing particles down -Y, but we are
			 * operating in object coordinates. If GeomTransform
			 * (i.e., g->matrix) is used to rotate the model, then
			 * gravity might not push the particles down in world
			 * coordinates. */
			float accel[3] = { 0, -1, 0};
			float timestep = 0.1f; // change this to change speed of explosion
			for(int k=0; k<3; k++)
			{
				pos[j*3+k] += timestep * (particles[i][j].velocity[k] + timestep * accel[k]/2);
				particles[i][j].velocity[k] += timestep * accel[k];
			}
		}
		g = g->next;
	}
}



/* Called by GLUT whenever a key is pressed. */
void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 'q':
		case 'Q':
		case 27: // ASCII code for Escape key
			exit(0);
			break;
		case '+': // increase size of points and width of lines
		{
			GLfloat currentPtSize;
			GLfloat sizeRange[2];
			glGetFloatv(GL_POINT_SIZE, &currentPtSize);
			glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, sizeRange);
			GLfloat temp = currentPtSize+1;
			if(temp > sizeRange[1])
				temp = sizeRange[1];
			glPointSize(temp);
			printf("Point size is %f (can be between %f and %f)\n", temp, sizeRange[0], sizeRange[1]);
			kuhl_errorcheck();

			GLfloat currentLineWidth;
			GLfloat widthRange[2];
			glGetFloatv(GL_LINE_WIDTH, &currentLineWidth);
			glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, widthRange);
			temp = currentLineWidth+1;
			if(temp > widthRange[1])
				temp = widthRange[1];
			glLineWidth(temp);
			printf("Line width is %f (can be between %f and %f)\n", temp, widthRange[0], widthRange[1]);
			kuhl_errorcheck();
			break;
		}
		case '-': // decrease size of points and width of lines
		{
			GLfloat currentPtSize;
			GLfloat sizeRange[2];
			glGetFloatv(GL_POINT_SIZE, &currentPtSize);
			glGetFloatv(GL_SMOOTH_POINT_SIZE_RANGE, sizeRange);
			GLfloat temp = currentPtSize-1;
			if(temp < sizeRange[0])
				temp = sizeRange[0];
			glPointSize(temp);
			printf("Point size is %f (can be between %f and %f)\n", temp, sizeRange[0], sizeRange[1]);
			kuhl_errorcheck();

			GLfloat currentLineWidth;
			GLfloat widthRange[2];
			glGetFloatv(GL_LINE_WIDTH, &currentLineWidth);
			glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, widthRange);
			temp = currentLineWidth-1;
			if(temp < widthRange[0])
				temp = widthRange[0];
			glLineWidth(temp);
			printf("Line width is %f (can be between %f and %f)\n", temp, widthRange[0], widthRange[1]);
			kuhl_errorcheck();
			break;
		}
		case 'x':
			explode();
			break;
		case 'z':
			update();
			break;
		case ' ': // Toggle different sections of the GLSL fragment shader
			renderStyle++;
			if(renderStyle > 8)
				renderStyle = 0;
			switch(renderStyle)
			{
				case 0: printf("Render style: Diffuse (headlamp light)\n"); break;
				case 1: printf("Render style: Texture\n"); break;
				case 2: printf("Render style: Vertex color\n"); break;
				case 3: printf("Render style: Vertex color + diffuse (headlamp light)\n"); break;
				case 4: printf("Render style: Normals\n"); break;
				case 5: printf("Render style: Texture coordinates\n"); break;
				case 6: printf("Render style: Front (green) and back (red) faces based on winding\n"); break;
				case 7: printf("Render style: Front (green) and back (red) based on normals\n"); break;
				case 8: printf("Render style: Depth (white=far; black=close)\n"); break;
			}
			break;
	}

	/* Whenever any key is pressed, request that display() get
	 * called. */ 
	glutPostRedisplay();
}


void get_model_matrix(float result[16])
{
	mat4f_identity(result);
	if(FIT_TO_VIEW_AND_ROTATE == 0)
	{
		/* Translate the model to where we were asked to put it */
		float translate[16];
		mat4f_translateVec_new(translate, placeToPutModel);

		/* Do inches to meters conversion if we are asked to. */
		float scale[16];
		mat4f_identity(scale);
		if(INCHES_TO_METERS)
		{
			float inchesToMeters=1/39.3701;
			mat4f_scale_new(scale, inchesToMeters, inchesToMeters, inchesToMeters);
		}
		mat4f_mult_mat4f_new(result, translate, scale);
		return;
	}

	/* Get a matrix to scale+translate the model based on the bounding
	 * box */
	float fitMatrix[16];
	kuhl_bbox_fit(fitMatrix, bbox, 1);

	/* Get a matrix that moves the model to the correct location. */
	float moveToLookPoint[16];
	mat4f_translateVec_new(moveToLookPoint, placeToPutModel);

	/* Create a single model matrix. */
	mat4f_mult_mat4f_new(result, moveToLookPoint, fitMatrix);
}

void display()
{
	dgr_update();

	dgr_setget("style", &renderStyle, sizeof(int));

	// Clear the screen to black, clear the depth buffer
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST); // turn on depth testing
	kuhl_errorcheck();

	/* Turn on blending (note, if you are using transparent textures,
	   the transparency may not look correct unless you draw further
	   items before closer items.). */
	glEnable(GL_BLEND);
	glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
	
	/* Render the scene once for each viewport. Frequently one
	 * viewport will fill the entire screen. However, this loop will
	 * run twice for HMDs (once for the left eye and once for the
	 * right. */
	for(int viewportID=0; viewportID<viewmat_num_viewports(); viewportID++)
	{
		/* Where is the viewport that we are drawing onto and what is its size? */
		int viewport[4]; // x,y of lower left corner, width, height
		viewmat_get_viewport(viewport, viewportID);
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

		/* Get the frustum information which will be later used to generate a perspective projection matrix. */
		float f[6]; // left, right, top, bottom, near>0, far>0
		projmat_get_frustum(f, viewport[2], viewport[3]);
	    
		/* Get the view or camera matrix; update the frustum values if needed. */
		float viewMat[16];
		viewmat_get(viewMat, f, viewportID);

		glUseProgram(program);
		/* Communicate matricies to OpenGL */
		float perspective[16];
		mat4f_frustum_new(perspective,f[0], f[1], f[2], f[3], f[4], f[5]);
		glUniformMatrix4fv(kuhl_get_uniform("Projection"),
		                   1, // count
		                   0, // transpose
		                   perspective); // value
		float modelMat[16];
		get_model_matrix(modelMat);
		
		// modelview = view * model
		float modelview[16];
		mat4f_mult_mat4f_new(modelview, viewMat, modelMat);

		glUniformMatrix4fv(kuhl_get_uniform("ModelView"),
		                   1, // count
		                   0, // transpose
		                   modelview); // value

		glUniform1i(kuhl_get_uniform("renderStyle"), renderStyle);
		// Copy far plane value into vertex program so we can render depth buffer.
		glUniform1f(kuhl_get_uniform("farPlane"), f[5]);
		
		kuhl_errorcheck();

		kuhl_limitfps(60);
		update();
		kuhl_geometry_draw(modelgeom); /* Draw the model */
		kuhl_errorcheck();

		glUseProgram(0); // stop using a GLSL program.

	} // finish viewport loop

	
	/* Check for errors. If there are errors, consider adding more
	 * calls to kuhl_errorcheck() in your code. */
	kuhl_errorcheck();

	glFlush();
	glFinish();
	
	/* Display the buffer we just drew (necessary for double buffering). */
	glutSwapBuffers();

	// kuhl_video_record("videoout", 30);
	
	/* Ask GLUT to call display() again. We shouldn't call display()
	 * ourselves recursively because it will not leave time for GLUT
	 * to call other callback functions for when a key is pressed, the
	 * window is resized, etc. */
	glutPostRedisplay();
}






int main(int argc, char** argv)
{
	if(argc == 2)
	{
		modelFilename = argv[1];
		modelTexturePath = NULL;
	}
	else if(argc == 3)
	{
		modelFilename = argv[1];
		modelTexturePath = argv[2];
	}
	else
	{
		printf("Usage:\n"
		       "%s modelFile     - Textures are assumed to be in the same directory as the model.\n"
		       "- or -\n"
		       "%s modelFile texturePath\n", argv[0], argv[0]);
		exit(1);
	}

	/* set up our GLUT window */
	glutInit(&argc, argv);
	glutInitWindowSize(512, 512);
	/* Ask GLUT to for a double buffered, full color window that
	 * includes a depth buffer */
#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitContextVersion(3,2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
#endif
	glutCreateWindow(argv[0]); // set window title to executable name
	glEnable(GL_MULTISAMPLE);

	/* Initialize GLEW */
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if(glewError != GLEW_OK)
	{
		fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(glewError));
		exit(EXIT_FAILURE);
	}
	/* When experimental features are turned on in GLEW, the first
	 * call to glGetError() or kuhl_errorcheck() may incorrectly
	 * report an error. So, we call glGetError() to ensure that a
	 * later call to glGetError() will only see correct errors. For
	 * details, see:
	 * http://www.opengl.org/wiki/OpenGL_Loading_Library */
	glGetError();

	// setup callbacks
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);

	/* Compile and link a GLSL program composed of a vertex shader and
	 * a fragment shader. */
	program = kuhl_create_program(GLSL_VERT_FILE, GLSL_FRAG_FILE);

	dgr_init();     /* Initialize DGR based on environment variables. */
	projmat_init(); /* Figure out which projection matrix we should use based on environment variables */

	float initCamPos[3]  = {0,1,2}; // location of camera
	float initCamLook[3] = {0,0,0}; // a point the camera is facing at
	float initCamUp[3]   = {0,1,0}; // a vector indicating which direction is up
	viewmat_init(initCamPos, initCamLook, initCamUp);
	
	// Clear the screen while things might be loading
	glClearColor(.2,.2,.2,1);
	glClear(GL_COLOR_BUFFER_BIT);
	glutSwapBuffers();

	// Load the model from the file
	modelgeom = kuhl_load_model(modelFilename, modelTexturePath, program, bbox);

	/* Count the number of kuhl_geometry objects for this model */
	unsigned int geomCount = kuhl_geometry_count(modelgeom);
	
	/* Allocate an array of particle arrays */
	particles = malloc(sizeof(particle*)*geomCount);
	int i = 0;
	for(kuhl_geometry *g = modelgeom; g != NULL; g=g->next)
	{
		/* allocate space to store velocity information for all of the
		 * vertices in this kuhl_geometry */
		particles[i] = malloc(sizeof(particle)*g->vertex_count);
		for(unsigned int j=0; j<g->vertex_count; j++)
			vec3f_set(particles[i][j].velocity, 0,0,0);

		/* Change the geometry to be drawn as points */
		g->primitive_type = GL_POINTS; // Comment out this line to default to triangle rendering.
		i++;
	}

	/* Tell GLUT to start running the main loop and to call display(),
	 * keyboard(), etc callback methods as needed. */
	glutMainLoop();
    /* // An alternative approach:
    while(1)
       glutMainLoopEvent();
    */

	exit(EXIT_SUCCESS);
}
