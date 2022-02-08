
#include <libremidi/libremidi.hpp>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#define __gl2_h_
#include <GLES2/gl2ext.h>
#include <vector>
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
srclens[i]=(GLsizei)strlen(sources[i]);
}
shader=glCreateShader(type);
glShaderSource(shader,nsources,sources,srclens);
glCompileShader(shader);
return shader;
}

GLfloat ink[]={0.0f,1.0f,0.0f,1.0f};
GLfloat vertices[2160]={};
GLuint VBO,VAO;
// long double white;
GLfloat white;
GLfloat x,y;
long double siz,outTimeA;
int a;
float b;
int m1,m2;

void noteOnGL(int note){
int aa;
for(aa=0;aa<note;aa++){
vertices[(note*aa)+3]=vertices[3]+0.2f;
vertices[(note*aa)+4]=vertices[100]+white;
vertices[(note*aa)+5]=vertices[33]+0.2f;
}
vertices[(note*aa)+6]=white;
}

void noteOffGL(int note){
/*int aab;
for(aab=0;aab<note;aab++){
vertices[(note*aab)+3]=vertices[3];
vertices[(note*aab)+4]=vertices[33];
vertices[(note*aab)+5]=vertices[333];
}*/}

void renderFrame(){
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
siz=0.33;
t2=steady_clock::now();
duration<double> time_spana=duration_cast<duration<double>>(t2 - t1);
outTimeA=time_spana.count();
abstime=outTimeA*1000;
mouseX=x/viewportSizeX;
mouseY=y/viewportSizeY;
ink[0]=mouseX/2;
ink[1]=mouseY;
white=abstime-(round(abstime/1000)*1000);
white=1000/white;
if(mouseLPressed==1.0f){
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

const EGLint attribut_list[]={
// EGL_GL_COLORSPACE_KHR,EGL_GL_COLORSPACE_SRGB,
EGL_NONE
};

const EGLint attribute_list[]={
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

void strt(){
emscripten_cancel_main_loop();
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
eglChooseConfig(display,attribute_list,&eglconfig,1,&config_size);
eglBindAPI(EGL_OPENGL_ES_API);
EGLint anEglCtxAttribs2[]={
EGL_CONTEXT_CLIENT_VERSION,3,
// EGL_COLOR_COMPONENT_TYPE_EXT,EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
EGL_CONTEXT_PRIORITY_LEVEL_IMG,EGL_CONTEXT_PRIORITY_REALTIME_NV,
EGL_NONE};
contextegl=eglCreateContext(display,eglconfig,EGL_NO_CONTEXT,anEglCtxAttribs2);
surface=eglCreateWindowSurface(display,eglconfig,NULL,attribut_list);
eglMakeCurrent(display,surface,surface,contextegl);
emscripten_webgl_make_context_current(ctx);
int h=EM_ASM_INT({return parseInt(document.getElementById('pmhig').innerHTML,10);});
int w=h;
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
glClearColor(0.0f,1.0f,0.0f,1.0f);
glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
emscripten_set_main_loop((void(*)())renderFrame,0,0);
}

extern "C" {
void str(){
strt();
}}

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

void midd(int idx,unsigned char k){
libremidi::midi_out outpu{libremidi::API::EMSCRIPTEN_WEBMIDI, "Emscripten"};
outpu.open_port(idx);
EM_ASM({console.log("note off");});
outpu.send_message(std::vector<unsigned char>{0x80,k,100});
}

void midd2(int idx,unsigned char k){
libremidi::midi_out outp{libremidi::API::EMSCRIPTEN_WEBMIDI, "Emscripten"};
outp.open_port(idx);
// EM_ASM({console.log("note on");});
outp.send_message(std::vector<unsigned char>{0x90,k,100});
}

void midd3(int idx){
libremidi::midi_out outp{libremidi::API::EMSCRIPTEN_WEBMIDI, "Emscripten"};
outp.open_port(idx);
// EM_ASM({console.log("note on");});
for (unsigned char ll=48;ll<83;ll++){
outp.send_message(std::vector<unsigned char>{0x80,ll,100});
nanosleep(&s_time,NULL);
}}

int emscripten_key_event_is_printable_character(const EmscriptenKeyboardEvent *keyEvent){
return number_of_characters_in_utf8_string(keyEvent->key)==1;
}

unsigned char k;
int kkey;

EM_BOOL up_callback(int eventType,const EmscriptenKeyboardEvent *e,void *userData){
if(e->repeat==true){return true;}
if(e->keyCode==112){k=59;midd(m1,k);kkey=10;noteOffGL(kkey);}
if(e->keyCode==113){k=58;midd(m1,k);kkey=20;noteOffGL(kkey);}
if(e->keyCode==114){k=57;midd(m1,k);kkey=30;noteOffGL(kkey);}
if(e->keyCode==115){k=56;midd(m1,k);kkey=40;noteOffGL(kkey);}
if(e->keyCode==116){k=55;midd(m1,k);kkey=50;noteOffGL(kkey);}
if(e->keyCode==117){k=54;midd(m1,k);kkey=60;noteOffGL(kkey);}
if(e->keyCode==118){k=53;midd(m1,k);kkey=70;noteOffGL(kkey);}
if(e->keyCode==119){k=52;midd(m1,k);kkey=80;noteOffGL(kkey);}
if(e->keyCode==120){k=51;midd(m1,k);kkey=90;noteOffGL(kkey);}
if(e->keyCode==121){k=50;midd(m1,k);kkey=100;noteOffGL(kkey);}
if(e->keyCode==122){k=49;midd(m1,k);kkey=110;noteOffGL(kkey);}
if(e->keyCode==123){k=48;midd(m1,k);kkey=120;noteOffGL(kkey);}
  
if(e->keyCode==49){k=71;midd(m1,k);kkey=220;noteOffGL(kkey);}
if(e->keyCode==50){k=70;midd(m1,k);kkey=210;noteOffGL(kkey);}
if(e->keyCode==51){k=69;midd(m1,k);kkey=200;noteOffGL(kkey);}
if(e->keyCode==52){k=68;midd(m1,k);kkey=190;noteOffGL(kkey);}
if(e->keyCode==53){k=67;midd(m1,k);kkey=180;noteOffGL(kkey);}
if(e->keyCode==54){k=66;midd(m1,k);kkey=170;noteOffGL(kkey);}
if(e->keyCode==55){k=65;midd(m1,k);kkey=150;noteOffGL(kkey);}
if(e->keyCode==56){k=64;midd(m1,k);kkey=140;noteOffGL(kkey);}
if(e->keyCode==57){k=63;midd(m1,k);kkey=130;noteOffGL(kkey);}
if(e->keyCode==48){k=62;midd(m1,k);kkey=117;noteOffGL(kkey);}
if(e->keyCode==189){k=61;midd(m1,k);kkey=107;noteOffGL(kkey);}
if(e->keyCode==187){k=60;midd(m1,k);kkey=97;noteOffGL(kkey);}
  
if(e->keyCode==221){k=72;midd(m1,k);kkey=111;noteOffGL(kkey);}
if(e->keyCode==219){k=73;midd(m1,k);kkey=222;noteOffGL(kkey);}
if(e->keyCode==80){k=74;midd(m1,k);kkey=333;noteOffGL(kkey);}
if(e->keyCode==79){k=75;midd(m1,k);kkey=338;noteOffGL(kkey);}
if(e->keyCode==73){k=76;midd(m1,k);kkey=343;noteOffGL(kkey);}
if(e->keyCode==85){k=77;midd(m1,k);kkey=348;noteOffGL(kkey);}
if(e->keyCode==89){k=78;midd(m1,k);kkey=353;noteOffGL(kkey);}
if(e->keyCode==84){k=79;midd(m1,k);kkey=358;noteOffGL(kkey);}
if(e->keyCode==82){k=80;midd(m1,k);kkey=362;noteOffGL(kkey);}
if(e->keyCode==69){k=81;midd(m1,k);kkey=367;noteOffGL(kkey);}
if(e->keyCode==87){k=82;midd(m1,k);kkey=372;noteOffGL(kkey);}
if(e->keyCode==81){k=83;midd(m1,k);kkey=377;noteOffGL(kkey);}

mouseLPressed=0.0f;
return true;
}

EM_BOOL key_callback(int eventType,const EmscriptenKeyboardEvent *e,void *userData){
int dom_pk_code=emscripten_compute_dom_pk_code(e->code);

if(e->repeat==true){return true;}
if(e->keyCode==32){midd3(m1);}
if(e->keyCode==112){k=59;midd2(m1,k);kkey=10;noteOnGL(kkey);}
if(e->keyCode==113){k=58;midd2(m1,k);kkey=20;noteOnGL(kkey);}
if(e->keyCode==114){k=57;midd2(m1,k);kkey=30;noteOnGL(kkey);}
if(e->keyCode==115){k=56;midd2(m1,k);kkey=40;noteOnGL(kkey);}
if(e->keyCode==116){k=55;midd2(m1,k);kkey=50;noteOnGL(kkey);}
if(e->keyCode==117){k=54;midd2(m1,k);kkey=60;noteOnGL(kkey);}
if(e->keyCode==118){k=53;midd2(m1,k);kkey=70;noteOnGL(kkey);}
if(e->keyCode==119){k=52;midd2(m1,k);kkey=80;noteOnGL(kkey);}
if(e->keyCode==120){k=51;midd2(m1,k);kkey=90;noteOnGL(kkey);}
if(e->keyCode==121){k=50;midd2(m1,k);kkey=100;noteOnGL(kkey);}
if(e->keyCode==122){k=49;midd2(m1,k);kkey=110;noteOnGL(kkey);}
if(e->keyCode==123){k=48;midd2(m1,k);kkey=120;noteOnGL(kkey);}
  
if(e->keyCode==49){k=71;midd2(m1,k);kkey=220;noteOnGL(kkey);}
if(e->keyCode==50){k=70;midd2(m1,k);kkey=210;noteOnGL(kkey);}
if(e->keyCode==51){k=69;midd2(m1,k);kkey=200;noteOnGL(kkey);}
if(e->keyCode==52){k=68;midd2(m1,k);kkey=190;noteOnGL(kkey);}
if(e->keyCode==53){k=67;midd2(m1,k);kkey=180;noteOnGL(kkey);}
if(e->keyCode==54){k=66;midd2(m1,k);kkey=170;noteOnGL(kkey);}
if(e->keyCode==55){k=65;midd2(m1,k);kkey=150;noteOnGL(kkey);}
if(e->keyCode==56){k=64;midd2(m1,k);kkey=140;noteOnGL(kkey);}
if(e->keyCode==57){k=63;midd2(m1,k);kkey=130;noteOnGL(kkey);}
if(e->keyCode==48){k=62;midd2(m1,k);kkey=117;noteOnGL(kkey);}
if(e->keyCode==189){k=61;midd2(m1,k);kkey=107;noteOnGL(kkey);}
if(e->keyCode==187){k=60;midd2(m1,k);kkey=97;noteOnGL(kkey);}
  
if(e->keyCode==221){k=72;midd2(m1,k);kkey=111;noteOnGL(kkey);}
if(e->keyCode==219){k=73;midd2(m1,k);kkey=222;noteOnGL(kkey);}
if(e->keyCode==80){k=74;midd2(m1,k);kkey=333;noteOnGL(kkey);}
if(e->keyCode==79){k=75;midd2(m1,k);kkey=338;noteOnGL(kkey);}
if(e->keyCode==73){k=76;midd2(m1,k);kkey=343;noteOnGL(kkey);}
if(e->keyCode==85){k=77;midd2(m1,k);kkey=348;noteOnGL(kkey);}
if(e->keyCode==89){k=78;midd2(m1,k);kkey=353;noteOnGL(kkey);}
if(e->keyCode==84){k=79;midd2(m1,k);kkey=358;noteOnGL(kkey);}
if(e->keyCode==82){k=80;midd2(m1,k);kkey=362;noteOnGL(kkey);}
if(e->keyCode==69){k=81;midd2(m1,k);kkey=367;noteOnGL(kkey);}
if(e->keyCode==87){k=82;midd2(m1,k);kkey=372;noteOnGL(kkey);}
if(e->keyCode==81){k=83;midd2(m1,k);kkey=377;noteOnGL(kkey);}
  
mouseLPressed=1.0f;
printf("%s, key: \"%s\" (printable: %s), code: \"%s\" = %s (%d), location: %lu,%s%s%s%s repeat: %d, locale: \"%s\", char: \"%s\", charCode: %lu (interpreted: %d), keyCode: %s(%lu), which: %lu\n",emscripten_event_type_to_string(eventType),e->key,emscripten_key_event_is_printable_character(e) ? "true" : "false", e->code,emscripten_dom_pk_code_to_string(dom_pk_code),dom_pk_code,e->location,e->ctrlKey ? " CTRL" : "",e->shiftKey ? " SHIFT" : "",e->altKey ? " ALT" : "",e->metaKey ? " META" : "",e->repeat, e->locale, e->charValue, e->charCode, interpret_charcode_for_keyevent(eventType, e), emscripten_dom_vk_to_string(e->keyCode),e->keyCode,e->which);
// if(eventType==EMSCRIPTEN_EVENT_KEYUP)printf("\n");
return e->keyCode==DOM_VK_F2||e->keyCode==DOM_VK_F3||e->keyCode==DOM_VK_F4||e->keyCode==DOM_VK_F5||e->keyCode==DOM_VK_F6||e->keyCode==DOM_VK_F7||e->keyCode==DOM_VK_F8||e->keyCode==DOM_VK_F9||e->keyCode==DOM_VK_F10||e->keyCode==DOM_VK_F11||e->keyCode==DOM_VK_F12||e->keyCode==DOM_VK_F1||e->keyCode==DOM_VK_BACK_SPACE||(e->keyCode>=DOM_VK_F1&&e->keyCode<=DOM_VK_F24)||e->ctrlKey||e->altKey||eventType==EMSCRIPTEN_EVENT_KEYPRESS||eventType||eventType==EMSCRIPTEN_EVENT_KEYUP;
}
#define TEST_RESULT(x) if (ret != EMSCRIPTEN_RESULT_SUCCESS) printf("%s returned %s.\n",#x);

int gotClick=0,gotMouseDown=0,gotMouseUp=0,gotDblClick=0,gotMouseMove=0,gotWheel=0;

EM_BOOL mouse_callback(int eventType,const EmscriptenMouseEvent *e,void *userData){
printf("%s,screen: (%ld,%ld),client: (%ld,%ld),%s%s%s%s button: %hu,buttons: %hu,movement: (%ld,%ld),target: (%ld,%ld)\n",emscripten_event_type_to_string(eventType),e->screenX,e->screenY,e->clientX,e->clientY,e->ctrlKey ? " CTRL" : "",e->shiftKey ? " SHIFT" : "",e->altKey ? " ALT" : "",e->metaKey ? " META" : "",e->button,e->buttons,e->movementX,e->movementY,e->targetX,e->targetY);
if(e->screenX!=0&&e->screenY!=0&&e->clientX!=0&&e->clientY!=0&&e->targetX!=0&&e->targetY!=0){
if(eventType==EMSCRIPTEN_EVENT_CLICK)gotClick=1;
if(eventType==EMSCRIPTEN_EVENT_MOUSEDOWN&&e->buttons!=0){
gotMouseDown=1;
mouseLPressed=1.0f;
}
if(eventType==EMSCRIPTEN_EVENT_MOUSEUP){
gotMouseUp=1;
mouseLPressed=0.0f;
}
if(eventType==EMSCRIPTEN_EVENT_MOUSEMOVE&&(e->movementX!=0||e->movementY!=0)){
gotMouseMove=1;
x=e->clientX;
y=e->clientY;
}}
return 0;
}

EM_BOOL wheel_callback(int eventType,const EmscriptenWheelEvent *e,void *userData){
printf("%s,screen: (%ld,%ld),client: (%ld,%ld),%s%s%s%s button: %hu,buttons: %hu,target: (%ld,%ld),delta:(%g,%g,%g),deltaMode:%lu\n",emscripten_event_type_to_string(eventType),e->mouse.screenX,e->mouse.screenY,e->mouse.clientX,e->mouse.clientY,e->mouse.ctrlKey ? " CTRL" : "",e->mouse.shiftKey ? " SHIFT" : "",e->mouse.altKey ? " ALT" : "",e->mouse.metaKey ? " META" : "",e->mouse.button,e->mouse.buttons,e->mouse.targetX,e->mouse.targetY,(float)e->deltaX,(float)e->deltaY,(float)e->deltaZ,e->deltaMode);
if(e->deltaY>0.f||e->deltaY<0.f){
gotWheel=1;
}
return 0;
}
int main(){
EM_ASM({FS.mkdir("/snd");FS.mkdir("/shader");});
std::vector<std::shared_ptr<libremidi::midi_in>>inputs;
std::vector<std::shared_ptr<libremidi::midi_out>>outputs;
libremidi::observer::callbacks callbacks{
.input_added=[&](int idx, const std::string& id){},
.input_removed=[&](int idx,const std::string& id){},
.output_added=[&](int idx,const std::string& id){
std::cout<<"MIDI Output connected: "<<idx<<" - "<<id<<std::endl;
m1=idx;
},
.output_removed=[&](int idx,const std::string& id){
}};
EMSCRIPTEN_RESULT ret=emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
TEST_RESULT(emscripten_set_click_callback);
ret=emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
TEST_RESULT(emscripten_set_mousedown_callback);
ret=emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
TEST_RESULT(emscripten_set_mouseup_callback);
ret=emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
TEST_RESULT(emscripten_set_dblclick_callback);
ret=emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,mouse_callback);
TEST_RESULT(emscripten_set_mousemove_callback);
ret=emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,wheel_callback);
TEST_RESULT(emscripten_set_wheel_callback);
emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,key_callback);
emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,up_callback);
emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,0,1,key_callback);
libremidi::observer obs{libremidi::API::EMSCRIPTEN_WEBMIDI,std::move(callbacks)};
emscripten_set_main_loop([]{},60,1);
return 1;
}
