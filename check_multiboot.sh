#!/usr/bin/env bash
if grub-file --is-x86-multiboot build/palleyos.bin; then
  echo -e "\e[32mPASS: \e[0mmultiboot confirmed"
  exit 0
else
  echo -e "\e[31mFAIL: \e[0mthe file is not multiboot"
  exit 1
fi
