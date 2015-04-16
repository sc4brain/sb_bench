#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <GL/glut.h>
#include "tb.h"
#include "trackball.h"
#include "glm.h"

GLuint     model_list = 0;		/* display list for object */
char*      model_file = NULL;		/* name of the obect file */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLMmodel*  model;
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    scale;			/* scaling factor */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;
GLint      mouse_state = -1;
GLint      mouse_button = -1;
GLboolean  bounding_box = GL_FALSE;
GLboolean  performance = GL_FALSE;
GLboolean  stats = GL_FALSE;
GLfloat    weld_distance = 0.00001;
GLuint     material_mode = 0;

GLfloat    fps=0.0;
GLint      nframe=0;

/* text: general purpose text routine.  draws a string according to
 * format in a stroke font at x, y after scaling it by the scale
 * specified (scale is in window-space (lower-left origin) pixels).  
 *
 * x      - position in x (in window-space)
 * y      - position in y (in window-space)
 * scale  - scale in pixels
 * format - as in printf()
 */
void 
text(GLuint x, GLuint y, GLfloat scale, char* format, ...)
{
  va_list args;
  char buffer[255], *p;
  GLfloat font_scale = 119.05 + 33.33;

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glTranslatef(x, y, 0.0);

  glScalef(scale/font_scale, scale/font_scale, scale/font_scale);

  for(p = buffer; *p; p++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  
  glPopAttrib();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void
lists(void)
{
  GLfloat ambient[] = { 0.2, 0.2, 0.2, 1.0 };
  GLfloat diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
  GLfloat specular[] = { 0.0, 0.0, 0.0, 1.0 };
  GLfloat shininess = 65.0;

  if (model_list)
    glDeleteLists(model_list, 1);

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

  /* generate a list */
  if (material_mode == 0) { 
    if (facet_normal)
      model_list = glmList(model, GLM_FLAT);
    else
      model_list = glmList(model, GLM_SMOOTH);
  } else if (material_mode == 1) {
    if (facet_normal)
      model_list = glmList(model, GLM_FLAT | GLM_COLOR);
    else
      model_list = glmList(model, GLM_SMOOTH | GLM_COLOR);
  } else if (material_mode == 2) {
    if (facet_normal)
      model_list = glmList(model, GLM_FLAT | GLM_MATERIAL);
    else
      model_list = glmList(model, GLM_SMOOTH | GLM_MATERIAL);
  }
}

void
init(void)
{
  tbInit(GLUT_MIDDLE_BUTTON);
  
  /* read in the model */
  model = glmReadOBJ(model_file);
  scale = glmUnitize(model);
  glmFacetNormals(model);
  glmVertexNormals(model, smoothing_angle);

  if (model->nummaterials > 0)
      material_mode = 2;

  /* create new display lists */
  lists();

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_CULL_FACE);
}

void
reshape(int width, int height)
{
  tbReshape(width, height);

  glViewport(0, 0, width, height);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -3.0);
}

void
display(void)
{
  nframe++;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (performance) {
    glColor3f(1.0, 1.0, 1.0);
    //text(5, 5, 20, "%.2f fps", 1.0 / ((end - last) / 1000.0));
    text(5, 5, 20, "%.2f fps", fps);
  }

  glPushMatrix();

  glTranslatef(pan_x, pan_y, 0.0);

  tbMatrix();

  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glColor3f(0.5, 0.5, 0.5);
  glCallList(model_list);

  if (bounding_box) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glColor4f(1.0, 0.0, 0.0, 0.25);
    glutSolidCube(2.0);
  }

  glPopMatrix();

  if (stats) {
    glColor3f(1.0, 1.0, 1.0);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*1), 20, "%s", 
	 model->pathname);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*2), 20, "%d vertices", 
	 model->numvertices);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*3), 20, "%d triangles", 
	 model->numtriangles);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*4), 20, "%d normals", 
	 model->numnormals);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*5), 20, "%d texcoords", 
	 model->numtexcoords);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*6), 20, "%d groups", 
	 model->numgroups);
    text(5, glutGet(GLUT_WINDOW_HEIGHT) - (5+20*7), 20, "%d materials", 
	 model->nummaterials);
  }

  glutSwapBuffers();

}

/* ARGSUSED1 */
void
keyboard(unsigned char key, int x, int y)
{
  //GLint params[2];

  switch (key) {
  case 'q':
  case 27:
    exit(0);
    break;
  }

  glutPostRedisplay();
}

void
menu(int item)
{
    keyboard((unsigned char)item, 0, 0);
}

void
mouse(int button, int state, int x, int y)
{
  GLdouble model[4*4];
  GLdouble proj[4*4];
  GLint view[4];

  tbMouse(button, state, x, y);

  mouse_state = state;
  mouse_button = button;

  if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);
    gluProject((GLdouble)x, (GLdouble)y, 0.0,
		 model, proj, view,
		 &pan_x, &pan_y, &pan_z);
    gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
		 model, proj, view,
		 &pan_x, &pan_y, &pan_z);
    pan_y = -pan_y;
  }

  glutPostRedisplay();
}

void
motion(int x, int y)
{
  GLdouble model[4*4];
  GLdouble proj[4*4];
  GLint view[4];

  tbMotion(x, y);

  if (mouse_state == GLUT_DOWN && mouse_button == GLUT_LEFT_BUTTON) {
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);
    gluProject((GLdouble)x, (GLdouble)y, 0.0,
		 model, proj, view,
		 &pan_x, &pan_y, &pan_z);
    gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
		 model, proj, view,
		 &pan_x, &pan_y, &pan_z);
    pan_y = -pan_y;
  }

  glutPostRedisplay();
}



void fpstimer(int value)
{
  static int prev=0, now=0;
  static unsigned int nframe_prev=0;

  now = glutGet(GLUT_ELAPSED_TIME);

  fps = (float)(nframe - nframe_prev)/(now - prev)*1000.0;
  nframe_prev = nframe;
  prev = now;

  glutTimerFunc(1000, fpstimer, 0);
}


#define USAGE "Usage : \n\t$ ./sb MODEL_ID\n\tMODEL_ID = 1:SB128 2:SB256 3:neurons\n"

int
main(int argc, char** argv)
{
  char sb_filename[100];

  if(argc!=2){
    printf("%s", USAGE);
    exit(-1);
  }
  if(argv[1][0]=='1'){
    strcpy(sb_filename, "obj/SB128.obj");
  }else if(argv[1][0]=='2'){
    strcpy(sb_filename, "obj/SB256.obj");
  }else if(argv[1][0]=='3'){
    strcpy(sb_filename, "obj/bench_neuron.obj");
  }else{
    printf("%s", USAGE);
    exit(-1);
  }

  glutInitWindowSize(900, 900);
  glutInit(&argc, argv);

  model_file = sb_filename;
  performance = 1;


  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Standard Brain Benchmark");
  
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  

  init();
  if(argv[1][0]=='1'){
    glmScale(model, 1.4);
  }else if(argv[1][0]=='2'){
    glmScale(model, 1.4);
  }else if(argv[1][0]=='3'){
    glmScale(model, 2.5);
  }else{
    printf("%s", USAGE);
    exit(-1);
  }
  lists();

  glutTimerFunc(1000, fpstimer, 0);

  
  glutMainLoop();
  return 0;
}
