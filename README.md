# gattclient
A gatt client, based on bluez stack -- just using socket

# context
Bluez is one of the bluetooth stack for linux. I was looking for a gatt client to make some testing for ble.
Until know I was not successfull to get C code "easy" to understand with a simple architecture.
I found that with the btgatt-client in bluez

# compile context
The main file is in <bluez source rep>/tools/btgatt-client.c
But now, what are the files needed to compile it? I figured out that, here is the list

   * att.c & att.h
   * att-types.h
   * bluetooth.c & bluetooth.h
   * btgatt-client.c (main)
   * config.h
   * crypto.c
   * crypto.h
   * gatt-client.c & gatt-client.h
   * gatt-db.c & gatt-db.h
   * gatt-helpers.c & gatt-helpers.h
   * hci_lib.h
   * hci.c & hci.h
   * io.h
   * io-mainloop.c
   * l2cap.h
   * mainloop.c & mainloop.h
   * queue.c & queue.h
   * timeout.h
   * timeout-glib.c
   * util.c & util.h
   * uuid.c & uuid.h
   
most of the files are in <bluez>/src/shared

# project is under eclipse

you can clone with git and there is an eclipse project