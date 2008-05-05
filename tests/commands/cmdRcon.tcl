#!/bin/sh
# \
exec tclsh "$0" "$@"

source tests/commands/common.tcl

proc main {} {
 global server client serverout clientout

 # set defaults
 expr srand([clock clicks])
 set pass [expr int(rand()*1000000)]
 client "print_stdout 1"
 client "cl_name Player"

 wait

 # no passwords
 server "set rcon_password"
 clear
 client "set rcon_password"
 expect $serverout {}

 # password too short
 server "set rcon_password a"
 clear
 client "set rcon_password a"
 expect $serverout {}

 # bad password
 server "rcon_password $pass"
 clear
 client "rcon_password whatever"
 expect $serverout {}

 # good password
 clear
 client "rcon_password $pass"
 expect $serverout {rcon login from 127.0.0.1:10501}
 
 # rcon ability
 clear
 client "rcon say hi"
 expect $serverout {[console]: hi}
}

start

set error [catch { main }]

if { $error } {
 puts "FAIL Test crashed!"
}

end

