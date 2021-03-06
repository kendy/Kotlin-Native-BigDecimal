/*
 * Copyright (C) 2008 The Android Open Source Project
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
 */

#include <cstdio>
#include <algorithm>
#include <memory>

#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>

#include <jni.h>
#include <nativehelper/scoped_utf_chars.h>
#include <nativehelper/scoped_primitive_array.h>

#include "kendy_math_NativeBN.h"

struct BN_CTX_Deleter {
  void operator()(BN_CTX* p) const {
    BN_CTX_free(p);
  }
};
typedef std::unique_ptr<BN_CTX, BN_CTX_Deleter> Unique_BN_CTX;

static BIGNUM* toBigNum(jlong address) {
  return reinterpret_cast<BIGNUM*>(static_cast<uintptr_t>(address));
}

static void DiscardPendingException(JNIEnv* env, const char* className) {
    jthrowable exception = env->ExceptionOccurred();
    env->ExceptionClear();
    if (exception == NULL) {
        return;
    }

    //struct ExpandableString summary;
    //ExpandableStringInitialize(&summary);
    //GetExceptionSummary(env, exception, &summary);
    //const char* details = (summary.data != NULL) ? summary.data : "Unknown";
    //ALOGW("Discarding pending exception (%s) to throw %s", details, className);
    //ExpandableStringRelease(&summary);
    env->DeleteLocalRef(exception);
}

int jniThrowException(JNIEnv* env, const char* className, const char* message) {
    DiscardPendingException(env, className);

    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        //ALOGE("Unable to find exception class %s", className);
        /* ClassNotFoundException now pending */
        return -1;
    }

    int status = 0;
    if (env->ThrowNew(exceptionClass, message) != JNI_OK) {
        //ALOGE("Failed throwing '%s' '%s'", className, message);
        /* an exception, most likely OOM, will now be pending */
        status = -1;
    }
    env->DeleteLocalRef(exceptionClass);

    return status;
}

void jniThrowOutOfMemoryError(JNIEnv* env, const char* message) {
    jniThrowException(env, "java/lang/OutOfMemoryError", message);
}

int jniThrowNullPointerException(JNIEnv* env, const char* msg) {
    return jniThrowException(env, "java/lang/NullPointerException", msg);
}

static void throwException(JNIEnv* env) {
  long error = ERR_get_error();
  // OpenSSL's error queue may contain multiple errors. Clean up after them.
  ERR_clear_error();

  if (error == 0) {
    // An operation failed but did not push to the error queue. Throw a default
    // exception.
    jniThrowException(env, "java/lang/ArithmeticException", "Operation failed");
    return;
  }

  char message[256];
  ERR_error_string_n(error, message, sizeof(message));
  int reason = ERR_GET_REASON(error);
  if (reason == BN_R_DIV_BY_ZERO) {
    jniThrowException(env, "java/lang/ArithmeticException", "BigInteger division by zero");
  } else if (reason == BN_R_NO_INVERSE) {
    jniThrowException(env, "java/lang/ArithmeticException", "BigInteger not invertible");
  } else if (reason == ERR_R_MALLOC_FAILURE) {
    jniThrowOutOfMemoryError(env, message);
  } else {
    jniThrowException(env, "java/lang/ArithmeticException", message);
  }
}

static int isValidHandle(JNIEnv* env, jlong handle, const char* message) {
  if (handle == 0) {
    jniThrowNullPointerException(env, message);
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

static int oneValidHandle(JNIEnv* env, jlong a) {
  return isValidHandle(env, a, "Mandatory handle (first) passed as null");
}

static int twoValidHandles(JNIEnv* env, jlong a, jlong b) {
  if (!oneValidHandle(env, a)) return JNI_FALSE;
  return isValidHandle(env, b, "Mandatory handle (second) passed as null");
}

static int threeValidHandles(JNIEnv* env, jlong a, jlong b, jlong c) {
  if (!twoValidHandles(env, a, b)) return JNI_FALSE;
  return isValidHandle(env, c, "Mandatory handle (third) passed as null");
}

static int fourValidHandles(JNIEnv* env, jlong a, jlong b, jlong c, jlong d) {
  if (!threeValidHandles(env, a, b, c)) return JNI_FALSE;
  return isValidHandle(env, d, "Mandatory handle (fourth) passed as null");
}

extern "C" JNIEXPORT jlong JNICALL Java_kendy_math_NativeBN_BN_1new(JNIEnv* env, jobject) {
  jlong result = static_cast<jlong>(reinterpret_cast<uintptr_t>(BN_new()));
  if (!result) {
    throwException(env);
  }
  return result;
}

extern "C" JNIEXPORT jlong JNICALL Java_kendy_math_NativeBN_getNativeFinalizer(JNIEnv*, jobject) {
  return static_cast<jlong>(reinterpret_cast<uintptr_t>(&BN_free));
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1free(JNIEnv* env, jobject, jlong a) {
  if (!oneValidHandle(env, a)) return;
  BN_free(toBigNum(a));
}

extern "C" JNIEXPORT int JNICALL Java_kendy_math_NativeBN_BN_1cmp(JNIEnv* env, jobject, jlong a, jlong b) {
  if (!twoValidHandles(env, a, b)) return 1;
  return BN_cmp(toBigNum(a), toBigNum(b));
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1copy(JNIEnv* env, jobject, jlong to, jlong from) {
  if (!twoValidHandles(env, to, from)) return;
  if (!BN_copy(toBigNum(to), toBigNum(from))) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_putULongInt(JNIEnv* env, jobject, jlong a0, jlong java_dw, jboolean neg) {
  if (!oneValidHandle(env, a0)) return;

  uint64_t dw = java_dw;
  BIGNUM* a = toBigNum(a0);

  if (!BN_set_u64(a, dw)) {
    throwException(env);
    return;
  }

  BN_set_negative(a, neg);
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_putLongInt(JNIEnv* env, jobject cls, jlong a, jlong dw) {
  if (dw >= 0) {
    Java_kendy_math_NativeBN_putULongInt(env, cls, a, dw, JNI_FALSE);
  } else {
    Java_kendy_math_NativeBN_putULongInt(env, cls, a, -dw, JNI_TRUE);
  }
}

extern "C" JNIEXPORT int JNICALL Java_kendy_math_NativeBN_BN_1dec2bn(JNIEnv* env, jobject, jlong a0, jstring str) {
  if (!oneValidHandle(env, a0)) return -1;
  ScopedUtfChars chars(env, str);
  if (chars.c_str() == NULL) {
    return -1;
  }
  BIGNUM* a = toBigNum(a0);
  int result = BN_dec2bn(&a, chars.c_str());
  if (result == 0) {
    throwException(env);
  }
  return result;
}

extern "C" JNIEXPORT int JNICALL Java_kendy_math_NativeBN_BN_1hex2bn(JNIEnv* env, jobject, jlong a0, jstring str) {
  if (!oneValidHandle(env, a0)) return -1;
  ScopedUtfChars chars(env, str);
  if (chars.c_str() == NULL) {
    return -1;
  }
  BIGNUM* a = toBigNum(a0);
  int result = BN_hex2bn(&a, chars.c_str());
  if (result == 0) {
    throwException(env);
  }
  return result;
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1bin2bn(JNIEnv* env, jobject, jbyteArray arr, int len, jboolean neg, jlong ret) {
  if (!oneValidHandle(env, ret)) return;
  ScopedByteArrayRO bytes(env, arr);
  if (bytes.get() == NULL) {
    return;
  }
  if (!BN_bin2bn(reinterpret_cast<const unsigned char*>(bytes.get()), len, toBigNum(ret))) {
    throwException(env);
    return;
  }

  BN_set_negative(toBigNum(ret), neg);
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_litEndInts2bn(JNIEnv* env, jobject, jintArray arr, int len, jboolean neg, jlong ret0) {
  if (!oneValidHandle(env, ret0)) return;
  BIGNUM* ret = toBigNum(ret0);

  ScopedIntArrayRO scopedArray(env, arr);

  if (scopedArray.get() == NULL) {
    return;
  }

  // We can simply interpret the little-endian integer stream as a
  // little-endian byte stream and use BN_le2bn.
  const uint8_t* tmpBytes = reinterpret_cast<const uint8_t *>(scopedArray.get());
  size_t numBytes = len * sizeof(int);

  if (!BN_le2bn(tmpBytes, numBytes, ret)) {
    throwException(env);
  }

  BN_set_negative(ret, neg);
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_twosComp2bn(JNIEnv* env, jobject, jbyteArray arr, int bytesLen, jlong ret0) {
  if (!oneValidHandle(env, ret0)) return;
  BIGNUM* ret = toBigNum(ret0);

  ScopedByteArrayRO bytes(env, arr);
  if (bytes.get() == NULL) {
    return;
  }

  if (bytesLen == 0) {
    BN_zero(ret);
    return;
  }

  const unsigned char* bytesTmp = reinterpret_cast<const unsigned char*>(bytes.get());

  if (!BN_bin2bn(bytesTmp, bytesLen, ret)) {
    throwException(env);
    return;
  }

  // Use the high bit to determine the sign in twos-complement.
  BN_set_negative(ret, (bytes[0] & 0x80) != 0);

  if (BN_is_negative(ret)) {
    // For negative values, BN_bin2bn doesn't interpret the twos-complement
    // representation, so ret is now (- value - 2^N). We can use nnmod_pow2 to set
    // ret to (-value).
    if (!BN_nnmod_pow2(ret, ret, bytesLen * 8)) {
      throwException(env);
      return;
    }

    // And now we correct the sign.
    BN_set_negative(ret, 1);
  }
}

extern "C" JNIEXPORT jlong JNICALL Java_kendy_math_NativeBN_longInt(JNIEnv* env, jobject, jlong a0) {
  if (!oneValidHandle(env, a0)) return -1;
  BIGNUM* a = toBigNum(a0);
  uint64_t word;

  if (BN_get_u64(a, &word)) {
    return BN_is_negative(a) ? -((jlong) word) : word;
  } else {
    // This should be unreachable if our caller checks BigInt::twosCompFitsIntoBytes(8)
    throwException(env);
    return 0;
  }
}

static char* leadingZerosTrimmed(char* s) {
    char* p = s;
    if (*p == '-') {
        p++;
        while ((*p == '0') && (*(p + 1) != 0)) { p++; }
        p--;
        *p = '-';
    } else {
        while ((*p == '0') && (*(p + 1) != 0)) { p++; }
    }
    return p;
}

extern "C" JNIEXPORT jstring JNICALL Java_kendy_math_NativeBN_BN_1bn2dec(JNIEnv* env, jobject, jlong a) {
  if (!oneValidHandle(env, a)) return NULL;
  char* tmpStr = BN_bn2dec(toBigNum(a));
  if (tmpStr == NULL) {
    throwException(env);
    return NULL;
  }
  char* retStr = leadingZerosTrimmed(tmpStr);
  jstring returnJString = env->NewStringUTF(retStr);
  OPENSSL_free(tmpStr);
  return returnJString;
}

extern "C" JNIEXPORT jstring JNICALL Java_kendy_math_NativeBN_BN_1bn2hex(JNIEnv* env, jobject, jlong a) {
  if (!oneValidHandle(env, a)) return NULL;
  char* tmpStr = BN_bn2hex(toBigNum(a));
  if (tmpStr == NULL) {
    throwException(env);
    return NULL;
  }
  char* retStr = leadingZerosTrimmed(tmpStr);
  jstring returnJString = env->NewStringUTF(retStr);
  OPENSSL_free(tmpStr);
  return returnJString;
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_kendy_math_NativeBN_BN_1bn2bin(JNIEnv* env, jobject, jlong a0) {
  if (!oneValidHandle(env, a0)) return NULL;
  BIGNUM* a = toBigNum(a0);
  jbyteArray result = env->NewByteArray(BN_num_bytes(a));
  if (result == NULL) {
    return NULL;
  }
  ScopedByteArrayRW bytes(env, result);
  if (bytes.get() == NULL) {
    return NULL;
  }
  BN_bn2bin(a, reinterpret_cast<unsigned char*>(bytes.get()));
  return result;
}

extern "C" JNIEXPORT jintArray JNICALL Java_kendy_math_NativeBN_bn2litEndInts(JNIEnv* env, jobject, jlong a0) {
  if (!oneValidHandle(env, a0)) return NULL;

  BIGNUM* a = toBigNum(a0);

  // The number of integers we need is BN_num_bytes(a) / sizeof(int), rounded up
  int intLen = (BN_num_bytes(a) + sizeof(int) - 1) / sizeof(int);

  // Allocate our result with the JNI boilerplate
  jintArray result = env->NewIntArray(intLen);

  if (result == NULL) {
    throwException(env);
    return NULL;
  }

  ScopedIntArrayRW ints(env, result);

  unsigned int* uints = reinterpret_cast<unsigned int*>(ints.get());
  if (uints == NULL) {
    throwException(env);
    return NULL;
  }

  // We can simply interpret a little-endian byte stream as a little-endian integer stream.
  if (!BN_bn2le_padded(reinterpret_cast<uint8_t*>(uints), intLen * sizeof(int), a)) {
    throwException(env);
    return NULL;
  }

  return result;
}

extern "C" JNIEXPORT int JNICALL Java_kendy_math_NativeBN_sign(JNIEnv* env, jobject, jlong a) {
  if (!oneValidHandle(env, a)) return -2;
  if (BN_is_zero(toBigNum(a))) {
      return 0;
  } else if (BN_is_negative(toBigNum(a))) {
    return -1;
  }
  return 1;
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1set_1negative(JNIEnv* env, jobject, jlong b, int n) {
  if (!oneValidHandle(env, b)) return;
  BN_set_negative(toBigNum(b), n);
}

extern "C" JNIEXPORT int JNICALL Java_kendy_math_NativeBN_bitLength(JNIEnv* env, jobject, jlong a0) {
  if (!oneValidHandle(env, a0)) return JNI_FALSE;
  BIGNUM* a = toBigNum(a0);

  // If a is not negative, we can use BN_num_bits directly.
  if (!BN_is_negative(a)) {
    return BN_num_bits(a);
  }

  // In the negative case, the number of bits in a is the same as the number of bits in |a|,
  // except one less when |a| is a power of two.
  BIGNUM positiveA;
  BN_init(&positiveA);

  if (!BN_copy(&positiveA, a)) {
    BN_free(&positiveA);
    throwException(env);
    return -1;
  }

  BN_set_negative(&positiveA, false);
  int numBits = BN_is_pow2(&positiveA) ? BN_num_bits(&positiveA) - 1 : BN_num_bits(&positiveA);

  BN_free(&positiveA);
  return numBits;
}

extern "C" JNIEXPORT jboolean JNICALL Java_kendy_math_NativeBN_BN_1is_1bit_1set(JNIEnv* env, jobject, jlong a, int n) {
  if (!oneValidHandle(env, a)) return JNI_FALSE;

  // NOTE: this is only called in the positive case, so BN_is_bit_set is fine here.
  return BN_is_bit_set(toBigNum(a), n) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1shift(JNIEnv* env, jobject, jlong r, jlong a, int n) {
  if (!twoValidHandles(env, r, a)) return;
  int ok;
  if (n >= 0) {
    ok = BN_lshift(toBigNum(r), toBigNum(a), n);
  } else {
    ok = BN_rshift(toBigNum(r), toBigNum(a), -n);
  }
  if (!ok) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1add_1word(JNIEnv* env, jobject, jlong a, jint w) {
  if (!oneValidHandle(env, a)) return;
  if (!BN_add_word(toBigNum(a), w)) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1mul_1word(JNIEnv* env, jobject, jlong a, jint w) {
  if (!oneValidHandle(env, a)) return;
  if (!BN_mul_word(toBigNum(a), w)) {
    throwException(env);
  }
}

extern "C" JNIEXPORT jint JNICALL Java_kendy_math_NativeBN_BN_1mod_1word(JNIEnv* env, jobject, jlong a, jint w) {
  if (!oneValidHandle(env, a)) return 0;
  BN_ULONG result = BN_mod_word(toBigNum(a), w);
  if (result == (BN_ULONG)-1) {
    throwException(env);
  }
  return result;
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1add(JNIEnv* env, jobject, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  if (!BN_add(toBigNum(r), toBigNum(a), toBigNum(b))) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1sub(JNIEnv* env, jobject, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  if (!BN_sub(toBigNum(r), toBigNum(a), toBigNum(b))) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1gcd(JNIEnv* env, jobject, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_gcd(toBigNum(r), toBigNum(a), toBigNum(b), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1mul(JNIEnv* env, jobject, jlong r, jlong a, jlong b) {
  if (!threeValidHandles(env, r, a, b)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_mul(toBigNum(r), toBigNum(a), toBigNum(b), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1exp(JNIEnv* env, jobject, jlong r, jlong a, jlong p) {
  if (!threeValidHandles(env, r, a, p)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_exp(toBigNum(r), toBigNum(a), toBigNum(p), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1div(JNIEnv* env, jobject, jlong dv, jlong rem, jlong m, jlong d) {
  if (!fourValidHandles(env, (rem ? rem : dv), (dv ? dv : rem), m, d)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_div(toBigNum(dv), toBigNum(rem), toBigNum(m), toBigNum(d), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1nnmod(JNIEnv* env, jobject, jlong r, jlong a, jlong m) {
  if (!threeValidHandles(env, r, a, m)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_nnmod(toBigNum(r), toBigNum(a), toBigNum(m), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1mod_1exp(JNIEnv* env, jobject, jlong r, jlong a, jlong p, jlong m) {
  if (!fourValidHandles(env, r, a, p, m)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_mod_exp(toBigNum(r), toBigNum(a), toBigNum(p), toBigNum(m), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1mod_1inverse(JNIEnv* env, jobject, jlong ret, jlong a, jlong n) {
  if (!threeValidHandles(env, ret, a, n)) return;
  Unique_BN_CTX ctx(BN_CTX_new());
  if (!BN_mod_inverse(toBigNum(ret), toBigNum(a), toBigNum(n), ctx.get())) {
    throwException(env);
  }
}

extern "C" JNIEXPORT void JNICALL Java_kendy_math_NativeBN_BN_1generate_1prime_1ex(JNIEnv* env, jobject, jlong ret, int bits,
                                          jboolean safe, jlong add, jlong rem) {
  if (!oneValidHandle(env, ret)) return;
  if (!BN_generate_prime_ex(toBigNum(ret), bits, safe, toBigNum(add), toBigNum(rem),
                            NULL)) {
    throwException(env);
  }
}

extern "C" JNIEXPORT jboolean JNICALL Java_kendy_math_NativeBN_BN_1primality_1test(JNIEnv* env, jobject, jlong candidate, int checks,
                                           jboolean do_trial_decryption) {
  if (!oneValidHandle(env, candidate)) return JNI_FALSE;
  Unique_BN_CTX ctx(BN_CTX_new());
  int is_probably_prime;
  if (!BN_primality_test(&is_probably_prime, toBigNum(candidate), checks, ctx.get(),
                         do_trial_decryption, NULL)) {
    throwException(env);
    return JNI_FALSE;
  }
  return is_probably_prime ? JNI_TRUE : JNI_FALSE;
}
