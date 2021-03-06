#include <android-config.h>
#include <ws.h>
#include "fbdev_window.h"
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
extern "C" {
#include <eglplatformcommon.h>
};

#include "logging.h"

static gralloc_module_t *gralloc = 0;
static framebuffer_device_t *framebuffer = 0;
static alloc_device_t *alloc = 0;
static FbDevNativeWindow *_nativewindow = NULL;

extern "C" void fbdevws_init_module(struct ws_egl_interface *egl_iface)
{
	int err;
	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
	if (gralloc==NULL) {
		fprintf(stderr, "failed to get gralloc module: (%s)\n",strerror(-err));
		assert(0);
	}

	err = framebuffer_open((hw_module_t *) gralloc, &framebuffer);
	if (err) {
		fprintf(stderr, "ERROR: failed to open framebuffer: (%s)\n",strerror(-err));
		assert(0);
	}
	TRACE("** framebuffer_open: status=(%s) format=x%x", strerror(-err), framebuffer->format);

	err = gralloc_open((const hw_module_t *) gralloc, &alloc);
	if (err) {
		fprintf(stderr, "ERROR: failed to open gralloc: (%s)\n",strerror(-err));
		assert(0);
	}
	TRACE("** gralloc_open %p status=%s", gralloc, strerror(-err));
	eglplatformcommon_init(egl_iface, gralloc, alloc);
}

extern "C" int fbdevws_IsValidDisplay(EGLNativeDisplayType display)
{
	assert (gralloc != NULL);
	return display == EGL_DEFAULT_DISPLAY;
}

extern "C" EGLNativeWindowType fbdevws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	assert (gralloc != NULL);
	assert (_nativewindow == NULL);

	_nativewindow = new FbDevNativeWindow(alloc, framebuffer);
	_nativewindow->common.incRef(&_nativewindow->common);
	return (EGLNativeWindowType) static_cast<struct ANativeWindow *>(_nativewindow);
}

extern "C" void fbdevws_DestroyWindow(EGLNativeWindowType win)
{
	assert (_nativewindow != NULL);
	assert (static_cast<FbDevNativeWindow *>((struct ANativeWindow *)win) == _nativewindow);

	_nativewindow->common.decRef(&_nativewindow->common);
	/* We are done with it, refcounting will delete the window when appropriate */
	_nativewindow = NULL;
}

extern "C" __eglMustCastToProperFunctionPointerType fbdevws_eglGetProcAddress(const char *procname) 
{
	return eglplatformcommon_eglGetProcAddress(procname);
}

extern "C" void fbdevws_passthroughImageKHR(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list)
{
	eglplatformcommon_passthroughImageKHR(ctx, target, buffer, attrib_list);
}

extern "C" void fbdevws_setSwapInterval(EGLDisplay dpy, EGLNativeWindowType win, EGLint interval)
{
	FbDevNativeWindow *window = static_cast<FbDevNativeWindow *>((struct ANativeWindow *)win);
	window->setSwapInterval(interval);
}

struct ws_module ws_module_info = {
	fbdevws_init_module,
	fbdevws_IsValidDisplay,
	fbdevws_CreateWindow,
	fbdevws_DestroyWindow,
	fbdevws_eglGetProcAddress,
	fbdevws_passthroughImageKHR,
	eglplatformcommon_eglQueryString,
	NULL,
	NULL,
	fbdevws_setSwapInterval,
};

// vim:ts=4:sw=4:noexpandtab
