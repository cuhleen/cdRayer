# **cdRayer**, a simple-to-edit, lightweight *CD Player Overlay*
<sub>So far only tested on Windows 10.</sub>

#### *FEATURES*
- Spotify sync!
- Transparent background
- Draggable window
- Always on top
- Thoroughly commented code! This app *wants* you to poke it with a stick

#### *CONTROLS*
- *SPACEBAR* - Toggle on/off
- *ESC* - Close app

#### *HOW TO ADD YOUR OWN COVER*
- Step 0: Download cdRayer v.01 or v.011
- Step 1: Have an *image editing software* that has clipping and radial blur <sub>(alternatively, look up "radial blur online" on your browser)</sub>
- Step 2: Open the app folder, *"assets"*, from there open *"cdCover.png"* or *"cdCoverFast.png"*
- Step 3: Add a layer on top. Place your album cover so it is scaled with the window. Then clip
- Step 4: Export as *"cdCover.png"* inside "assets" in the app folder
- Step 5: Apply radial blur to your photo. Repeat steps 3 and 4, but export as *"cdCoverFast.png"*
- You should be done!

#### *HOW TO SYNC TO SPOTIFY*
- Step 0: Download cdRayer v.011 S
- Step 1: Open Spotify for Developers, Log In, Dashboard, Create App, Web API, write "http://localhost/" for Redirect URI.
- Step 2: Open the folder "pythonSpotifyAPI", "getCurrentSongInfo.py", and write your clientID and clientSecret as they appear on your Spotify for Developers project.
- Step 3: Run the program, it'll probably tell you in the terminal to paste some URL you get redirected to. Do that.
- Step 4: In the terminal, paste the command found at the top of the file. That will compile this getCurrentSongInfo script into an exe.
- Step 5: Open getCurrentSongInfo.exe and main.exe. cdRayer should now sync with your Spotify.
- DISCLAIMER - "getCurrentSongInfo.exe" does NOT appear on the taskbar, and it does NOT sync its opening/closing with "main.exe". It must be closed through the task manager.

Suggestions, feedback, or just a conversation are more than welcome on my email ^^ collie.dawg.8075@gmail.com

Almost all declared variables are not set in stone. ***Play around!!***