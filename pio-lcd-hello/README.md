# ESP32-S3-Touch-LCD-4 PlatformIO Hello

Minimal PlatformIO project for lighting the Waveshare ESP32-S3-Touch-LCD-4 screen.

## Build and Upload

```powershell
cd C:\Users\26466\Desktop\86screen\pio-lcd-hello
pio run -t upload
pio device monitor -b 115200
```

If the board does not enter download mode automatically, hold `BOOT`, press `RESET`, then release `BOOT` when uploading starts.

This project reuses the local `GFX_Library_for_Arduino` bundled in the Waveshare examples folder.
