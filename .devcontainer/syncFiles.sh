#!/usr/bin/expect -f

set timeout -1

set USER pi
set HOST 192.168.178.150
set PASSW raspberry

spawn /bin/bash
expect "root@*"

send "cd /printer/.devcontainer\r"
expect "root@*"

send "mkdir -p rootfs rootfs/usr rootfs/opt\r"
expect "root@*"

send "rsync -avz --rsync-path='sudo rsync' --delete $USER@$HOST:/lib rootfs\r"
expect -re {\.*password:}
send -- "$PASSW\r"
expect "root@*"

send "rsync -avz --rsync-path='sudo rsync' --delete $USER@$HOST:/usr/include rootfs/usr\r"
expect -re {\.*password:}
send -- "$PASSW\r"
expect "root@*"

send "rsync -avz --rsync-path='sudo rsync' --delete $USER@$HOST:/usr/lib rootfs/usr\r"
expect -re {\.*password:}
send -- "$PASSW\r"
expect "root@*"

send "rsync -avz --rsync-path='sudo rsync' --delete $USER@$HOST:/opt/vc rootfs/opt\r" 
expect -re {\.*password:}
send -- "$PASSW\r"
expect "root@*"

send "python sysroot-relativelinks.py rootfs\r"

send "exit\r"
expect eof