
#include <libremidi/libremidi.hpp>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
#include <emscripten.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <memory>

using namespace std;
using namespace std::chrono;
using namespace std::literals;
using std::string;
struct timespec s_time={0,10000000};
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
"#version 300 es \n precision highp float;precision highp int;precision lowp sampler3D;precision highp sampler2D;";

static const char vertex_shader_body_gles3[]=
"layout(location=0)in vec3 aPos;layout(location=1)in vec3 aColor;"
"out vec3 ourColor;void main(){gl_Position=vec4(aPos,1.0);ourColor=aColor;} \n\0";

static const char fragment_shader_header_gles3[]=
"in vec3 ourColor; \n"
"out vec4 FragColor; \n";

static const char fragment_shader_footer_gles3[]=
" \n\0";

EGLDisplay display;
EGLContext contextegl;
EGLSurface surface;
EmscriptenWebGLContextAttributes attr;

EMSCRIPTEN_RESULT ret;

static const char* common_shader_header=common_shader_header_gles3;
static const char* vertex_shader_body=vertex_shader_body_gles3;
static const char* fragment_shader_header=fragment_shader_header_gles3;
static const char* fragment_shader_footer=fragment_shader_footer_gles3;

GLuint shader_program;
static GLfloat mouseX;
static GLfloat mouseY;
static GLint mouseLPressed;
static GLint portOpen;
GLfloat mouseRPressed;
static GLfloat viewportSizeX;
static GLfloat viewportSizeY;
static GLfloat abstime;
GLuint compile_shader(GLenum type,GLsizei nsources,const char **sources){
GLuint shader;
GLsizei i,srclens[nsources];
for (i=0;i<nsources;++i){
srclens[i]=(GLsizei)strlen(sources[i]);
}
shader=glCreateShader(type);
glShaderSource(shader,nsources,sources,srclens);
glCompileShader(shader);
return shader;
}
GLfloat F=1.0f;
GLfloat F0=0.0f;
GLfloat Fm1=-1.0f;
GLfloat ink[]={F0,F,F0,F};
GLfloat vertices[2160]={};
GLuint VBO,VAO;
// long double white;
GLfloat white;
GLfloat x,y;
GLfloat siz,outTimeA;
unsigned short a;
float b;
int m1,m2;
unsigned char kl;
unsigned char ll;
int idx;
unsigned short ii;
GLuint vtx,frag;
char *fileloc="/shader/shader1.glsl";
unsigned short kkey;
unsigned char k;
EM_BOOL gotClick=0,gotMouseDown=0,gotMouseUp=0,gotDblClick=0,gotMouseMove=0,gotWheel=0;
unsigned short aa;
unsigned short *glkey=&kkey;
libremidi::midi_out outpu{libremidi::API::EMSCRIPTEN_WEBMIDI,"Emscripten"};
static int h,w;

void midd(int idx,int kll,int com){
kl=kll;
if(portOpen==0){
outpu.open_port(idx);
portOpen=1;
}
if(com==3){
outpu.send_message(std::vector<unsigned char>{0x90,kl,127});
mouseLPressed=mouseLPressed+1;
}
if(com==2){
outpu.send_message(std::vector<unsigned char>{0x80,kl,100});
mouseLPressed=mouseLPressed-1;
}
if(com==1){
for (ll=48;ll<83;ll++){
outpu.send_message(std::vector<unsigned char>{0x80,ll,100});
nanosleep(&s_time,NULL);
}
mouseLPressed=0;
}}

void renderFrame(){
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
siz=0.33;
t2=steady_clock::now();
duration<double>time_spana=duration_cast<duration<double>>(t2 - t1);
outTimeA=time_spana.count();
abstime=outTimeA*1000;
mouseX=x/viewportSizeX;
mouseY=y/viewportSizeY;
ink[0]=mouseX/2;
ink[1]=mouseY;
white=abstime-(round(abstime/1000)*1000);
white=1000/white;
if(mouseLPressed>=1){
for(aa=0;aa<kkey;aa++){
vertices[(*glkey*aa)+3]=vertices[3]+0.2f;
vertices[(*glkey*aa)+4]=vertices[100]+white;
vertices[(*glkey*aa)+5]=vertices[33]+(0.888f*(*glkey/1000));
vertices[(*glkey*aa)+7]=vertices[33]+(0.777f*(*glkey/1000));
}
vertices[(*glkey*aa)+6]=white;
ink[2]=white+0.1f;
siz=0.88;
vertices[7]=1.0f-mouseX;
vertices[1]=1.0f-mouseY;
vertices[13]=1.0f-mouseX;
vertices[10]=1.0f-white;
vertices[11]=1.0f-mouseY;
vertices[2]=white;
vertices[8]=vertices[11];
vertices[32]=white/1.5f;
vertices[38]=white/1.2f;
vertices[0]=vertices[32]-white;
vertices[3]=vertices[1]-white;
vertices[2]=vertices[2]-white;
}else{
for(ii=0;ii<2161;ii++){
vertices[ii]=F0;
}
for(a=0;a<361;a++){
b=(float)a/360;
vertices[a*6]=siz*cos(a);
vertices[(a*6)+1]=siz*sin(a);
vertices[(a*6)+2]=b;
vertices[(a*6)+3]=1.0f;
vertices[(a*6)+4]=white;
vertices[(a*6)+5]=1.0f-b;
};
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
EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB,
EGL_NONE
};

static const EGLint attribute_list[]={
EGL_COLOR_COMPONENT_TYPE_EXT,EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT,EGL_TRUE,
EGL_DEPTH_ENCODING_NV,EGL_DEPTH_ENCODING_NONLINEAR_NV,
EGL_RENDER_BUFFER,EGL_QUADRUPLE_BUFFER_NV,
EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE,EGL_TRUE,
EGL_RED_SIZE,8,
EGL_GREEN_SIZE,8,
EGL_BLUE_SIZE,8,
EGL_ALPHA_SIZE,8,
EGL_DEPTH_SIZE,24,
EGL_STENCIL_SIZE,8,
EGL_BUFFER_SIZE,32,
EGL_NONE
};

void strt(){
emscripten_cancel_main_loop();
h=EM_ASM_INT({return parseInt(document.getElementById('pmhig').innerHTML,10);});
w=h;
for(ii=0;ii<2161;ii++){
vertices[ii]=0.0f;
}
string program_source=read_file_into_str(fileloc);
static const char* default_fragment_shader=program_source.c_str();
static const char *sources[4];
emscripten_webgl_init_context_attributes(&attr);
attr.alpha=EM_TRUE;
attr.stencil=EM_TRUE;
attr.depth=EM_TRUE;
attr.antialias=EM_TRUE;
attr.premultipliedAlpha=EM_FALSE;
attr.preserveDrawingBuffer=EM_FALSE;
attr.enableExtensionsByDefault=EM_TRUE;
attr.powerPreference=EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
attr.failIfMajorPerformanceCaveat=EM_FALSE;
attr.majorVersion=2;
attr.minorVersion=0;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx=emscripten_webgl_create_context("#canvas",&attr);
EGLConfig eglconfig=NULL;
EGLint config_size,major,minor;
display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
eglInitialize(display,&major,&minor);
eglChooseConfig(display,attribute_list,&eglconfig,1,&config_size);
eglBindAPI(EGL_OPENGL_ES_API);
static const EGLint anEglCtxAttribs2[]={
EGL_CONTEXT_CLIENT_VERSION,3,
EGL_CONTEXT_MINOR_VERSION_KHR,0,
EGL_COLOR_COMPONENT_TYPE_EXT,EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
EGL_CONTEXT_PRIORITY_LEVEL_IMG,EGL_CONTEXT_PRIORITY_REALTIME_NV,
EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR,
EGL_CONTEXT_FLAGS_KHR,EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR,
EGL_NONE};
contextegl=eglCreateContext(display,eglconfig,EGL_NO_CONTEXT,anEglCtxAttribs2);
surface=eglCreateWindowSurface(display,eglconfig,NULL,attribut_list);
eglMakeCurrent(display,surface,surface,contextegl);
emscripten_webgl_make_context_current(ctx);
glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT,GL_NICEST);
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
t1=steady_clock::now();
viewportSizeX=w;
viewportSizeY=h;
portOpen=0;
glClearColor(F0,F,F0,F);
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
emscripten_set_main_loop((void(*)())renderFrame,0,0);
}


inline const char *emscripten_event_type_to_string(int eventType){
const char *events[]={"(invalid)","(none)","keypress","keydown","keyup","click","mousedown","mouseup","dblclick","mousemove","wheel","resize","scroll","blur","focus","focusin","focusout","deviceorientation","devicemotion","orientationchange","fullscreenchange","pointerlockchange","visibilitychange","touchstart","touchend","touchmove","touchcancel","gamepadconnected","gamepaddisconnected","beforeunload","batterychargingchange","batterylevelchange","webglcontextlost","webglcontextrestored","(invalid)"};
++eventType;
if(eventType<0)eventType=0;
if(eventType>=sizeof(events)/sizeof(events[0]))eventType=sizeof(events)/sizeof(events[0])-1;
return events[eventType];
}

int interpret_charcode_for_keyevent(int eventType,const EmscriptenKeyboardEvent *e){
if(eventType==EMSCRIPTEN_EVENT_KEYPRESS&&e->which)return e->which;
if(e->charCode)return e->charCode;
if(strlen(e->key)==1)return(int)e->key[0];
if(e->which)return e->which;
return e->keyCode;
}

int number_of_characters_in_utf8_string(const char *str){
if(!str)return 0;
int num_chars=0;
while(*str){
if((*str++&0xC0)!=0x80)++num_chars;
}
return num_chars;
}

int emscripten_key_event_is_printable_character(const EmscriptenKeyboardEvent *keyEvent){
return number_of_characters_in_utf8_string(keyEvent->key)==1;
}

#define TEST_RESULT(x) if (ret != EMSCRIPTEN_RESULT_SUCCESS) printf("%s returned %s.\n",#x);

extern "C" {

EM_BOOL mouse_callback(int eventType,const EmscriptenMouseEvent *e,void *userData){
// printf("%s,screen: (%ld,%ld),client: (%ld,%ld),%s%s%s%s button: %hu,buttons: %hu,movement: (%ld,%ld),target: (%ld,%ld)\n",emscripten_event_type_to_string(eventType),e->screenX,e->screenY,e->clientX,e->clientY,e->ctrlKey ? " CTRL" : "",e->shiftKey ? " SHIFT" : "",e->altKey ? " ALT" : "",e->metaKey ? " META" : "",e->button,e->buttons,e->movementX,e->movementY,e->targetX,e->targetY);
if(e->screenX!=0&&e->screenY!=0&&e->clientX!=0&&e->clientY!=0&&e->targetX!=0&&e->targetY!=0){
if(eventType==EMSCRIPTEN_EVENT_CLICK)gotClick=1;
if(eventType==EMSCRIPTEN_EVENT_MOUSEDOWN&&e->buttons!=0){
mouseLPressed=mouseLPressed+1;
gotMouseDown=1;
}
if(eventType==EMSCRIPTEN_EVENT_MOUSEUP){
mouseLPressed=mouseLPressed-1;
gotMouseUp=1;
}
if(eventType==EMSCRIPTEN_EVENT_MOUSEMOVE&&(e->movementX!=0||e->movementY!=0)){
gotMouseMove=1;
x=e->clientX;
y=e->clientY;
}}
return 0;
}

EM_BOOL wheel_callback(int eventType,const EmscriptenWheelEvent *e,void *userData){
// printf("%s,screen: (%ld,%ld),client: (%ld,%ld),%s%s%s%s button: %hu,buttons: %hu,target: (%ld,%ld),delta:(%g,%g,%g),deltaMode:%lu\n",emscripten_event_type_to_string(eventType),e->mouse.screenX,e->mouse.screenY,e->mouse.clientX,e->mouse.clientY,e->mouse.ctrlKey ? " CTRL" : "",e->mouse.shiftKey ? " SHIFT" : "",e->mouse.altKey ? " ALT" : "",e->mouse.metaKey ? " META" : "",e->mouse.button,e->mouse.buttons,e->mouse.targetX,e->mouse.targetY,(float)e->deltaX,(float)e->deltaY,(float)e->deltaZ,e->deltaMode);
if(e->deltaY>0.f||e->deltaY<0.f){
gotWheel=1;
}
return 0;
}

EM_BOOL key_callback(int eventType,const EmscriptenKeyboardEvent *e,void *userData){
int dom_pk_code=emscripten_compute_dom_pk_code(e->code);
if(e->repeat==true){
return e->keyCode==DOM_VK_F2||e->keyCode==DOM_VK_F3||e->keyCode==DOM_VK_F4||e->keyCode==DOM_VK_F5||e->keyCode==DOM_VK_F6||e->keyCode==DOM_VK_F7||e->keyCode==DOM_VK_F8||e->keyCode==DOM_VK_F9||e->keyCode==DOM_VK_F10||e->keyCode==DOM_VK_F11||e->keyCode==DOM_VK_F12||e->keyCode==DOM_VK_F1||e->keyCode==DOM_VK_BACK_SPACE||(e->keyCode>=DOM_VK_F1&&e->keyCode<=DOM_VK_F24)||e->ctrlKey||e->altKey||eventType==EMSCRIPTEN_EVENT_KEYPRESS||eventType||eventType==EMSCRIPTEN_EVENT_KEYUP;
}
if(e->keyCode==32){midd(m1,0,1);}
if(e->keyCode==112){kkey=kkey+10;k=59;midd(m1,k,3);
}
if(e->keyCode==113){kkey=kkey+20;k=58;midd(m1,k,3);
}
if(e->keyCode==114){kkey=kkey+30;k=57;midd(m1,k,3);
}
if(e->keyCode==115){kkey=kkey+40;k=56;midd(m1,k,3);
}
if(e->keyCode==116){kkey=kkey+50;k=55;midd(m1,k,3);
}
if(e->keyCode==117){kkey=kkey+68;k=54;midd(m1,k,3);
}
// if(e->keyCode==118){kkey=70;k=53;midd(m1,k,3);}
if(e->which==118){kkey=kkey+70;k=53;midd(m1,k,3);
}
if(e->keyCode==119){kkey=kkey+80;k=52;midd(m1,k,3);
}
if(e->keyCode==120){kkey=kkey+90;k=51;midd(m1,k,3);
}
if(e->keyCode==121){kkey=kkey+100;k=50;midd(m1,k,3);
}
if(e->keyCode==122){kkey=kkey+110;k=49;midd(m1,k,3);
}
if(e->keyCode==123){kkey=kkey+120;k=48;midd(m1,k,3);
}
if(e->keyCode==49){kkey=kkey+220;k=71;midd(m1,k,3);}
if(e->keyCode==50){kkey=kkey+210;k=70;midd(m1,k,3);}
if(e->keyCode==51){kkey=kkey+200;k=69;midd(m1,k,3);}
if(e->keyCode==52){kkey=kkey+190;k=68;midd(m1,k,3);}
if(e->keyCode==53){kkey=kkey+180;k=67;midd(m1,k,3);}
if(e->keyCode==54){kkey=kkey+170;k=66;midd(m1,k,3);}
if(e->keyCode==55){kkey=kkey+150;k=65;midd(m1,k,3);}
if(e->keyCode==56){kkey=kkey+140;k=64;midd(m1,k,3);}
if(e->keyCode==57){kkey=kkey+130;k=63;midd(m1,k,3);}
if(e->keyCode==48){kkey=kkey+117;k=62;midd(m1,k,3);}
if(e->keyCode==189){kkey=kkey+107;k=61;midd(m1,k,3);}
if(e->keyCode==187){kkey=kkey+97;k=60;midd(m1,k,3);}
if(e->keyCode==221){kkey=kkey+111;k=72;midd(m1,k,3);}
if(e->keyCode==219){kkey=kkey+222;k=73;midd(m1,k,3);}
if(e->keyCode==80){kkey=kkey+333;k=74;midd(m1,k,3);}
if(e->keyCode==79){kkey=kkey+338;k=75;midd(m1,k,3);}
if(e->keyCode==73){kkey=kkey+343;k=76;midd(m1,k,3);}
if(e->keyCode==85){kkey=kkey+348;k=77;midd(m1,k,3);}
if(e->keyCode==89){kkey=kkey+353;k=78;midd(m1,k,3);}
if(e->keyCode==84){kkey=kkey+358;k=79;midd(m1,k,3);}
if(e->keyCode==82){kkey=kkey+362;k=80;midd(m1,k,3);}
if(e->keyCode==69){kkey=kkey+367;k=81;midd(m1,k,3);}
if(e->keyCode==87){kkey=kkey+372;k=82;midd(m1,k,3);}
if(e->keyCode==81){kkey=kkey+377;k=83;midd(m1,k,3);}
return e->keyCode==DOM_VK_F2||e->keyCode==DOM_VK_F3||e->keyCode==DOM_VK_F4||e->keyCode==DOM_VK_F5||e->keyCode==DOM_VK_F6||e->keyCode==DOM_VK_F7||e->keyCode==DOM_VK_F8||e->keyCode==DOM_VK_F9||e->keyCode==DOM_VK_F10||e->keyCode==DOM_VK_F11||e->keyCode==DOM_VK_F12||e->keyCode==DOM_VK_F1||e->keyCode==DOM_VK_BACK_SPACE||(e->keyCode>=DOM_VK_F1&&e->keyCode<=DOM_VK_F24)||e->ctrlKey||e->altKey||eventType==EMSCRIPTEN_EVENT_KEYPRESS||eventType||eventType==EMSCRIPTEN_EVENT_KEYUP;
// printf("%s, key: \"%s\" (printable: %s), code: \"%s\" = %s (%d), location: %lu,%s%s%s%s repeat: %d, locale: \"%s\", char: \"%s\", charCode: %lu (interpreted: %d), keyCode: %s(%lu), which: %lu\n",emscripten_event_type_to_string(eventType),e->key,emscripten_key_event_is_printable_character(e) ? "true" : "false", e->code,emscripten_dom_pk_code_to_string(dom_pk_code),dom_pk_code,e->location,e->ctrlKey ? " CTRL" : "",e->shiftKey ? " SHIFT" : "",e->altKey ? " ALT" : "",e->metaKey ? " META" : "",e->repeat, e->locale, e->charValue, e->charCode, interpret_charcode_for_keyevent(eventType, e), emscripten_dom_vk_to_string(e->keyCode),e->keyCode,e->which);
return EM_TRUE;
}

EM_BOOL up_callback(int eventType,const EmscriptenKeyboardEvent *e,void *userData){
if(e->keyCode==112){kkey=kkey-10;k=59;midd(m1,k,2);
}
if(e->keyCode==113){kkey=kkey-20;k=58;midd(m1,k,2);
}
if(e->keyCode==114){kkey=kkey-30;k=57;midd(m1,k,2);
}
if(e->keyCode==115){kkey=kkey-40;k=56;midd(m1,k,2);
}
if(e->keyCode==116){kkey=kkey-50;k=55;midd(m1,k,2);
}
if(e->keyCode==117){kkey=kkey-68;k=54;midd(m1,k,2);
}
// if(e->keyCode==118){kkey=70;k=53;midd(m1,k,2);}
if(e->which==118){midd(m1,53,2);kkey=kkey-70;
}
if(e->keyCode==119){kkey=kkey-80;k=52;midd(m1,k,2);
}
if(e->keyCode==120){kkey=kkey-90;k=51;midd(m1,k,2);
}
if(e->keyCode==121){kkey=kkey-100;k=50;midd(m1,k,2);
}
if(e->keyCode==122){kkey=kkey-110;k=49;midd(m1,k,2);
}
if(e->keyCode==123){kkey=kkey-120;k=48;midd(m1,k,2);
}
if(e->keyCode==49){kkey=kkey-220;k=71;midd(m1,k,2);}
if(e->keyCode==50){kkey=kkey-210;k=70;midd(m1,k,2);}
if(e->keyCode==51){kkey=kkey-200;k=69;midd(m1,k,2);}
if(e->keyCode==52){kkey=kkey-190;k=68;midd(m1,k,2);}
if(e->keyCode==53){kkey=kkey-180;k=67;midd(m1,k,2);}
if(e->keyCode==54){kkey=kkey-170;k=66;midd(m1,k,2);}
if(e->keyCode==55){kkey=kkey-150;k=65;midd(m1,k,2);}
if(e->keyCode==56){kkey=kkey-140;k=64;midd(m1,k,2);}
if(e->keyCode==57){kkey=kkey-130;k=63;midd(m1,k,2);}
if(e->keyCode==48){kkey=kkey-117;k=62;midd(m1,k,2);}
if(e->keyCode==189){kkey=kkey-107;k=61;midd(m1,k,2);}
if(e->keyCode==187){kkey=kkey-97;k=60;midd(m1,k,2);}
if(e->keyCode==221){kkey=kkey-111;k=72;midd(m1,k,2);}
if(e->keyCode==219){kkey=kkey-222;k=73;midd(m1,k,2);}
if(e->keyCode==80){kkey=kkey-333;k=74;midd(m1,k,2);}
if(e->keyCode==79){kkey=kkey-338;k=75;midd(m1,k,2);}
if(e->keyCode==73){kkey=kkey-343;k=76;midd(m1,k,2);}
if(e->keyCode==85){kkey=kkey-348;k=77;midd(m1,k,2);}
if(e->keyCode==89){kkey=kkey-353;k=78;midd(m1,k,2);}
if(e->keyCode==84){kkey=kkey-358;k=79;midd(m1,k,2);}
if(e->keyCode==82){kkey=kkey-362;k=80;midd(m1,k,2);}
if(e->keyCode==69){kkey=kkey-367;k=81;midd(m1,k,2);}
if(e->keyCode==87){kkey=kkey-372;k=82;midd(m1,k,2);}
if(e->keyCode==81){kkey=kkey-377;k=83;midd(m1,k,2);}
return e->keyCode==DOM_VK_F2||e->keyCode==DOM_VK_F3||e->keyCode==DOM_VK_F4||e->keyCode==DOM_VK_F5||e->keyCode==DOM_VK_F6||e->keyCode==DOM_VK_F7||e->keyCode==DOM_VK_F8||e->keyCode==DOM_VK_F9||e->keyCode==DOM_VK_F10||e->keyCode==DOM_VK_F11||e->keyCode==DOM_VK_F12||e->keyCode==DOM_VK_F1||e->keyCode==DOM_VK_BACK_SPACE||(e->keyCode>=DOM_VK_F1&&e->keyCode<=DOM_VK_F24)||e->ctrlKey||e->altKey||eventType==EMSCRIPTEN_EVENT_KEYPRESS||eventType||eventType==EMSCRIPTEN_EVENT_KEYUP;
}

}

#include <SDL2/SDL.h>
SDL_AudioDeviceID dev;
struct{Uint8* snd;int pos;Uint32 slen;SDL_AudioSpec spec;}wave;

void cls_aud(){
if(dev!=0){
SDL_PauseAudioDevice(dev,SDL_TRUE);
SDL_CloseAudioDevice(dev);
dev=0;
return;
}}

void qu(int rc){
SDL_Quit();
return;
}

void opn_aud(){
dev=SDL_OpenAudioDevice(NULL,SDL_FALSE,&wave.spec,NULL,0);
if(!dev){
SDL_FreeWAV(wave.snd);
}
SDL_PauseAudioDevice(dev,SDL_FALSE);
return;
}

void SDLCALL bfr(void *unused,Uint8* stm,int len){
Uint8* wptr;
int lft;
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
return;
}

void plt(){
char flnm[24];
SDL_FreeWAV(wave.snd);
SDL_SetMainReady();
if (SDL_Init(SDL_INIT_AUDIO)<0){
qu(1);
}
SDL_strlcpy(flnm,"/snd/sample.wav",sizeof(flnm));
if(SDL_LoadWAV(flnm,&wave.spec,&wave.snd,&wave.slen)==NULL){
qu(1);
}
wave.pos=0;
wave.spec.callback=bfr;
opn_aud();
return;
}

extern "C" {

void str(){
strt();
}
  
void pl(){
plt();
}
  
}

std::vector<std::shared_ptr<libremidi::midi_out>>outputs;
std::vector<std::shared_ptr<libremidi::midi_in>>inputs;

int main(int argc, char**){
EM_ASM({
FS.mkdir("/snd");
FS.mkdir("/shader");
});
libremidi::observer::callbacks callbacks{
.input_added=[&](int idx,const std::string& id){},
.input_removed=[&](int idx,const std::string& id){},
.output_added=[&](int idx,const std::string& id){std::cout<<"MIDI Output: "<<idx<<" - "<<id<<std::endl;m1=idx;},
.output_removed=[&](int idx,const std::string& id){}
};
libremidi::observer obs{libremidi::API::EMSCRIPTEN_WEBMIDI,std::move(callbacks)};
ret=emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,0,key_callback);
ret=emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,0,key_callback);
ret=emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,0,up_callback);
ret=emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
// TEST_RESULT(emscripten_set_click_callback);
ret=emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
// TEST_RESULT(emscripten_set_mousedown_callback);
ret=emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
// TEST_RESULT(emscripten_set_mouseup_callback);
ret=emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
// TEST_RESULT(emscripten_set_mousemove_callback);
// ret=emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
// TEST_RESULT(emscripten_set_dblclick_callback);
// ret=emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,wheel_callback);
// TEST_RESULT(emscripten_set_wheel_callback);
emscripten_set_main_loop([]{},60,1);
}
