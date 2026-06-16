#include <jni.h>
#include <string>
#include "../../../../../gamemodel.h"

static GameModel* model = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_init(JNIEnv* env, jobject thiz) {
    if (model) delete model;
    model = new GameModel();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_reset(JNIEnv* env, jobject thiz) {
    if (model) model->reset();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_update(JNIEnv* env, jobject thiz, jdouble delta_time) {
    if (model) model->update(delta_time);
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_setDirection(JNIEnv* env, jobject thiz, jint dir) {
    if (model) model->setPacmanNextDirection(static_cast<Direction>(dir));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_pacman_PacmanGame_getScore(JNIEnv* env, jobject thiz) {
    return model ? model->getScore() : 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_pacman_PacmanGame_getLives(JNIEnv* env, jobject thiz) {
    return model ? model->getLives() : 0;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_isGameOver(JNIEnv* env, jobject thiz) {
    return model ? model->isGameOver() : true;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_isDying(JNIEnv* env, jobject thiz) {
    return model ? model->isDying() : false;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_pacman_PacmanGame_getTile(JNIEnv* env, jobject thiz, jint x, jint y) {
    return model ? static_cast<jint>(model->getTile(x, y)) : 0;
}

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_pacman_PacmanGame_getPacmanPos(JNIEnv* env, jobject thiz) {
    jdoubleArray result = env->NewDoubleArray(3);
    if (model) {
        const Entity& p = model->getPacman();
        double arr[3] = { p.x, p.y, static_cast<double>(p.dir) };
        env->SetDoubleArrayRegion(result, 0, 3, arr);
    }
    return result;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_pacman_PacmanGame_getGhosts(JNIEnv* env, jobject thiz) {
    if (!model) return nullptr;
    const auto& ghosts = model->getGhosts();
    jclass doubleArrayClass = env->FindClass("[D");
    jobjectArray result = env->NewObjectArray(ghosts.size(), doubleArrayClass, nullptr);

    for (size_t i = 0; i < ghosts.size(); ++i) {
        const Entity& g = ghosts[i];
        jdoubleArray arr = env->NewDoubleArray(4);
        double data[4] = { g.x, g.y, static_cast<double>(g.dir), g.frightened ? 1.0 : 0.0 };
        env->SetDoubleArrayRegion(arr, 0, 4, data);
        env->SetObjectArrayElement(result, i, arr);
        env->DeleteLocalRef(arr);
    }
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_startRecording(JNIEnv* env, jobject thiz) {
    if (model) model->startRecording();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_stopRecording(JNIEnv* env, jobject thiz) {
    if (model) model->stopRecording();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_startPlayback(JNIEnv* env, jobject thiz) {
    if (model) model->startPlayback();
}

extern "C" JNIEXPORT void JNICALL
Java_com_pacman_PacmanGame_clearRecording(JNIEnv* env, jobject thiz) {
    if (model) model->clearRecording();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_hasRecording(JNIEnv* env, jobject thiz) {
    return model ? model->hasRecording() : false;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_isReplaying(JNIEnv* env, jobject thiz) {
    return model ? model->isReplaying() : false;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_isPowerMode(JNIEnv* env, jobject thiz) {
    return model ? model->isPowerMode() : false;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_isScatterMode(JNIEnv* env, jobject thiz) {
    return model ? model->isScatterMode() : false;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_pacman_PacmanGame_hasWon(JNIEnv* env, jobject thiz) {
    return model ? model->hasWon() : false;
}
