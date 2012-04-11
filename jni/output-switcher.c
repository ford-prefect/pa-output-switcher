/*
 * Copyright (C) 2012 Collabora Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <jni.h>

jboolean Java_org_pulseaudio_outputswitcher_OutputSwitcher_switchToNetwork(JNIEnv* env, jobject thiz)
{
    return JNI_TRUE;
}

jboolean Java_org_pulseaudio_outputswitcher_OutputSwitcher_switchToLocal(JNIEnv* env, jobject thiz)
{
    return JNI_TRUE;
}
