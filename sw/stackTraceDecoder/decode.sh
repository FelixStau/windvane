#!/bin/sh

DECODER="./EspStackTraceDecoder.jar"
ADDR2LINE="/../../bin/xtensa-lx106-elf-addr2line"
ELF="/../test.ino.elf"
STACK_FILE="./stacktrace.txt"

echo "__________________________________________________________"
java -jar $DECODER $ADDR1LINE $ELF $STACK_FILE
echo "__________________________________________________________"
