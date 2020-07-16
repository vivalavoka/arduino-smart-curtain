# arduino-smart-curtain
Remote curtains control by IR receiver

## arduino-cli

**Create skecth**
`arduino-cli sketch new test`

**Compile**
`arduino-cli compile --fqbn arduino:avr:uno ./test`

**Upload**
`arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno ./test`

**Search library**
`arduino-cli lib search debouncer`

**Add library**
`arduino-cli lib install FTDebouncer`
