#include "MageApp.h"

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace mage
{

	//---------------------------------------
	struct Engine
	{
		android_app* app;
		int32 width, height;

		EGLDisplay display;
		EGLSurface surface;
		EGLContext context;
	};
	static Engine gEngine;

	static bool gIsRunning;
	static bool gIsPaused;
	static Clock* gMasterClock;

	static UpdateFn gUpdateFn = DefaultUpdateFn;
	static RenderFn gRenderFn = DefaultRenderFn;
	static OnSaveStateRestoredFn gOnSaveStateRestoredFn = DefaultOnSaveStateRestoredFn;
	static OnSaveStateFn gOnSaveStateFn = DefaultOnSaveStateFn;
	static OnDestroyFn gOnDestroyFn = DefaultOnDestroyFn;
	static OnScreenSizeChangedFn gOnScreenSizeChanged = DefaultOnScreenSizeChangedFn;
	static OnWindowShownFn gOnWindowShownFn = DefaultOnWindowShownFn;

	static void handleAppCmd( struct android_app* app, int32_t cmd );
	static int32_t handleInputEvent( struct android_app* app, AInputEvent* event );

	static void OnDraw();

	static int initGL();
	static void shutdownGL();
	static void printGLString( const char *name, GLenum s );
	//---------------------------------------


	//---------------------------------------
	bool InitApp( const char* title, struct android_app* app )
	{
		// Make sure glue isn't stripped.
		app_dummy();

		DebugPrintf( "Initializing Application\n" );

		memset( &gEngine, 0, sizeof(gEngine) );
		gEngine.app = app;
		app->userData = &gEngine;
		app->onAppCmd = handleAppCmd;
		app->onInputEvent = handleInputEvent;

		DebugPrintf( "Registering asset manager\n" );

		InitializeAssetManager( app->activity->assetManager );

		DebugPrintf( "Creating clock\n" );

		gMasterClock = &Clock::Initialize();

		if( app->savedState != NULL )
		{
			// We are starting with a previous saved state; restore from it.
			DebugPrintf( "Restoring previous savestate\n" );
			gOnSaveStateRestoredFn( app->savedState );
		}

		DebugPrintf( "Init complete\n" );

		return true;
	}
	//---------------------------------------
	void Run()
	{
		gIsRunning = true;
		const float dt = 1.0f / 60.0f;

		while( gIsRunning )
		{
			// Read all pending events.
			int ident;
			int events;
			struct android_poll_source* source;

			while( (ident = ALooper_pollAll( 0, NULL, &events, (void**)&source ) ) >= 0 )
			{
				// Process this event.
				if( source != NULL )
					source->process( gEngine.app, source );

				// Check if we are exiting.
				if( gEngine.app->destroyRequested != 0 )
				{
					DebugPrintf( "Shuting Down\n" );
					shutdownGL();
					gOnDestroyFn();
					return;
				}
			}

			// Only do stuff if not paused
			if ( !gIsPaused )
			{
				gMasterClock->AdvanceTime( dt );
				gUpdateFn( dt );
				OnDraw();
			}
		}
	}
	//---------------------------------------
	// Default Callbacks
	//---------------------------------------
	void DefaultUpdateFn( float dt )
	{}
	//---------------------------------------
	void DefaultRenderFn()
	{
		ClearScreen();
	}
	//---------------------------------------
	void DefaultOnSaveStateRestoredFn( const void* data )
	{}
	//---------------------------------------
	size_t DefaultOnSaveStateFn( void* data )
	{
		return 0;
	}
	//---------------------------------------
	void DefaultOnDestroyFn()
	{}
	//---------------------------------------
	void DefaultOnScreenSizeChangedFn( int32 w, int32 h )
	{}
	//---------------------------------------
	void DefaultOnWindowShownFn()
	{}
	//---------------------------------------
	// Callback registers
	//---------------------------------------
	void RegisterUpdateFn( UpdateFn fn )
	{
		gUpdateFn = fn;
	}
	//---------------------------------------
	void RegisterRenderFn( RenderFn fn )
	{
		gRenderFn = fn;
	}
	//---------------------------------------
	void RegisterOnSaveStateRestoredFn( OnSaveStateRestoredFn fn )
	{
		gOnSaveStateRestoredFn = fn;
	}
	//---------------------------------------
	void RegisterOnSaveStateFn( OnSaveStateFn fn )
	{
		gOnSaveStateFn = fn;
	}
	//---------------------------------------
	void RegisterOnDestroyFn( OnDestroyFn fn )
	{
		gOnDestroyFn = fn;
	}
	//---------------------------------------
	void RegisterOnScreenSizeChangedFn( OnScreenSizeChangedFn fn )
	{
		gOnScreenSizeChanged = fn;
	}
	//---------------------------------------
	void RegisterOnWindowShownFn( OnWindowShownFn fn )
	{
		gOnWindowShownFn = fn;
	}
	//---------------------------------------
	// Input handling
	//---------------------------------------
	void handleAppCmd( struct android_app* app, int32_t cmd )
	{
		Engine* engine = (Engine*) app->userData;

		switch( cmd )
		{
			case APP_CMD_SAVE_STATE:
			{
				DebugPrintf( "Saving state\n" );
				// The system has asked us to save our current state.  Do so.
				engine->app->savedStateSize = gOnSaveStateFn( engine->app->savedState );
			}
			break;

			case APP_CMD_INIT_WINDOW:
			{
				// The window is being shown, get it ready.
				if( engine->app->window != NULL )
				{
					initGL();
					OnDraw();
					DebugPrintf( "Showing window\n" );
					gOnWindowShownFn();
				}
			}
			break;

			case APP_CMD_TERM_WINDOW:
			{
				// The window is being hidden or closed, clean it up.
				DebugPrintf( "Hiding window\n" );
				shutdownGL();
			}
			break;

			case APP_CMD_GAINED_FOCUS:
			{
				DebugPrintf( "Focus gained...\n" );
			}
			break;

			case APP_CMD_LOST_FOCUS:
			{
				DebugPrintf( "Focus lost...\n" );
			}
			break;

			case APP_CMD_PAUSE:
			{
				DebugPrintf( "Pausing Activity...\n" );
				gIsPaused = true;
			}
			break;

			case APP_CMD_RESUME:
			{
				DebugPrintf( "Resuming Activity...\n" );
				gIsPaused = false;
			}
			break;
		}
	}
	//---------------------------------------
	int32_t handleInputEvent( struct android_app* app, AInputEvent* pEvent )
	{
	//    LOGI("AMotionEvent_getAction=%d", AMotionEvent_getAction(pEvent));
	//    LOGI("AMotionEvent_getFlags=%d", AMotionEvent_getFlags(pEvent));
	//    LOGI("AMotionEvent_getMetaState=%d", AMotionEvent_getMetaState(pEvent));
	//    LOGI("AMotionEvent_getEdgeFlags=%d", AMotionEvent_getEdgeFlags(pEvent));
	//    LOGI("AMotionEvent_getDownTime=%lld", AMotionEvent_getDownTime(pEvent));
	//    LOGI("AMotionEvent_getEventTime=%lld", AMotionEvent_getEventTime(pEvent));
	//    LOGI("AMotionEvent_getXOffset=%f", AMotionEvent_getXOffset(pEvent));
	//    LOGI("AMotionEvent_getYOffset=%f", AMotionEvent_getYOffset(pEvent));
	//    LOGI("AMotionEvent_getXPrecision=%f", AMotionEvent_getXPrecision(pEvent));
	//    LOGI("AMotionEvent_getYPrecision=%f", AMotionEvent_getYPrecision(pEvent));
	//    LOGI("AMotionEvent_getPointerCount=%d", AMotionEvent_getPointerCount(pEvent));
	//    LOGI("AMotionEvent_getRawX=%f", AMotionEvent_getRawX(pEvent, 0));
	//    LOGI("AMotionEvent_getRawY=%f", AMotionEvent_getRawY(pEvent, 0));
	//    LOGI("AMotionEvent_getX=%f", AMotionEvent_getX(pEvent, 0));
	//    LOGI("AMotionEvent_getY=%f", AMotionEvent_getY(pEvent, 0));
	//    LOGI("AMotionEvent_getPressure=%f", AMotionEvent_getPressure(pEvent, 0));
	//    LOGI("AMotionEvent_getSize=%f", AMotionEvent_getSize(pEvent, 0));
	//    LOGI("AMotionEvent_getOrientation=%f", AMotionEvent_getOrientation(pEvent, 0));
	//    LOGI("AMotionEvent_getTouchMajor=%f", AMotionEvent_getTouchMajor(pEvent, 0));
	//    LOGI("AMotionEvent_getTouchMinor=%f", AMotionEvent_getTouchMinor(pEvent, 0));

		Engine* engine = (Engine*) app->userData;

		if( AInputEvent_getType( pEvent ) == AINPUT_EVENT_TYPE_MOTION )
		{
			size_t pointerCount = AMotionEvent_getPointerCount( pEvent );

			for( size_t i = 0; i < pointerCount; ++i )
			{
				DebugPrintf( "Received motion event from pointer %zu: (%.1f, %.1f)",
				      i, AMotionEvent_getX( pEvent, i ), AMotionEvent_getY( pEvent, i ) );
			}
			return 1;
		}
		else if( AInputEvent_getType( pEvent ) == AINPUT_EVENT_TYPE_KEY )
		{
			DebugPrintf( "Received key event: %d", AKeyEvent_getKeyCode( pEvent ) );

			return 1;
		}

		return 0;
	}
	//---------------------------------------
	void OnDraw()
	{
		if( gEngine.display == NULL )
		{
			// No display.
			return;
		}

		gRenderFn();

		eglSwapBuffers( gEngine.display, gEngine.surface );
	}
	//---------------------------------------
	// OpenGL Initialization
	int initGL()
	{
		// initialize OpenGL ES and EGL

		//
		// Here specify the attributes of the desired configuration.
		// Below, we select an EGLConfig with at least 8 bits per color
		// that supports OpenGL ES 2.0.
		//

		const EGLint attribs[] =
		{
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE
		};

		EGLint w, h, dummy, format;
		EGLint numConfigs;
		EGLConfig config;
		EGLSurface surface;
		EGLContext context;

		EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

		eglInitialize( display, 0, 0 );

		//
		// Here, the application chooses the configuration it desires. In this
		// sample, we have a very simplified selection process, where we pick
		// the first EGLConfig that matches our criteria.
		//

		eglChooseConfig( display, attribs, &config, 1, &numConfigs );

		//
		// EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
		// guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
		// As soon as we picked a EGLConfig, we can safely reconfigure the
		// ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
		//

		eglGetConfigAttrib( display, config, EGL_NATIVE_VISUAL_ID, &format );

		ANativeWindow_setBuffersGeometry( gEngine.app->window, 0, 0, format );

		surface = eglCreateWindowSurface( display, config, gEngine.app->window, NULL );

		const EGLint attrib_list [] =
		{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
		};

		context = eglCreateContext( display, config, NULL, attrib_list );

		if( eglMakeCurrent( display, surface, surface, context ) == EGL_FALSE )
		{
			WarnCrit( "Unable to eglMakeCurrent" );
			return -1;
		}

		eglQuerySurface( display, surface, EGL_WIDTH, &w );
		eglQuerySurface( display, surface, EGL_HEIGHT, &h );

		gEngine.display = display;
		gEngine.context = context;
		gEngine.surface = surface;
		gEngine.width = w;
		gEngine.height = h;

		// Initialize GL state.
		//glEnable( GL_DEPTH_TEST );
		//glEnable( GL_CULL_FACE );
		glFrontFace( GL_CCW );

		printGLString( "Version", GL_VERSION );
		printGLString( "Vendor", GL_VENDOR );
		printGLString( "Renderer", GL_RENDERER );
		printGLString( "Extensions", GL_EXTENSIONS );

		if ( !CreateRenderer() )
		{
			FatalError( "Failed to create Renderer\n" );
			return -1;
		}

		SetViewport( 0, 0, w, h );
		SetOrthoView( -w/2, w/2, -h/2, h/2, 0, 1 );

		gOnScreenSizeChanged( w, h );

		return 0;
	}
	//---------------------------------------
	// OpenGL shutdown
	void shutdownGL()
	{
		if( gEngine.display != EGL_NO_DISPLAY )
		{
			eglMakeCurrent( gEngine.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );

			if( gEngine.context != EGL_NO_CONTEXT )
				eglDestroyContext( gEngine.display, gEngine.context );

			if( gEngine.surface != EGL_NO_SURFACE )
				eglDestroySurface( gEngine.display, gEngine.surface );

			eglTerminate( gEngine.display );
		}

		gEngine.display = EGL_NO_DISPLAY;
		gEngine.context = EGL_NO_CONTEXT;
		gEngine.surface = EGL_NO_SURFACE;
	}
	//---------------------------------------
	void printGLString( const char *name, GLenum s )
	{
		const char *v = (const char *) glGetString( s );
		DebugPrintf( "GL %s = %s\n", name, v );
	}
	//---------------------------------------


}
