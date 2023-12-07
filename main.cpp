#include <EGL/egl.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>

#include <cstddef>
#include <string>
#include <sstream>
#include <stdexcept>

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
	glClearColor(0.9, 0.8, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

    glDeleteFramebuffers(1, &frameBuffer);
	glDeleteTextures(1, &t);

    // 	eglDestroyContext(display, context);
	// assertEGLError("eglDestroyContext");

  // 6. Terminate EGL when finished
  eglTerminate(eglDpy);
  assertEGLError("eglTerminate");
  return 0;
}