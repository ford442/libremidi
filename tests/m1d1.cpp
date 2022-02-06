#include <vector>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <libremidi/libremidi.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <memory>
#include <SDL2/SDL.h>
#include <iostream>
#include <ctime>

using namespace std;
using namespace std::chrono;

Uint8 *stm;
SDL_AudioDeviceID dev;
struct{SDL_AudioSpec spec;Uint8* snd;Uint32 slen;int pos;}wave;

using std::string;
high_resolution_clock::time_point t1;
high_resolution_clock::time_point t2;

static const char *read_file_into_str(const char *filename){
char *result=NULL;
long length=0;
FILE *file=fopen(filename,"r");
if(file){
int status=fseek(file,0,SEEK_END);
if(status!=0){
fclose(file);
return NULL;
}
length=ftell(file);
status=fseek(file,0,SEEK_SET);
if(status!=0){
fclose(file);
return NULL;
}
result=static_cast<char*>(malloc((length+1)*sizeof(char)));
if(result){
size_t actual_length=fread(result,sizeof(char),length,file);
result[actual_length++]={'\0'};
} 
fclose(file);
return result;
}
return NULL;
}

static const char common_shader_header_gles3[]=
"#version 300 es \n"
"precision highp float;precision highp int; \n";

static const char vertex_shader_body_gles3[]=
"layout(location=0)in vec3 aPos;layout(location=1)in vec3 aColor;"
"out vec3 ourColor;void main(){gl_Position=vec4(aPos,1.0);ourColor=aColor;} \n\0";

static const char fragment_shader_header_gles3[]=
"in highp vec3 ourColor; \n"
"out highp vec4 FragColor; \n";

static const char fragment_shader_footer_gles3[]=
" \n\0";

EGLDisplay display;
EGLContext contextegl;
EGLSurface surface;
EmscriptenWebGLContextAttributes attr;
SDL_Window *win;
SDL_GLContext *glCtx;

static const char* common_shader_header=common_shader_header_gles3;
static const char* vertex_shader_body=vertex_shader_body_gles3;
static const char* fragment_shader_header=fragment_shader_header_gles3;
static const char* fragment_shader_footer=fragment_shader_footer_gles3;

GLuint shader_program;
GLfloat mouseX=0.0f;
GLfloat mouseY=0.0f;
GLfloat mouseLPressed=0.0f;
GLfloat mouseRPressed=0.0f;
GLfloat viewportSizeX=0.0f;
GLfloat viewportSizeY=0.0f;
GLfloat abstime;

static GLuint compile_shader(GLenum type,GLsizei nsources,const char **sources){
GLuint shader;
GLsizei i,srclens[nsources];
for (i=0;i<nsources;++i){
SDL_Log("GL Shader: %s",sources[i]);
srclens[i]=(GLsizei)strlen(sources[i]);
}
shader=glCreateShader(type);
glShaderSource(shader,nsources,sources,srclens);
glCompileShader(shader);
return shader;
}

GLfloat ink[]={1.0f,0.0f,0.0f,1.0f};
GLfloat vertices[2160]={};
GLuint VBO,VAO;
long double white;
int x,y;
long double siz,outTimeA;
int a;
float b;
Uint32 buttons;

static void renderFrame(){
glClear(GL_COLOR_BUFFER_BIT);
siz=0.42;
SDL_PumpEvents();
t2=steady_clock::now();
duration<double> time_spana=duration_cast<duration<double>>(t2 - t1);
outTimeA=time_spana.count();
abstime=outTimeA*1000;
buttons=SDL_GetMouseState(&x, &y);
mouseX=x/viewportSizeX;
mouseY=y/viewportSizeY;
ink[0]=mouseX/2;
ink[1]=mouseY;
white=abstime-(round(abstime/1000)*1000);
white=1000/white;
if((buttons & SDL_BUTTON_LMASK)!=0){
mouseLPressed=1.0f;
ink[2]=white;
siz=0.77;
vertices[7]=1.0f-mouseX;
vertices[1]=1.0f-mouseY;
vertices[13]=1.0f-mouseX;
vertices[10]=0.0f-white;
vertices[11]=1.0f-mouseY;
vertices[2]=white;
vertices[8]=vertices[11];
vertices[32]=white/1.2f;
vertices[38]=white/1.1f;
vertices[0]=vertices[32]-white;
vertices[3]=vertices[1]-white;
vertices[2]=vertices[2]-white;
}else{
for(a=0;a<361;a++){
b=(float)a/360;
vertices[a*6]=siz*cos(a);
vertices[(a*6)+1]=siz*sin(a);
vertices[(a*6)+2]=b;
vertices[(a*6)+3]=1.0f;
vertices[(a*6)+4]=white;
vertices[(a*6)+5]=1.0f-b;
};
mouseLPressed=0.0f;
vertices[7]=-0.5f;
vertices[1]=-0.5f;
vertices[13]=0.7f;
vertices[10]=1.0f-white;
vertices[11]=white;
vertices[3]=1.0f;
vertices[0]=0.5f;
vertices[1]=-0.5f;
vertices[2]=0.0f;
ink[2]=white;
ink[0]=white/100;
}

glClearColor(ink[0],ink[1],ink[2],ink[3]);
glGenVertexArrays(1,&VAO);
glGenBuffers(1,&VBO);
glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER,VBO);
glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
glEnableVertexAttribArray(0);
glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
glEnableVertexAttribArray(1);
glUseProgram(shader_program);
glDrawArrays(GL_TRIANGLES,0,360);
eglSwapBuffers(display,surface);
}

static const EGLint attribut_list[]={
// EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB,
EGL_NONE
};

static const EGLint attribute_list[]={
// EGL_COLOR_COMPONENT_TYPE_EXT,EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT,EGL_TRUE,
EGL_DEPTH_ENCODING_NV,EGL_DEPTH_ENCODING_NONLINEAR_NV,
EGL_RENDER_BUFFER,EGL_QUADRUPLE_BUFFER_NV,
// EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE,EGL_TRUE,
EGL_RED_SIZE,8,
EGL_GREEN_SIZE,8,
EGL_BLUE_SIZE,8,
EGL_ALPHA_SIZE,8,
EGL_DEPTH_SIZE,24,
EGL_STENCIL_SIZE,8,
EGL_BUFFER_SIZE,32,
EGL_NONE
};
int ii;
GLuint vtx,frag;
char *fileloc="/shader/shader1.glsl";

static void strt(){
for(ii=0;ii<2161;ii++){
vertices[ii]=0.0f;
}
string program_source=read_file_into_str(fileloc);
const char* default_fragment_shader=program_source.c_str();
const char *sources[4];
emscripten_webgl_init_context_attributes(&attr);
attr.alpha=EM_TRUE;
attr.stencil=EM_TRUE;
attr.depth=EM_TRUE;
attr.antialias=EM_FALSE;
attr.premultipliedAlpha=EM_FALSE;
attr.preserveDrawingBuffer=EM_FALSE;
attr.enableExtensionsByDefault=EM_FALSE;
attr.powerPreference=EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
attr.failIfMajorPerformanceCaveat=EM_FALSE;
attr.majorVersion=2;
attr.minorVersion=0;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx=emscripten_webgl_create_context("#canvas",&attr);
EGLConfig eglconfig=NULL;
EGLint config_size,major,minor;
display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
eglInitialize(display,&major,&minor);
if(eglChooseConfig(display,attribute_list,&eglconfig,1,&config_size)==EGL_TRUE && eglconfig!=NULL){
if(eglBindAPI(EGL_OPENGL_ES_API)!=EGL_TRUE){
}
EGLint anEglCtxAttribs2[]={
EGL_CONTEXT_CLIENT_VERSION,3,
// EGL_COLOR_COMPONENT_TYPE_EXT,EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
EGL_CONTEXT_PRIORITY_LEVEL_IMG,EGL_CONTEXT_PRIORITY_REALTIME_NV,
EGL_NONE};
contextegl=eglCreateContext(display,eglconfig,EGL_NO_CONTEXT,anEglCtxAttribs2);
if(contextegl==EGL_NO_CONTEXT){
}
else{
surface=eglCreateWindowSurface(display,eglconfig,NULL,attribut_list);
eglMakeCurrent(display,surface,surface,contextegl);
}}
emscripten_webgl_make_context_current(ctx);
int h=EM_ASM_INT({return parseInt(document.getElementById('pmhig').innerHTML,10);});
int w=h;
win=SDL_CreateWindow("Shader",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,w,h,0);
glCtx=&contextegl;
sources[0]=common_shader_header;
sources[1]=vertex_shader_body;
vtx=compile_shader(GL_VERTEX_SHADER,2,sources);
sources[0]=common_shader_header;
sources[1]=fragment_shader_header;
sources[2]=default_fragment_shader;
sources[3]=fragment_shader_footer;
frag=compile_shader(GL_FRAGMENT_SHADER,4,sources);
shader_program=glCreateProgram();
glAttachShader(shader_program,vtx);
glAttachShader(shader_program,frag);
glLinkProgram(shader_program);
glDeleteShader(vtx);
glDeleteShader(frag);
glReleaseShaderCompiler();
glUseProgram(shader_program);
SDL_Init(SDL_INIT_EVENTS);
t1=steady_clock::now();
viewportSizeX=w;
viewportSizeY=h;
glClearColor(0.0f,1.0f,0.0f,1.0f);
std::vector<std::shared_ptr<libremidi::midi_in>>inputs;
std::vector<std::shared_ptr<libremidi::midi_out>>outputs;
libremidi::observer::callbacks callbacks{
.input_added=[&](int idx, const std::string& id){
std::cout<<"MIDI Input connected: "<<idx<<" - "<<id<<std::endl;
auto input=std::make_shared<libremidi::midi_in>();
input->set_callback([](const libremidi::message& msg){
std::cout<<(int)msg.bytes[0]<<" "
<<(int)msg.bytes[1]<<" "
<<(int)msg.bytes[2]<<" "
<<(double) msg.timestamp<<std::endl;
});
input->open_port(idx);
inputs.push_back(input);
},
.input_removed=[&](int idx,const std::string& id){
},
.output_added=[&](int idx,const std::string& id){
std::cout<<"MIDI Output connected: "<<idx<<" - "<<id<<std::endl;
libremidi::midi_out output{};
output.open_port(idx);
output.send_message(std::vector<unsigned char>{0x90,64,100});
},
.output_removed=[&](int idx,const std::string& id){
}
};
libremidi::observer obs{libremidi::API::EMSCRIPTEN_WEBMIDI,std::move(callbacks)};
emscripten_set_main_loop([]{},0,1);
emscripten_cancel_main_loop();
emscripten_set_main_loop((void(*)())renderFrame,0,1);
}

static void cls_aud(){
if(dev!=0){
SDL_PauseAudioDevice(dev,SDL_TRUE);
SDL_CloseAudioDevice(dev);
dev=0;
}}
static void qu(int rc){
SDL_Quit();
exit(rc);
}
static void opn_aud(){
dev=SDL_OpenAudioDevice(NULL,SDL_FALSE,&wave.spec,NULL,0);
if(!dev){
SDL_FreeWAV(wave.snd);
qu(2);
}
SDL_PauseAudioDevice(dev,SDL_FALSE);
}
Uint8 *wptr;
int lft;
static void SDLCALL bfr(void *unused,Uint8 *stm,int len){
wptr=wave.snd+wave.pos;
lft=wave.slen-wave.pos;
while (lft<=len){
SDL_memcpy(stm,wptr,lft);
stm+=lft;
len-=lft;
wptr=wave.snd;
lft=wave.slen;
wave.pos=0;
}
SDL_memcpy(stm,wptr,len);
wave.pos+=len;
}
char flnm[16];
static void plt(){
SDL_SetMainReady();
if (SDL_Init(SDL_INIT_AUDIO)<0){
qu(1);
SDL_Log("SDL failed to init.");
}
SDL_strlcpy(flnm,"/snd/sample.wav",sizeof(flnm));
if(SDL_LoadWAV(flnm,&wave.spec,&wave.snd,&wave.slen)==NULL){
qu(1);
}
wave.pos=0;
wave.spec.callback=bfr;
opn_aud();
}
extern "C" {
void str(){
strt();
}
void pl(){
plt();
}}
int main(){
EM_ASM({
FS.mkdir("/snd");
FS.mkdir("/shader");
});

return 1;
}
