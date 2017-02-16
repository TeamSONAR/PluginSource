// Example low level rendering Unity plugin

#include "PlatformBase.h"
#include "RenderAPI.h"
#include "Windows.h"
#include "GLEW\glew.h"

#include <assert.h>
#include <math.h>

#pragma comment(lib, "user32.lib")
#define BUF_SIZE 4+640*480*4

TCHAR szName[] = TEXT("Local\\MyFileMappingObject");
struct Harambe {
	PVOID BufferLoc;
	HANDLE hMapFile;
};

// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity (float t) { 
	g_Time = t; 
}

static int gError;

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetError() {
	return gError;
}

//static int CamX;
//static int CamY;
//static int CamH;
//static int CamW;
static int* PixelBufferPtr;
static void *texHandle;
static HGLRC context;
static HDC Dcontext;
static int depthfunc;
static bool depthtestenabled;
static void* StructPtr;
static int GLDepthFormat = GL_DEPTH_COMPONENT;

void* CreateDepthBufMapFile()
{
	static Harambe DataStruct;
	void *pBuf;

	DataStruct.hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		BUF_SIZE,                // maximum object size (low-order DWORD)
		szName);                 // name of mapping object

	if (DataStruct.hMapFile == NULL)
	{
		//_tprintf(TEXT("Could not create file mapping object (%d).\n"),
		//GetLastError());
		return 0;
	}
	pBuf = MapViewOfFile(DataStruct.hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		BUF_SIZE);

	if (pBuf == NULL)
	{

		CloseHandle(DataStruct.hMapFile);

		return 0;
	}

	DataStruct.BufferLoc = pBuf;
	char* DumyPtr = (char*)pBuf;
	char* DumyPtr2 = DumyPtr + 4;
	PixelBufferPtr = (int*)(DumyPtr2);
	return &DataStruct;
}

int UnmapDepthBufFile() {
	Harambe *HarUnmapFileStruct = (Harambe*)StructPtr;
	UnmapViewOfFile(HarUnmapFileStruct->BufferLoc);

	CloseHandle(HarUnmapFileStruct->hMapFile);

	return 47;
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetupReadPixels() {
	StructPtr = CreateDepthBufMapFile();

	if (StructPtr == 0) {
		return 0;
	}

	Harambe* DummyStruct = (Harambe*)StructPtr;
	int* LaserPtr = (int*)DummyStruct->BufferLoc;
	*LaserPtr = 1;

	return 1;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnmapFile() {
	UnmapDepthBufFile();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetDepthF(int GDF) {
	GLDepthFormat = GDF;
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetPBVal(int num) {
	return PixelBufferPtr[num];
}

static void* g_TextureHandle = NULL;
static int   g_TextureWidth  = 0;
static int   g_TextureHeight = 0;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTextureFromUnity(void* textureHandle, int w, int h)
{
	g_TextureHandle = textureHandle;
	g_TextureWidth = w;
	g_TextureHeight = h;
}

// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	s_UnityInterfaces = unityInterfaces;
	s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
	s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
	
	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

// --------------------------------------------------------------------------
// GraphicsDeviceEvent


static RenderAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

// --------------------------------------------------------------------------
// OnRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.

static void  UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	// Unknown / unsupported graphics device type? Do nothing
	if (s_CurrentAPI == NULL)
		return;

	Harambe* DummyStruct = (Harambe*)StructPtr;
	int* LaserPtr = (int*)DummyStruct->BufferLoc;
	*LaserPtr = 1;

	context = wglGetCurrentContext();
	Dcontext = wglGetCurrentDC();

	GLint Viewport[4];
	glGetIntegerv(GL_VIEWPORT, Viewport);

	glReadBuffer(GL_FRONT);
	glReadPixels(Viewport[0], Viewport[1], Viewport[2], Viewport[3], GLDepthFormat, GL_UNSIGNED_INT_8_8_8_8, PixelBufferPtr);
	gError = glGetError();

	*LaserPtr = 0;
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ReadPixelsOutOfContext() {
	wglMakeCurrent(Dcontext, context);
	GLint Viewport[4];
	glGetIntegerv(GL_VIEWPORT, Viewport);
	glReadBuffer(GL_FRONT);
	glReadPixels(Viewport[0], Viewport[1], Viewport[2], Viewport[3], GL_DEPTH_COMPONENT, GL_FLOAT, PixelBufferPtr);

	gError = glGetError();
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API IsDTE() {
	return depthtestenabled;
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetDepthFunc() {
	return depthfunc;
}

static void UNITY_INTERFACE_API  OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		assert(s_CurrentAPI == NULL);
		s_DeviceType = s_Graphics->GetRenderer();
		s_CurrentAPI = CreateRenderAPI(s_DeviceType);
	}

	// Let the implementation process the device related events
	if (s_CurrentAPI)
	{
		s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
	}

	// Cleanup graphics API implementation upon shutdown
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		delete s_CurrentAPI;
		s_CurrentAPI = NULL;
		s_DeviceType = kUnityGfxRendererNull;
	}
}


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTHandle(void *tHandle) {
	texHandle = tHandle;
}