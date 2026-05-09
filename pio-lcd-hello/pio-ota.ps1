$gitCmd = "C:\Program Files\Git\cmd"
$uploadPort = if ($args.Count -gt 0) { $args[0] } else { "home-net-panel.local" }

if (Test-Path $gitCmd) {
  $env:Path = "$gitCmd;$env:Path"
}

& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-s3-touch-lcd-4-ota --target upload --upload-port $uploadPort
