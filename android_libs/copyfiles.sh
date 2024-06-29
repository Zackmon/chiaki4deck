#!/bin/bash

mkdir arm64-v8a
cd arm64-v8a
mkdir bin
mkdir include
mkdir lib
mkdir share
cd ..

mkdir armeabi-v7a
cd armeabi-v7a
mkdir bin
mkdir include
mkdir lib
mkdir share
cd ..

mkdir x86
cd x86
mkdir bin
mkdir include
mkdir lib
mkdir share
cd ..

mkdir x86_64
cd x86_64
mkdir bin
mkdir include
mkdir lib
mkdir share
cd ..

cp -r *-android-21-arm64-v8a/bin/* arm64-v8a/bin/
cp -r *-android-21-arm64-v8a/share/* arm64-v8a/share
cp -r *-android-21-arm64-v8a/lib/* arm64-v8a/lib
cp -r *-android-21-arm64-v8a/include/* arm64-v8a/include
cp -r *-android-21-arm64-v8a/.ndk-pkg/dependencies/lib/* arm64-v8a/lib/

cp -r *-android-21-armeabi-v7a/bin/* armeabi-v7a/bin/
cp -r *-android-21-armeabi-v7a/share/* armeabi-v7a/share
cp -r *-android-21-armeabi-v7a/lib/* armeabi-v7a/lib
cp -r *-android-21-armeabi-v7a/include/* armeabi-v7a/include
cp -r *-android-21-armeabi-v7a/.ndk-pkg/dependencies/lib/* armeabi-v7a/lib/

cp -r *-android-21-x86/bin/* x86/bin/
cp -r *-android-21-x86/share/* x86/share
cp -r *-android-21-x86/lib/* x86/lib
cp -r *-android-21-x86/include/* x86/include
cp -r *-android-21-x86/.ndk-pkg/dependencies/lib/* x86/lib/

cp -r *-android-21-x86_64/bin/* x86_64/bin/
cp -r *-android-21-x86_64/share/* x86_64/share
cp -r *-android-21-x86_64/lib/* x86_64/lib
cp -r *-android-21-x86_64/include/* x86_64/include
cp -r *-android-21-x86_64/.ndk-pkg/dependencies/lib/* x86_64/lib/