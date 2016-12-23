/*
 * @author @hpsaturn
 * All rights reserved.
 */
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <jni.h>

#include "./android_utils.h"

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
    jmethodID onSendFace;
    jmethodID onFacesCount;
    jmethodID onFinishDetection;
    jmethodID onTrackerFrame;
    pthread_mutex_t  lock;
    int      done;
} MainContext;
MainContext g_ctx;

using namespace std;

extern "C"
JNIEXPORT jint JNICALL Java_admobilize_matrix_gt_XC3Sprog_burnFirmware
(JNIEnv* env, jobject object, jint size )
{


}

extern "C"
JNIEXPORT jint JNICALL Java_admobilize_matrix_gt_XC3Sprog_loadFirmware
(JNIEnv* env, jobject thiz, jobject ctx, jstring path)
{
  LOGI("==LoadTracker init==");
  LOGD("-->isLittleEndian: %i",isLittleEndian());
  
  std::string cascadepath = ConvertJString( env, path );
  LOGD("-->loading cascade from: %s",(char*)cascadepath.c_str());

  LOGI("Loading Java classes..");
  jclass clz = env->FindClass("admobilize/matrix/gt/XC3Sprog/JNICallBacks");
  g_ctx.jniHelperClz = env->NewGlobalRef(clz);
  LOGD("-->JniHandler class founded.");

  jmethodID  jniHelperCtor = env->GetMethodID(g_ctx.jniHelperClz,"<init>", "(Landroid/content/Context;)V");
  jobject    handler = env->NewObject(g_ctx.jniHelperClz,jniHelperCtor,ctx);
  g_ctx.jniHelperObj = env->NewGlobalRef(handler);
  LOGD("-->JniHandler global reference");

 
  g_ctx.onFacesCount = env->GetMethodID(g_ctx.jniHelperClz,"onFirmwareLoad","(I)V");
  if (!g_ctx.onFacesCount) {
    LOGE("Failed to retrieve getRuntimeMemorySize() methodID @ line %d",__LINE__);
    return;
  }

  LOGD("-->RuntimeInfo:");
  queryRuntimeInfo(env, g_ctx.jniHelperObj, g_ctx.jniHelperClz);

  LOGD("-->Main context ready");
  LOGI("==Finish Init==");

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
