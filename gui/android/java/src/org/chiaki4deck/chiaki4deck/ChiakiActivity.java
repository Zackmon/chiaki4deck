package org.chiaki4deck.chiaki4deck;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDL;
import org.libsdl.app.SDLAudioManager;
import org.libsdl.app.SDLControllerManager;
import org.libsdl.app.HIDDeviceManager;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.content.Intent;
import android.util.Log;
import org.qtproject.qt.android.QtActivityBase;

public class ChiakiActivity extends QtActivityBase{
    private static final String TAG = "SDL_ChiakiActivity";
    private static final int SDL_MAJOR_VERSION = 2;
    private static final int SDL_MINOR_VERSION = 30;
    private static final int SDL_MICRO_VERSION = 3;

    @Override
        public void onCreate(Bundle savedInstanceState)
        {
            Log.v(TAG, "Device: " + Build.DEVICE);
            Log.v(TAG, "Model: " + Build.MODEL);
            Log.v(TAG, "onCreate()");

            SDL.loadLibrary("SDL2");
            SDL.setupJNI();
            SDL.initialize();
            SDL.setContext( this );
            SDLControllerManager.initialize();
            SDLControllerManager.nativeSetupJNI();
            HIDDeviceManager.acquire(this);


            super.onCreate(savedInstanceState);
            /*Log.d("myTag", "This is chiaki4deck Activity");
            Intent intent = new Intent(this, SDLActivity.class);
            startActivity(intent);*/



        }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        Log.d("MySDLActivity", "onDestroy");
        if (SDL.getContext() == this)
            {
                 SDL.setContext( null );

            }
    }

}
