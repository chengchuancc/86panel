$gitCmd = "C:\Program Files\Git\cmd"

if (Test-Path $gitCmd) {
  $env:Path = "$gitCmd;$env:Path"
}

& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
