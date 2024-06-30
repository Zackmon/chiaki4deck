package org.chiaki4deck.chiaki4deck;

import org.libsdl.app.SDLActivity;
import org.libsdl.app.SDL;
import org.libsdl.app.SDLAudioManager;
import android.os.Bundle;
import android.content.Intent;
import android.util.Log;
import org.qtproject.qt.android.QtActivityBase;

public class ChiakiActivity extends QtActivityBase{
    @Override
        public void onCreate(Bundle savedInstanceState)
        {




            /*SDL.initialize();
            SDL.setContext( this );
            SDLAudioManager.initialize();
            SDLAudioManager.setContext( this );
            */
            Log.d("myTag", "This is chiaki4deck Activity");
            Intent intent = new Intent(this, SDLActivity.class);
            startActivity(intent);
            super.onCreate(savedInstanceState);


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
