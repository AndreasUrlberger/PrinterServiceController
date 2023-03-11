#!/usr/bin/expect -f
set timeout -1

set USER pi
set HOST 192.168.178.155
set PASSW raspberry

spawn scp ./build/PrinterServiceController $USER@$HOST:/home/pi/
expect -re {\.*password:}
send -- "$PASSW\r"
interact

spawn ssh $USER@$HOST
expect "password:"
send -- "$PASSW\r"

expect -re {\.*[$#]}
send -- "./PrinterServiceController\r"

interact