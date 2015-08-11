# gattclient
A gatt client, based on bluez stack -- just using socket

# context
Bluez is one of the bluetooth stack for linux. I was looking for a gatt client to make some testing for ble.
Until know I was not successfull to get C code "easy" to understand with a simple architecture.
I found that with the btgatt-client in bluez (based on version 5.32)

# compile context
The main file is in &lt;bluez source rep&gt;/tools/btgatt-client.c
But now, what are the files needed to compile it? I figured that out, here is the list

   * att.c & att.h
   * att-types.h
   * bluetooth.c & bluetooth.h
   * btgatt-client.c (main)
   * config.h
   * crypto.c & crypto.h
   * gatt-client.c & gatt-client.h
   * gatt-db.c & gatt-db.h
   * gatt-helpers.c & gatt-helpers.h
   * hci_lib.h
   * hci.c & hci.h
   * io.h
   * io-mainloop.c (don't take io-glib.c)
   * l2cap.h
   * mainloop.c & mainloop.h
   * queue.c & queue.h
   * timeout.h
   * timeout-glib.c
   * util.c & util.h
   * uuid.c & uuid.h
   
most of the files are in &lt;bluez&gt;/src/shared

# project is under eclipse

you can clone with git and there is an eclipse project

# Code Documentation
A doxygen documentation is provided, see the html directory (index.html file)
This is a work in progress with the intent of documenting all important functions and data structures

# How to use Gatt Client

<code>
$./gattclient help
Options:
	-i, --index <id>		Specify adapter index, e.g. hci0
	-d, --dest <addr>		Specify the destination address
	-t, --type [random|public] 	Specify the LE address type
	-m, --mtu <mtu> 		The ATT MTU to use
	-s, --security-level <sec> 	Set security level (low|medium|high)
	-v, --verbose			Enable extra logging
	-h, --help			Display help
Example:
gattclient -v -d C4:BE:84:70:29:04

[GATT client]#help
Commands:
	help           		Display help message
	services       		Show discovered services
	read-value     		Read a characteristic or descriptor value
	read-long-value		Read a long characteristic or desctriptor value
	read-multiple  		Read Multiple
	write-value    		Write a characteristic or descriptor value
	write-long-value	Write long characteristic or descriptor value
	write-prepare  		Write prepare characteristic or descriptor value
	write-execute  		Execute already prepared write
	register-notify		Subscribe to not/ind from a characteristic
	unregister-notify	Unregister a not/ind session
	set-security   		Set security level on le connection
	get-security   		Get security level on le connection
	set-sign-key   		Set signing key for signed write command
	quit           		Quit
[GATT client]# read-value
Usage: read-value <value_handle>
[GATT client]# read-long-value
Usage: read-long-value <value_handle> <offset>
[GATT client]# read-multiple
Usage: read-multiple <handle_1> <handle_2> ...
[GATT client]# write-value
Usage: write-value [options] <value_handle> <value>
Options:
	-w, --without-response	Write without response
	-s, --signed-write	Signed write command
e.g.:
	write-value 0x0001 00 01 00
[GATT client]# write-long-value
Usage: write-long-value [options] <value_handle> <offset> <value>
Options:
	-r, --reliable-write	Reliable write
e.g.:
	write-long-value 0x0001 0 00 01 00
[GATT client]# write-prepare
Usage: write-prepare [options] <value_handle> <offset> <value>
Options:
	-s, --session-id	Session id
e.g.:
	write-prepare -s 1 0x0001 00 01 00
[GATT client]# write-prepare
Usage: write-prepare [options] <value_handle> <offset> <value>
Options:
	-s, --session-id	Session id
e.g.:
	write-prepare -s 1 0x0001 00 01 00
[GATT client]# write-execute
Usage: write-execute <session_id> <execute>
e.g.:
	write-execute 1 0
[GATT client]# register-notify
Usage: register-notify <chrc value handle>
[GATT client]# unregister-notify	
Usage: unregister-notify <notify id>
[GATT client]# set-security
Usage: set_security <level>
level: 1-3
e.g.:
	set-sec-level 2
[GATT client]# get-security
Security level: 1
Usage: set-sign-key [options]
Options:
	 -c, --sign-key <csrk>	CSRK
e.g.:
	set-sign-key -c D8515948451FEA320DC05A2E88308188
</code>