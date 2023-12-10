#include <EGL/egl.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

#include <cstddef>
#include <string>
#include <sstream>
#include <stdexcept>
#include <assert.h>
#include <iostream>

static GLubyte *pixels = NULL;
static GLuint fbo;
static GLuint rbo_color;
static GLuint rbo_depth;
static int offscreen = 1;
static unsigned int max_nframes = 128;
static unsigned int nframes = 0;
static unsigned int time0;
static unsigned int height = 500;
static unsigned int width = 500;


/* Model. */
static double angle;
static double delta_angle;

  static const EGLint configAttribs[] = {
          EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_DEPTH_SIZE, 8,
          EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
          EGL_NONE
  };    

  static const int pbufferWidth = 9;
  static const int pbufferHeight = 9;

  static const EGLint pbufferAttribs[] = {
        EGL_WIDTH, pbufferWidth,
        EGL_HEIGHT, pbufferHeight,
        EGL_NONE,
  };


static void screenshot_ppm(const char *filename, unsigned int width,
        unsigned int height, GLubyte **pixels) {
    size_t i, j, cur;
    const size_t format_nchannels = 3;
    FILE *f = fopen(filename, "w");
    fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
    *pixels = (GLubyte*)realloc((void*)*pixels, format_nchannels * sizeof(GLubyte) * width * height);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, *pixels);
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            cur = format_nchannels * ((height - i - 1) * width + j);
            fprintf(f, "%3d %3d %3d ", (*pixels)[cur], (*pixels)[cur + 1], (*pixels)[cur + 2]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

static void model_init(void) {
    angle = 0;
    delta_angle = 1;
}

static int model_update(void) {
    angle += delta_angle;
    return 0;
}

static int model_finished(void) {
    return nframes >= max_nframes;
}

static void draw_scene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glRotatef(angle, 0.0f, 0.0f, -1.0f);
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f( 0.0f,  0.5f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f( 0.5f, -0.5f, 0.0f);
    glEnd();
}


static void init(void)  {
    int glget;

    /*  Framebuffer */
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* Color renderbuffer. */
    glGenRenderbuffers(1, &rbo_color);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
    /* Storage must be one of: */
    /* GL_RGBA4, GL_RGB565, GL_RGB5_A1, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX8. */
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, width, height);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_color);

    /* Depth renderbuffer. */
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    /* Sanity check. */
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glget);
    assert(width < (unsigned int)glget);
    assert(height < (unsigned int)glget);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    time0 += 2;
    model_init();
}


void assertOpenGLError(const std::string& msg) {
	GLenum error = glGetError();

	if (error != GL_NO_ERROR) {
		std::stringstream s;
		s << "OpenGL error 0x" << std::hex << error << " at " << msg;
		throw std::runtime_error(s.str());
	}
}


void assertEGLError(const std::string& msg) {
	EGLint error = eglGetError();

	if (error != EGL_SUCCESS) {
		std::stringstream s;
		s << "EGL error 0x" << std::hex << error << " at " << msg;
		throw std::runtime_error(s.str());
	}
}


int main(int argc, char *argv[])
{
  // 1. Initialize EGL
  EGLDisplay eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  assertEGLError("eglGetDisplay");

  EGLint major, minor;

  eglInitialize(eglDpy, &major, &minor);
  assertEGLError("eglInitialize");

  // 2. Select an appropriate configuration
  EGLint numConfigs;
  EGLConfig eglCfg;

  eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
  assertEGLError("eglChooseConfig");

  // 3. Create a surface
  EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, 
                                               pbufferAttribs);

  // 4. Bind the API
  eglBindAPI(EGL_OPENGL_API);
  assertEGLError("eglBindAPI");

  // 5. Create a context and make it current
  EGLContext eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, 
                                       NULL);
  assertEGLError("eglCreateContext");

  eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
  assertEGLError("eglMakeCurrent");

  // from now on use your OpenGL context

  /*
    * Create an OpenGL framebuffer as render target.
    */
    GLuint frameBuffer;
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    assertOpenGLError("glBindFramebuffer");

	/*
	 * Create a texture as color attachment.
	 */
	GLuint t;
	glGenTextures(1, &t);

	glBindTexture(GL_TEXTURE_2D, t);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 500, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	assertOpenGLError("glTexImage2D");
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    	/*
	 * Attach the texture to the framebuffer.
	 */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t, 0);
	assertOpenGLError("glFramebufferTexture2D");

	
	/*
	 * Render something.
	 */
	// glClearColor(0.9, 0.8, 0.5, 1.0);
	// glClear(GL_COLOR_BUFFER_BIT);

  init();
  model_init();
  draw_scene();
  glFlush();
  screenshot_ppm("result.ppm", width, height, &pixels);

  glDeleteFramebuffers(1, &frameBuffer);
	glDeleteTextures(1, &t);

// 	eglDestroyContext(display, context);
// assertEGLError("eglDestroyContext");

  // 6. Terminate EGL when finished
  eglTerminate(eglDpy);
  assertEGLError("eglTerminate");
  return 0;
}