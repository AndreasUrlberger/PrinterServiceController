#!/usr/bin/expect -f
set timeout -1

set USER pi
set HOST 192.168.178.143
set PASSW raspberry

spawn scp ./build/PrinterServiceController ./PrinterConfig.json $USER@$HOST:/home/pi/
expect -re {\.*password:}
send -- "$PASSW\r"
interact

spawn ssh $USER@$HOST
expect "password:"
send -- "$PASSW\r"

expect -re {\.*[$#]}
send -- "sudo ./PrinterServiceController\r"

interact