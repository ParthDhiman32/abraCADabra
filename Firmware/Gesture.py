import asyncio
import threading
import time
import serial
import keyboard
import pyautogui

from winsdk.windows.media.control import \
    GlobalSystemMediaTransportControlsSessionManager as MediaManager

try:
    from ctypes import cast, POINTER
    from comtypes import CLSCTX_ALL
    from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
    HAVE_PYCAW = True
except ImportError:
    HAVE_PYCAW = False

ser = serial.Serial("COM3", 115200, timeout=0.1)

last_track_line = ""
last_stat_line = ""


def get_system_volume_pct():
    if not HAVE_PYCAW:
        return 100
    devices = AudioUtilities.GetSpeakers()
    interface = devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
    volume = cast(interface, POINTER(IAudioEndpointVolume))
    return int(round(volume.GetMasterVolumeLevelScalar() * 100))


async def spotify_task():
    global last_track_line, last_stat_line

    while True:
        try:
            sessions = await MediaManager.request_async()
            current = sessions.get_current_session()

            song = "No Track"
            artist = "--"
            pos_sec = 0
            dur_sec = 0
            playing = 0

            if current:
                info = await current.try_get_media_properties_async()
                song = info.title or "No Track"
                artist = info.artist or "--"

                timeline = current.get_timeline_properties()
                pos_sec = int(timeline.position.total_seconds())
                dur_sec = int(timeline.end_time.total_seconds())

                playback = current.get_playback_info()
                playing = 1 if playback.playback_status == 4 else 0

            volume = get_system_volume_pct()

            # sanitize any '|' in title/artist so it doesn't break the field split
            song_clean = song.replace("|", "-")
            artist_clean = artist.replace("|", "-")

            track_line = f"TRACK:{song_clean}|{artist_clean}|{pos_sec}|{dur_sec}|{volume}|{playing}\n"
            if track_line != last_track_line:
                ser.write(track_line.encode())
                last_track_line = track_line

            clock_str = time.strftime("%H:%M")
            stat_line = f"STAT:{clock_str}|100|1\n"
            if stat_line != last_stat_line:
                ser.write(stat_line.encode())
                last_stat_line = stat_line

        except Exception as e:
            print("spotify_task error:", e)

        await asyncio.sleep(1)


def spotify_loop():
    asyncio.run(spotify_task())


threading.Thread(target=spotify_loop, daemon=True).start()


while True:

    if ser.in_waiting:

        msg = ser.readline().decode(errors="ignore").strip()

        print(msg)

        if msg == "Right":
            keyboard.send("next track")

        elif msg == "Left":
            keyboard.send("previous track")

        elif msg == "Up":
            keyboard.send("alt+tab")

        elif msg == "Down":
            keyboard.send("alt+shift+tab")

        elif msg == "Forward":
            pyautogui.scroll(300)

        elif msg == "Backward":
            pyautogui.scroll(-300)

        elif msg == "Clockwise":
            keyboard.send("ctrl+shift+=")

        elif msg == "anti-clockwise":
            keyboard.send("ctrl+-")

        elif msg == "wave":
            keyboard.send("play/pause media")