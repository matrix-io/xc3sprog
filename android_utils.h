#include <string.h>
#include <iostream>
#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    "NDK_DEBUG: "
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


void queryRuntimeInfo(JNIEnv *env, jobject instance, jclass helper);
std::string ConvertJString(JNIEnv* env, jstring str);
int isLittleEndian();
jbyteArray StringToJByteArray(JNIEnv * env, const std::string &nativeString);
