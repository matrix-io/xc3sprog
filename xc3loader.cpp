/*
 * @author @hpsaturn
 * All rights reserved.
 */
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <jni.h>

#include "./android_utils.h"
#include "./xc3sprog.h"

#include <android/log.h>

#define  LOG_TAG    "NDK_DEBUG: "
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

// processing callback to handler class
typedef struct tick_context {
    JavaVM  *javaVM;
    jclass   jniHelperClz;
    jobject  jniHelperObj;
    jclass   mainActivityClz;
    jobject  mainActivityObj;
    JNIEnv  *env;
    jmethodID onFirmwareLoad;
    jmethodID writeTDI;
    jmethodID writeTMS;
    jmethodID writeTCK;
    jmethodID readTDO;
    jmethodID writeLED;
    jmethodID readLED;
    pthread_mutex_t  lock;
    int      done;
} MainContext;
MainContext g_ctx;

using namespace std;
std::string firmware;

void writeTDI(bool state){
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.writeTDI, state);
}

void writeTMS(bool state){
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.writeTMS, state);
}

void writeTCK(bool state){
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.writeTCK, state);
}

bool readTDO(){
  return (g_ctx.env->CallBooleanMethod(g_ctx.jniHelperObj,g_ctx.readTDO));
}

void writeLED(bool state){
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.writeLED, state);
}

bool readLED(){
  return (g_ctx.env->CallBooleanMethod(g_ctx.jniHelperObj,g_ctx.readLED));
}

void testWriteReadFunctions(JNIEnv *env){
  g_ctx.env = env;
  bool state=false;
  for (int i=0;i<6;i++){
    writeLED(state);
    LOGD("-->LED state: %i",state);
    state=!state;
    usleep(100000);
  }
  LOGD("-->LED final state: %i",readLED());
}

extern "C"
JNIEXPORT jint JNICALL Java_admobilize_matrix_gt_XC3Sprog_JNIPrimitives_burnFirmware
(JNIEnv* env, jobject object, jint size )
{
  testWriteReadFunctions(env);
  g_ctx.env = env;
  //fpga_program(firmware);
  return 1;
}

extern "C"
JNIEXPORT jint JNICALL Java_admobilize_matrix_gt_XC3Sprog_JNIPrimitives_loadFirmware
(JNIEnv* env, jobject thiz, jobject ctx, jstring path)
{
  LOGI("==LoadTracker init==");
  LOGD("-->isLittleEndian: %i",isLittleEndian());
  
  firmware = ConvertJString( env, path );
  LOGD("-->loading cascade from: %s",(char*)firmware.c_str());

  LOGI("Loading Java classes..");
  jclass clz = env->FindClass("admobilize/matrix/gt/XC3Sprog/JNICallbacks");
  g_ctx.jniHelperClz = env->NewGlobalRef(clz);
  LOGD("-->JniHandler class founded.");

  jmethodID  jniHelperCtor = env->GetMethodID(g_ctx.jniHelperClz,"<init>", "(Landroid/content/Context;)V");
  jobject    handler = env->NewObject(g_ctx.jniHelperClz,jniHelperCtor,ctx);
  g_ctx.jniHelperObj = env->NewGlobalRef(handler);
  LOGD("-->JniHandler global reference");
 
  g_ctx.onFirmwareLoad = env->GetMethodID(g_ctx.jniHelperClz,"onFirmwareLoad","(I)V");
  if (!g_ctx.onFirmwareLoad) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.writeTDI = env->GetMethodID(g_ctx.jniHelperClz, "writeTDI", "(Z)V");
  if (!g_ctx.writeTDI) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.writeTMS = env->GetMethodID(g_ctx.jniHelperClz, "writeTMS", "(Z)V");
  if (!g_ctx.writeTMS) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.writeTCK = env->GetMethodID(g_ctx.jniHelperClz, "writeTCK", "(Z)V");
  if (!g_ctx.writeTCK) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.readTDO= env->GetMethodID(g_ctx.jniHelperClz, "readTDO", "()Z");
  if (!g_ctx.readTDO) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.writeLED = env->GetMethodID(g_ctx.jniHelperClz, "writeLED", "(Z)V");
  if (!g_ctx.writeLED) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.readLED = env->GetMethodID(g_ctx.jniHelperClz, "readLED", "()Z");
  if (!g_ctx.readLED) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  LOGD("-->RuntimeInfo:");
  queryRuntimeInfo(env, g_ctx.jniHelperObj, g_ctx.jniHelperClz);

  LOGD("-->Main context ready");
  LOGI("==Finish Init==");

  return 1;

}

extern "C"
JNIEXPORT void JNICALL Java_admobilize_matrix_gt_XC3Sprog_stopLoader(JNIEnv* env, jobject thiz)
{
// release object we allocated from StartTicks() function
    env->DeleteGlobalRef(g_ctx.mainActivityClz);
    env->DeleteGlobalRef(g_ctx.mainActivityObj);
    env->DeleteGlobalRef(g_ctx.jniHelperClz);
    env->DeleteGlobalRef(g_ctx.jniHelperObj);
    g_ctx.mainActivityObj = NULL;
    g_ctx.mainActivityClz = NULL;
}

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {

  LOGI("JNI_OnLoad starting..");
  JNIEnv* env;
  memset(&g_ctx, 0, sizeof(g_ctx));
  g_ctx.javaVM = vm;
  LOGD("-->vm context created");
  g_ctx.done = 0;
  g_ctx.mainActivityObj = NULL;
  return  JNI_VERSION_1_6;
}
