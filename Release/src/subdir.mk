################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/att.c \
../src/bluetooth.c \
../src/btgattclient.c \
../src/crypto.c \
../src/gatt-client.c \
../src/gatt-db.c \
../src/gatt-helpers.c \
../src/hci.c \
../src/io-mainloop.c \
../src/mainloop.c \
../src/queue.c \
../src/timeout-glib.c \
../src/util.c \
../src/uuid.c 

OBJS += \
./src/att.o \
./src/bluetooth.o \
./src/btgattclient.o \
./src/crypto.o \
./src/gatt-client.o \
./src/gatt-db.o \
./src/gatt-helpers.o \
./src/hci.o \
./src/io-mainloop.o \
./src/mainloop.o \
./src/queue.o \
./src/timeout-glib.o \
./src/util.o \
./src/uuid.o 

C_DEPS += \
./src/att.d \
./src/bluetooth.d \
./src/btgattclient.d \
./src/crypto.d \
./src/gatt-client.d \
./src/gatt-db.d \
./src/gatt-helpers.d \
./src/hci.d \
./src/io-mainloop.d \
./src/mainloop.d \
./src/queue.d \
./src/timeout-glib.d \
./src/util.d \
./src/uuid.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


