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

#include "xc3sprog.h"
#include "./android_utils.h"

#include <android/log.h>

#define  LOG_TAG    "NDK_DEBUG: "
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
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
    jmethodID txrx_block;
    jmethodID txrx;
    jmethodID tx;
    jmethodID tx_tms;
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

void printArrayToHex(const unsigned char *array,int length){
  char buffer[4096];
  memset(buffer,0,4096);
  for(int i=0; i<length; i++)
  {
    sprintf(&buffer[i*2],"%02x",array[i]);
  }
  LOGI("0x%s", buffer);  
}

void java_txrx_block(const unsigned char *tdi, unsigned char *tdo, int length, bool last){
 /* 
  LOGI("java_txrx_block TDI IN:");  
  if(tdi)printArrayToHex(tdi,length);
  LOGI("java_txrx_block TDO IN:");  
  if(tdo)printArrayToHex(tdo,length);
*/
  jbyteArray jtdo = g_ctx.env->NewByteArray(0);
  jbyteArray jtdi = g_ctx.env->NewByteArray(0);

  if(tdi){
    LOGW("PRE txrx_block ==> for TDI length=%d",length);
    jtdi = g_ctx.env->NewByteArray(length);
    //jtdi = (jbyteArray) g_ctx.env->NewDirectByteBuffer((void *)tdi,length);
    LOGW("PRE txrx_block ==> starting TDI SetByteArrayRegion..");
    g_ctx.env->SetByteArrayRegion(jtdi, 0, length, (jbyte *)tdi);
  }

  if (tdo) {
    LOGW("PRE txrx_block ==> for TDO length=%d",length);
    jtdo = g_ctx.env->NewByteArray(length);
    LOGW("PRE txrx_block ==> starting TDO SetByteArrayRegion..");
    g_ctx.env->SetByteArrayRegion(jtdo, 0, length, (jbyte *)tdo);
  }

  LOGW("PRE txrx_block ==> starting txrx_block on java..");
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.txrx_block, jtdo, jtdi, length, last);
  
  if(tdo){
    jbyte *pjtdo = g_ctx.env->GetByteArrayElements(jtdo, 0);
    memcpy(tdo, pjtdo, length);
    LOGW("POST txrx_block ==> tdo length:%d",(int)g_ctx.env->GetArrayLength(jtdo));
    g_ctx.env->ReleaseByteArrayElements(jtdo,pjtdo, 0);
  }
  //LOGD("java_txrx_block: ReleaseByteArrayElements");
}

bool java_txrx(bool tms, bool tdi){
  //LOGW("PRE txrx");
  jbyte out=g_ctx.env->CallByteMethod(g_ctx.jniHelperObj,g_ctx.txrx, tms, tdi); 
  //LOGW("POST txrx");
  if(out==0)return false;
  else return true;
}

void java_tx(bool tms, bool tdi){
  //LOGW("PRE tx");
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.tx, tms, tdi);
  //LOGW("POST tx");
}

void java_tx_tms(unsigned char *pat, int length, int force){
  LOGW("PRE tx_tms");
  jbyteArray jpat = g_ctx.env->NewByteArray(length);
  g_ctx.env->SetByteArrayRegion(jpat, 0, length, (jbyte *)pat);
  g_ctx.env->CallVoidMethod(g_ctx.jniHelperObj,g_ctx.tx_tms, jpat, length, force);
  LOGW("POST tx_tms");
}

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
  LOGI("==Testing READ/WRITE on LED:");
  g_ctx.env = env;
  bool state=false;
  for (int i=0;i<5;i++){
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
  if(detect_chain()){
    LOGI("==Starting FPGA flashing..");
    fpga_program(firmware);
  }
  else LOGE("detect_chain() failed");
  LOGI("==FPGA flashing finish!");
  return 1;
}

extern "C"
JNIEXPORT jint JNICALL Java_admobilize_matrix_gt_XC3Sprog_JNIPrimitives_loadFirmware
(JNIEnv* env, jobject thiz, jobject obj, jstring path)
{
  LOGI("==Load Firmware path ==");
  LOGD("-->isLittleEndian: %i",isLittleEndian());
  
  firmware = ConvertJString( env, path );
  LOGD("-->cascade path: %s",(char*)firmware.c_str());

  LOGI("Loading Java Callbacks..");
  jclass clz = env->FindClass("admobilize/matrix/gt/XC3Sprog/JNIPrimitives");
  g_ctx.jniHelperClz = (jclass)env->NewGlobalRef(clz);
  LOGD("-->JniHandler class founded.");

  LOGI("Loading Java Context..");
  //jmethodID  jniHelperCtor = env->GetMethodID(g_ctx.jniHelperClz,"<init>", "(Landroid/content/Context;)V");
  //jobject    handler = env->NewObject(g_ctx.jniHelperClz,jniHelperCtor,ctx);
  g_ctx.jniHelperObj =  env->NewGlobalRef(obj);
 
  LOGD("-->Loading Java Methods..");
  g_ctx.onFirmwareLoad = env->GetMethodID(g_ctx.jniHelperClz,"onFirmwareLoad","(I)V");
  if (!g_ctx.onFirmwareLoad) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.txrx_block = env->GetMethodID(g_ctx.jniHelperClz, "txrx_block", "([B[BIZ)V");
  if (!g_ctx.txrx_block) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.txrx = env->GetMethodID(g_ctx.jniHelperClz, "txrx", "(ZZ)B");
  if (!g_ctx.txrx) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.tx = env->GetMethodID(g_ctx.jniHelperClz, "tx", "(ZZ)V");
  if (!g_ctx.tx) {
    LOGE("Failed to retrieve methodID @ line %d",__LINE__);
    return 0;
  }

  g_ctx.tx_tms = env->GetMethodID(g_ctx.jniHelperClz, "tx_tms", "([BII)V");
  if (!g_ctx.tx_tms) {
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
  g_ctx.env=env;

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
