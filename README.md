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

**Prepare to log**
`stty -F /dev/ttyACM0 cs8 9600 ignbrk -brkint -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts`

**Log /dev/tty/ACM0**
`cat < /dev/ttyACM0`

**Compile && Upload && Logs**
`arduino-cli compile ./curtain && arduino-cli upload ./curtain && cat < /dev/ttyACM0`


## Полезные ссылки:

- [EEPROM.h](https://alexgyver.ru/lessons/eeprom/)
- [Library](https://www.arduino.cc/en/Hacking/libraryTutorial)
