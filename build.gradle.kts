plugins {
    kotlin("multiplatform") version "1.4.10"
    id("com.android.library")
    id("kotlin-android-extensions")
}

group = "kendy.math"
version = "1.0-SNAPSHOT"

repositories {
    google()
    jcenter()
    mavenCentral()
}

kotlin {
    android()
    iosX64("ios") {
        binaries {
            framework {
                baseName = "library"
            }
        }
        // Build a native interop from the boringssl library; details here:
        // https://kotlinlang.org/docs/mpp-dsl-reference.html#cinterops
        // The boringssl provides the BIGNUM implementation
        compilations["main"].cinterops {
            compilations["main"].cinterops {
                val boringssl by creating {
                    // Def-file describing the native API.
                    // The default path is src/nativeInterop/cinterop/<interop-name>.def
                    // TODO find a way to not hardcode the path...
                    defFile(project.file("/Volumes/Projects/BigDecimal/bignum/ios/boringssl.def"))

                    // Package to place the Kotlin API generated.
                    packageName("boringssl")

                    // Options to be passed to compiler by cinterop tool.
                    //compilerOpts("-Ipath/to/headers")

                    // Directories for header search (an analogue of the -I<path> compiler option).
                    //includeDirs.allHeaders("path1", "path2")

                    // A shortcut for includeDirs.allHeaders.
                    //includeDirs("include/directory", "another/directory")
                }
            }
        }
    }
    sourceSets {
        val commonMain by getting
        val commonTest by getting {
            dependencies {
                implementation(kotlin("test-common"))
                implementation(kotlin("test-annotations-common"))
            }
        }
        val androidMain by getting {
            dependencies {
                implementation("com.google.android.material:material:1.2.1")
            }
        }
        val androidTest by getting {
            dependencies {
                implementation(kotlin("test-junit"))
                implementation("junit:junit:4.13")
            }
        }
        val iosMain by getting
        val iosTest by getting
    }
}

android {
    compileSdkVersion(29)
    sourceSets["main"].manifest.srcFile("src/androidMain/AndroidManifest.xml")
    defaultConfig {
        minSdkVersion(24)
        targetSdkVersion(29)
    }
}
