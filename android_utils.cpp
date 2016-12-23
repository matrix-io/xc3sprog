#include "./android_utils.h"

void queryRuntimeInfo(JNIEnv *env, jobject instance, jclass helper) {

    jmethodID versionFunc = env->GetStaticMethodID(helper,"getBuildVersion", "()Ljava/lang/String;");

    if (!versionFunc) {
        LOGE("Failed to retrieve getBuildVersion() methodID @ line %d",__LINE__);
        return;
    }

    jstring buildVersion = env->CallStaticObjectMethod(helper, versionFunc);
    const char *version = env->GetStringUTFChars(buildVersion, NULL);

    if (!version) {
        LOGE("Unable to get version string @ line %d", __LINE__);
        return;
    }

    LOGD("-->Android Version - %s", version);
    env->ReleaseStringUTFChars(buildVersion, version);

    // we are called from JNI_OnLoad, so got to release it to avoid
    // leaking
    env->DeleteLocalRef(buildVersion);

    jmethodID memFunc = env->GetMethodID(helper,"getRuntimeMemorySize", "()J");

    if (!memFunc) {
        LOGE("Failed to retrieve getRuntimeMemorySize() methodID @ line %d",__LINE__);
        return;
    }

    jlong result = env->CallLongMethod(instance, memFunc);
    LOGD("-->Runtime free memory size: %lld", result);
    (void)result;  // silence the compiler warning
}

std::string ConvertJString(JNIEnv* env, jstring str){

  const jsize len = env->GetStringUTFLength(str);
  const char* strChars = env->GetStringUTFChars(str, (jboolean *)0);
  std::string Result(strChars, len);
  env->ReleaseStringUTFChars(str, strChars);

  return Result;
}

int isLittleEndian(){

  unsigned short word=0x0102;
  unsigned char*  p= (unsigned char*)&word;
  int endianess = int(p[0]==2);
  return endianess;

}

jbyteArray StringToJByteArray(JNIEnv * env, const std::string &nativeString) {
    jbyteArray arr = env->NewByteArray(nativeString.length());
    env->SetByteArrayRegion(arr,0,nativeString.length(),(jbyte*)nativeString.c_str());
    return arr;
}

