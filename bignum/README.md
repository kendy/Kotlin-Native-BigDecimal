BigDecimal depends on the 'bignum' native implementation from the Google's
BoringSSL package.  The library can be built both for JNI (Java Native
Interface) for Android, and as a static library for iOS.

The JNI approach is only experimental, mostly for testing, because in a real
app, you'll probably prefer the java.math.BigDecimal; most probably only the
iOS port is interesting for you, so follow the 'iOS' parts of the following.

To build the 'nativebn' lib for the Kendy's BigDecimal port:

# Pre-requirements

To be able to build BoringSSL, you need to install the following:

* cmake
* go lang

# Build BoringSSL

Change dir to 'ios' (or to 'jni' - if you want to build the JNI version).

    cd ios

    git clone git@github.com:google/boringssl.git
    cd boringssl
    mkdir build-arm64
    cd build-arm64

# Configure & build BoringSSL

* For iOS Simulator:

        /Applications/CMake.app/Contents/bin/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 ..
        make -j8

* For iOS (probably - untested yet):

        /Applications/CMake.app/Contents/bin/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 ..

* For JNI (only for testing):

        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-fPIC -GNinja ..
        ninja

# Install

You are done, no particular installation is necessary, the 'cinterops' bits
from build.gradle.kts take care of this.

The following is only for the case you'd like to build the klib by hand or if
you wanted to build Java version of the lib (but you don't have to, in the
Kotlin/JVM, you can use the java.math.BigDecimal right away.

## Manual build of the native library - iOS

If you are interested in details, check the following:

* https://kotlinlang.org/docs/native-libraries.html#library-search-sequence
* https://github.com/JetBrains/kotlin-native/issues/2314

The actual build:

    cd ../..

    ~/.konan/kotlin-native-prebuilt-macos-1.4.30-M1/bin/cinterop -def boringssl.def -o boringssl

    ~/.konan/kotlin-native-prebuilt-macos-1.4.30-M1/bin/klib install boringssl.klib

## Manual build of the JNI library - only for testing from Java

You need to build the nativebn library:

    git clone https://android.googlesource.com/platform/libnativehelper
    git clone https://android.googlesource.com/platform/libcore

Now adapt the A path to the dir where the libcore & libnative helper is
checked out, and run:

    cd jni
    ./build-nativebn.sh

Then install it somewhere reachable, like:

    sudo cp libnativebn.so /usr/lib64/
