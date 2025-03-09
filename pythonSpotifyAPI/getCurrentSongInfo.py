#python -m PyInstaller --onefile --windowed getCurrentSongInfo.py

import spotipy
from spotipy.oauth2 import SpotifyOAuth
import requests
from PIL import Image, ImageDraw
import os
import time

# Spotify API Credentials
clientID = ""
clientSecret = ""
redirectUri = "http://localhost/"
scope = "user-read-currently-playing"

# Initialize Spotify API
sp = spotipy.Spotify(auth_manager=SpotifyOAuth(client_id=clientID, client_secret=clientSecret, redirect_uri=redirectUri, scope=scope))

# Variable to track last song
last_track_id = None

songEndedNaturally = False
cdCoverPingPong = 9999

def create_cd_effect(image_path, output_path, size=(500, 500), hole_diameter=61):
    # Open the image
    image = Image.open(image_path).convert("RGBA")

    # Resize the image to 500x500
    image = image.resize(size)

    # Create a mask to turn the image into a circle
    mask = Image.new('L', size, 0)
    draw = ImageDraw.Draw(mask)
    draw.ellipse((0, 0, size[0], size[1]), fill=255)

    # Apply the mask to the image
    circular_image = Image.new('RGBA', size)
    circular_image.paste(image, (0, 0), mask)

    # Create a mask for the hole in the middle
    hole_radius = hole_diameter // 2
    hole_mask = Image.new('L', size, 255)
    draw = ImageDraw.Draw(hole_mask)
    center = (size[0] // 2, size[1] // 2)
    draw.ellipse((center[0] - hole_radius, center[1] - hole_radius,
                  center[0] + hole_radius, center[1] + hole_radius), fill=0)

    # Apply the hole mask to the circular image
    final_image = Image.new('RGBA', size)
    final_image.paste(circular_image, (0, 0), hole_mask)

    # Save the final image
    final_image.save(output_path, "PNG")

while True:
    currentTrack = sp.currently_playing()

    if currentTrack and currentTrack.get('item'):
        track_id = currentTrack['item']['id']
        track = currentTrack['item']['name']
        artist = currentTrack['item']['artists'][0]['name']
        cover = currentTrack['item']['album']['images'][0]['url']
        duration = currentTrack['item']['duration_ms'] / 1000  # Convert ms to seconds
        progress = currentTrack['progress_ms'] / 1000  # Convert ms to seconds
        remaining_time = max(duration - progress, 0)  # Ensure non-negative value
        print(str(int(remaining_time/60)) + ":" + str(int(remaining_time)%60))
        # if int(remaining_time/60) == 0 and int(remaining_time)%60 < 2 and songEndedNaturally == False:
        #     print("Song ended naturally")
        #     songEndedNaturally = True
        #     file = open("songChangedFlag1.txt", "w")
        #     file.close()

        if track_id != last_track_id:  # If song has changed
            if songEndedNaturally == False:
                print("Song did not end naturally")
                file = open("songChangedFlag2.txt", "w")
                file.close()
            else:
                songEndedNaturally = False
            print(f"Now Playing: {track} by {artist}")

            # Download and save cover image
            coverImage = requests.get(cover).content
            with open("cdCoverSq.jpg", "wb") as handler:
                handler.write(coverImage)
            coverImage = Image.open("cdCoverSq.jpg")
            coverImage.save("cdCoverSq.png")
            os.remove("cdCoverSq.jpg")
            if cdCoverPingPong == 9999:
                create_cd_effect("cdCoverSq.png", "cdCover9999.png")
                #create_cd_effect("cdCoverSq.png", "cdCoverFast9999.png")
                os.replace("cdCover9999.png", "../assets/cdCover9999.png")
                #os.replace("cdCoverFast9999.png", "../assets/cdCoverFast9999.png")
                cdCoverPingPong = 10000
            else:
                create_cd_effect("cdCoverSq.png", "cdCover10000.png")
                #create_cd_effect("cdCoverSq.png", "cdCoverFast10000.png")
                os.replace("cdCover10000.png", "../assets/cdCover10000.png")
                #os.replace("cdCoverFast10000.png", "../assets/cdCoverFast10000.png")
                cdCoverPingPong = 9999

            last_track_id = track_id  # Update last played track ID

        sleep_time = 1  # Sleep until next song + small buffer
    else:
        sleep_time = 5  # If nothing is playing, check again in 5 seconds

    time.sleep(sleep_time)
