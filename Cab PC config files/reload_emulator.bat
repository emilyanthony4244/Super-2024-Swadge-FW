:loop
tasklist |findstr /ibc:"swadge_emulator.exe" || cmd /c "swadge_emulator.exe -f --hide-leds --headless --screensaver"
timeout 1
goto :loop